#!/usr/bin/env python3
"""
Comprehensive test suite for Hearts AI API.

Tests cover:
- Health check endpoint
- /api/move endpoint with full AI config
- /api/play-one endpoint with default config
- Card format validation (string format: "AS", "10H", "2C")
- Game state validation
- Trick history
- Error handling
- Edge cases

Run: python test_api.py [--server URL]
"""

import requests
import json
import sys
import time
from typing import List, Dict, Any, Optional

# Default server URL
SERVER_URL = "http://localhost:8080"

# Card constants for readability
SUITS = ['S', 'D', 'C', 'H']  # Spades, Diamonds, Clubs, Hearts
RANKS = ['A', 'K', 'Q', 'J', '10', '9', '8', '7', '6', '5', '4', '3', '2']


class TestResult:
    def __init__(self, name: str, passed: bool, message: str = "", duration_ms: float = 0):
        self.name = name
        self.passed = passed
        self.message = message
        self.duration_ms = duration_ms


class HeartsAPITester:
    def __init__(self, server_url: str = SERVER_URL):
        self.server_url = server_url
        self.results: List[TestResult] = []

    def run_all_tests(self) -> bool:
        """Run all tests and return True if all pass."""
        print(f"\n{'='*60}")
        print(f"Hearts AI API Test Suite")
        print(f"Server: {self.server_url}")
        print(f"{'='*60}\n")

        # Health check
        self._run_test("Health Check", self.test_health_check)

        # Basic move tests
        self._run_test("Basic Move - First Trick (2C lead)", self.test_first_trick_lead)
        self._run_test("Basic Move - Follow Suit", self.test_follow_suit)
        self._run_test("Basic Move - Can't Follow Suit", self.test_cant_follow_suit)

        # Trick history tests
        self._run_test("Trick History - Empty", self.test_trick_history_empty)
        self._run_test("Trick History - One Completed Trick", self.test_trick_history_one)
        self._run_test("Trick History - Multiple Tricks", self.test_trick_history_multiple)

        # Current trick tests
        self._run_test("Current Trick - Leading", self.test_current_trick_leading)
        self._run_test("Current Trick - Second Player", self.test_current_trick_second)
        self._run_test("Current Trick - Last Player", self.test_current_trick_last)

        # Hearts broken tests
        self._run_test("Hearts Broken - Can Lead Hearts", self.test_hearts_broken_lead)
        self._run_test("Hearts Not Broken - Cannot Lead Hearts", self.test_hearts_not_broken)

        # AI config tests
        self._run_test("AI Config - Different Player Types", self.test_ai_player_types)
        self._run_test("AI Config - Simulation Count", self.test_ai_simulations)

        # Play-one endpoint tests
        self._run_test("Play One - Basic Request", self.test_play_one_basic)
        self._run_test("Play One - With Optional Config", self.test_play_one_with_config)

        # Card format tests
        self._run_test("Card Format - All Ranks", self.test_card_format_ranks)
        self._run_test("Card Format - All Suits", self.test_card_format_suits)
        self._run_test("Card Format - Response Format", self.test_card_format_response)

        # Error handling tests
        self._run_test("Error - Invalid Card Format", self.test_error_invalid_card)
        self._run_test("Error - Missing Game State", self.test_error_missing_game_state)
        self._run_test("Error - Empty Hand", self.test_error_empty_hand)
        self._run_test("Error - Invalid JSON", self.test_error_invalid_json)

        # Edge cases
        self._run_test("Edge Case - Single Card in Hand", self.test_single_card)
        self._run_test("Edge Case - Full 13 Card Hand", self.test_full_hand)
        self._run_test("Edge Case - Queen of Spades Avoidance", self.test_queen_spades)

        # Print summary
        self._print_summary()

        return all(r.passed for r in self.results)

    def _run_test(self, name: str, test_func):
        """Run a single test and record the result."""
        try:
            start = time.time()
            test_func()
            duration = (time.time() - start) * 1000
            self.results.append(TestResult(name, True, "OK", duration))
            print(f"  ✓ {name} ({duration:.1f}ms)")
        except AssertionError as e:
            self.results.append(TestResult(name, False, str(e)))
            print(f"  ✗ {name}: {e}")
        except Exception as e:
            self.results.append(TestResult(name, False, f"Exception: {e}"))
            print(f"  ✗ {name}: Exception - {e}")

    def _print_summary(self):
        """Print test summary."""
        passed = sum(1 for r in self.results if r.passed)
        total = len(self.results)
        print(f"\n{'='*60}")
        print(f"Results: {passed}/{total} tests passed")
        if passed == total:
            print("All tests passed! ✓")
        else:
            print("Some tests failed:")
            for r in self.results:
                if not r.passed:
                    print(f"  - {r.name}: {r.message}")
        print(f"{'='*60}\n")

    def _make_request(self, endpoint: str, data: Dict[str, Any]) -> requests.Response:
        """Make a POST request to the API."""
        url = f"{self.server_url}{endpoint}"
        return requests.post(url, json=data, timeout=30)

    def _make_move_request(self, game_state: Dict, ai_config: Optional[Dict] = None) -> Dict:
        """Make a /api/move request and return parsed response."""
        data = {"game_state": game_state}
        if ai_config:
            data["ai_config"] = ai_config
        response = self._make_request("/api/move", data)
        return response.json()

    def _create_game_state(
        self,
        player_hand: List[str],
        current_trick_cards: List[Dict] = None,
        lead_player: int = 0,
        trick_history: List[Dict] = None,
        scores: List[int] = None,
        hearts_broken: bool = False,
        pass_direction: int = 0,
        rules: int = 0
    ) -> Dict:
        """Create a game state object."""
        return {
            "player_hand": player_hand,
            "current_player": 0,
            "current_trick": {
                "cards": current_trick_cards or [],
                "lead_player": lead_player
            },
            "trick_history": trick_history or [],
            "scores": scores or [0, 0, 0, 0],
            "hearts_broken": hearts_broken,
            "pass_direction": pass_direction,
            "rules": rules
        }

    # ==================== Health Check ====================

    def test_health_check(self):
        """Test /api/health endpoint."""
        response = requests.get(f"{self.server_url}/api/health", timeout=5)
        assert response.status_code == 200, f"Expected 200, got {response.status_code}"
        data = response.json()
        assert data.get("status") == "ok", f"Expected status 'ok', got {data}"

    # ==================== Basic Move Tests ====================

    def test_first_trick_lead(self):
        """Test first trick - must lead 2C."""
        game_state = self._create_game_state(
            player_hand=["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"]
        )
        response = self._make_move_request(game_state, {"simulations": 100})

        assert response["status"] == "success", f"Expected success, got {response}"
        assert "move" in response, "Response missing 'move'"
        # First trick with 2C in hand must play 2C
        assert response["move"]["card"] == "2C", f"Expected 2C, got {response['move']['card']}"

    def test_follow_suit(self):
        """Test following suit when possible."""
        game_state = self._create_game_state(
            player_hand=["5C", "8C", "JD", "AS", "3S", "7D", "9H", "KC"],
            current_trick_cards=[
                {"player": 1, "card": "2C"},
                {"player": 2, "card": "AC"},
                {"player": 3, "card": "7C"}
            ],
            lead_player=1
        )
        response = self._make_move_request(game_state, {"simulations": 100})

        assert response["status"] == "success"
        card = response["move"]["card"]
        # Must follow suit (play a club)
        assert card.endswith("C"), f"Expected a Club, got {card}"

    def test_cant_follow_suit(self):
        """Test playing any card when can't follow suit."""
        game_state = self._create_game_state(
            player_hand=["5D", "8D", "JH", "AS", "3S", "7H", "9H", "KS"],
            current_trick_cards=[
                {"player": 1, "card": "2C"},
                {"player": 2, "card": "AC"}
            ],
            lead_player=1
        )
        response = self._make_move_request(game_state, {"simulations": 100})

        assert response["status"] == "success"
        card = response["move"]["card"]
        # Can't follow suit, card should be from hand
        assert card in ["5D", "8D", "JH", "AS", "3S", "7H", "9H", "KS"], f"Invalid card {card}"

    # ==================== Trick History Tests ====================

    def test_trick_history_empty(self):
        """Test with empty trick history (first trick)."""
        game_state = self._create_game_state(
            player_hand=["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"],
            trick_history=[]
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"

    def test_trick_history_one(self):
        """Test with one completed trick."""
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"],
            trick_history=[
                {
                    "cards": [
                        {"player": 0, "card": "2C"},
                        {"player": 1, "card": "AC"},
                        {"player": 2, "card": "7C"},
                        {"player": 3, "card": "3C"}
                    ],
                    "lead_player": 0,
                    "winner": 1
                }
            ],
            lead_player=1
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"

    def test_trick_history_multiple(self):
        """Test with multiple completed tricks."""
        trick_history = [
            {
                "cards": [
                    {"player": 0, "card": "2C"},
                    {"player": 1, "card": "AC"},
                    {"player": 2, "card": "7C"},
                    {"player": 3, "card": "3C"}
                ],
                "lead_player": 0,
                "winner": 1
            },
            {
                "cards": [
                    {"player": 1, "card": "KD"},
                    {"player": 2, "card": "5D"},
                    {"player": 3, "card": "2D"},
                    {"player": 0, "card": "JD"}
                ],
                "lead_player": 1,
                "winner": 1
            },
            {
                "cards": [
                    {"player": 1, "card": "4S"},
                    {"player": 2, "card": "8S"},
                    {"player": 3, "card": "2S"},
                    {"player": 0, "card": "AS"}
                ],
                "lead_player": 1,
                "winner": 0
            }
        ]
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "3S", "7D", "9H", "KC", "4D", "6S", "10H"],
            trick_history=trick_history,
            lead_player=0
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"

    # ==================== Current Trick Tests ====================

    def test_current_trick_leading(self):
        """Test when player is leading (empty current trick)."""
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC"],
            current_trick_cards=[],
            lead_player=0,
            trick_history=[
                {
                    "cards": [
                        {"player": 1, "card": "2C"},
                        {"player": 2, "card": "AC"},
                        {"player": 3, "card": "7C"},
                        {"player": 0, "card": "3C"}
                    ],
                    "lead_player": 1,
                    "winner": 2
                }
            ]
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"
        assert response["move"]["card"] in game_state["player_hand"]

    def test_current_trick_second(self):
        """Test when player is second to play."""
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC"],
            current_trick_cards=[
                {"player": 3, "card": "2D"}
            ],
            lead_player=3
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"

    def test_current_trick_last(self):
        """Test when player is last to play."""
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC"],
            current_trick_cards=[
                {"player": 1, "card": "4D"},
                {"player": 2, "card": "KD"},
                {"player": 3, "card": "2D"}
            ],
            lead_player=1
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"
        # Should follow suit if possible
        card = response["move"]["card"]
        assert card == "8D" or card == "7D", f"Should play a diamond, got {card}"

    # ==================== Hearts Broken Tests ====================

    def test_hearts_broken_lead(self):
        """Test leading hearts when hearts are broken."""
        game_state = self._create_game_state(
            player_hand=["5H", "8H", "JH", "AH"],  # Only hearts
            current_trick_cards=[],
            lead_player=0,
            hearts_broken=True,
            trick_history=[
                {
                    "cards": [
                        {"player": 1, "card": "2C"},
                        {"player": 2, "card": "AC"},
                        {"player": 3, "card": "7C"},
                        {"player": 0, "card": "3C"}
                    ],
                    "lead_player": 1,
                    "winner": 2
                }
            ]
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"
        # Should be able to lead hearts
        assert response["move"]["card"].endswith("H")

    def test_hearts_not_broken(self):
        """Test cannot lead hearts when hearts not broken."""
        game_state = self._create_game_state(
            player_hand=["5H", "8H", "JH", "AS", "3S"],  # Mix of hearts and spades
            current_trick_cards=[],
            lead_player=0,
            hearts_broken=False,
            trick_history=[
                {
                    "cards": [
                        {"player": 1, "card": "2C"},
                        {"player": 2, "card": "AC"},
                        {"player": 3, "card": "7C"},
                        {"player": 0, "card": "3C"}
                    ],
                    "lead_player": 1,
                    "winner": 2
                }
            ]
        )
        response = self._make_move_request(game_state, {"simulations": 100})
        assert response["status"] == "success"
        # Should not lead hearts
        assert response["move"]["card"].endswith("S"), f"Should not lead hearts, got {response['move']['card']}"

    # ==================== AI Config Tests ====================

    def test_ai_player_types(self):
        """Test different AI player types."""
        game_state = self._create_game_state(
            player_hand=["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"]
        )

        for player_type in ["simple", "safe_simple", "global", "global2", "global3"]:
            response = self._make_move_request(game_state, {
                "simulations": 50,
                "player_type": player_type
            })
            assert response["status"] == "success", f"Failed with player_type={player_type}"

    def test_ai_simulations(self):
        """Test different simulation counts."""
        game_state = self._create_game_state(
            player_hand=["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"]
        )

        for sims in [10, 100, 500]:
            response = self._make_move_request(game_state, {"simulations": sims})
            assert response["status"] == "success", f"Failed with simulations={sims}"
            assert "computation_time_ms" in response

    # ==================== Play-One Endpoint Tests ====================

    def test_play_one_basic(self):
        """Test /api/play-one with minimal request."""
        game_state = self._create_game_state(
            player_hand=["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"]
        )
        response = self._make_request("/api/play-one", {"game_state": game_state})
        data = response.json()

        assert data["status"] == "success", f"Expected success, got {data}"
        assert "move" in data
        assert data["move"]["card"] == "2C"

    def test_play_one_with_config(self):
        """Test /api/play-one with optional config overrides."""
        game_state = self._create_game_state(
            player_hand=["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS", "2H"],
            current_trick_cards=[
                {"player": 1, "card": "2C"}
            ],
            lead_player=1
        )
        response = self._make_request("/api/play-one", {
            "game_state": game_state,
            "simulations": 200,
            "player_type": "global"
        })
        data = response.json()

        assert data["status"] == "success"
        # Must follow suit
        assert data["move"]["card"] == "5C"

    # ==================== Card Format Tests ====================

    def test_card_format_ranks(self):
        """Test all rank values in card format."""
        # Create hand with one of each rank
        hand = [f"{rank}S" for rank in RANKS]
        game_state = self._create_game_state(
            player_hand=hand,
            current_trick_cards=[{"player": 1, "card": "2C"}],
            lead_player=1,
            hearts_broken=True
        )
        response = self._make_move_request(game_state, {"simulations": 50})
        assert response["status"] == "success"

    def test_card_format_suits(self):
        """Test all suit values in card format."""
        # Test each suit
        for suit in SUITS:
            hand = [f"A{suit}", f"K{suit}", f"Q{suit}", f"J{suit}", f"10{suit}"]
            game_state = self._create_game_state(
                player_hand=hand,
                current_trick_cards=[{"player": 1, "card": f"2{suit}"}],
                lead_player=1,
                hearts_broken=True
            )
            response = self._make_move_request(game_state, {"simulations": 50})
            assert response["status"] == "success", f"Failed with suit {suit}"
            # Should follow suit
            assert response["move"]["card"].endswith(suit)

    def test_card_format_response(self):
        """Test response card format is correct."""
        game_state = self._create_game_state(
            player_hand=["2C", "5D", "8H", "JS"]
        )
        response = self._make_move_request(game_state, {"simulations": 50})

        assert response["status"] == "success"
        card = response["move"]["card"]
        # Card should be a string
        assert isinstance(card, str), f"Card should be string, got {type(card)}"
        # Card should match format: rank + suit letter
        assert card[-1] in "SDCH", f"Invalid suit in card {card}"
        rank = card[:-1]
        assert rank in RANKS, f"Invalid rank in card {card}"

    # ==================== Error Handling Tests ====================

    def test_error_invalid_card(self):
        """Test error handling for invalid card format."""
        game_state = self._create_game_state(
            player_hand=["2C", "INVALID", "8H"]
        )
        response = self._make_move_request(game_state, {"simulations": 50})
        assert response["status"] == "error"

    def test_error_missing_game_state(self):
        """Test error handling for missing game_state."""
        response = self._make_request("/api/move", {"ai_config": {"simulations": 100}})
        data = response.json()
        assert data["status"] == "error"

    def test_error_empty_hand(self):
        """Test error handling for empty hand."""
        game_state = self._create_game_state(player_hand=[])
        response = self._make_move_request(game_state, {"simulations": 50})
        assert response["status"] == "error"

    def test_error_invalid_json(self):
        """Test error handling for invalid JSON."""
        response = requests.post(
            f"{self.server_url}/api/move",
            data="not valid json",
            headers={"Content-Type": "application/json"},
            timeout=5
        )
        data = response.json()
        assert data["status"] == "error"

    # ==================== Edge Case Tests ====================

    def test_single_card(self):
        """Test with single card in hand (forced move)."""
        game_state = self._create_game_state(
            player_hand=["AH"],
            hearts_broken=True
        )
        response = self._make_move_request(game_state, {"simulations": 50})

        assert response["status"] == "success"
        assert response["move"]["card"] == "AH"

    def test_full_hand(self):
        """Test with full 13 card hand."""
        hand = ["2C", "5C", "8C", "JC", "AC",
                "3D", "7D", "KD",
                "4S", "9S", "QS",
                "6H", "10H"]
        game_state = self._create_game_state(player_hand=hand)
        response = self._make_move_request(game_state, {"simulations": 100})

        assert response["status"] == "success"
        # Should play 2C on first trick
        assert response["move"]["card"] == "2C"

    def test_queen_spades(self):
        """Test Queen of Spades handling."""
        # Player has QS and must follow spades
        game_state = self._create_game_state(
            player_hand=["QS", "KS", "AS", "5H", "6H"],
            current_trick_cards=[
                {"player": 1, "card": "2S"},
                {"player": 2, "card": "4S"},
                {"player": 3, "card": "7S"}
            ],
            lead_player=1,
            trick_history=[
                {
                    "cards": [
                        {"player": 0, "card": "2C"},
                        {"player": 1, "card": "AC"},
                        {"player": 2, "card": "7C"},
                        {"player": 3, "card": "3C"}
                    ],
                    "lead_player": 0,
                    "winner": 1
                }
            ]
        )
        response = self._make_move_request(game_state, {"simulations": 200})

        assert response["status"] == "success"
        card = response["move"]["card"]
        # Should follow suit with a spade
        assert card.endswith("S"), f"Should play spade, got {card}"


def main():
    # Parse command line args
    server_url = SERVER_URL
    if len(sys.argv) > 1:
        if sys.argv[1] == "--server" and len(sys.argv) > 2:
            server_url = sys.argv[2]
        elif sys.argv[1].startswith("http"):
            server_url = sys.argv[1]

    # Run tests
    tester = HeartsAPITester(server_url)
    success = tester.run_all_tests()

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
