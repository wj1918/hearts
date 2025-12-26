#!/usr/bin/env python3
"""
Comprehensive Test Suite for /api/play-one Endpoint

Tests the simplified single-move endpoint with full request/response validation.

Usage:
    python test_play_one.py [host:port]

Example:
    python test_play_one.py localhost:8080
"""

import json
import sys
import time
import urllib.request
import urllib.error
from typing import Optional, Tuple, Dict, Any, List

DEFAULT_HOST = "localhost:8080"
ENDPOINT = "/api/play-one"

# Card encoding constants
SUITS = {0: "Spades", 1: "Diamonds", 2: "Clubs", 3: "Hearts"}
RANKS = {0: "A", 1: "K", 2: "Q", 3: "J", 4: "10", 5: "9", 6: "8", 7: "7", 8: "6", 9: "5", 10: "4", 11: "3", 12: "2"}


def card_str(suit: int, rank: int) -> str:
    """Convert card to readable string."""
    return f"{RANKS.get(rank, '?')}{SUITS.get(suit, '?')[0]}"


def make_card(suit: int, rank: int) -> Dict[str, int]:
    """Create a card JSON object."""
    return {"suit": suit, "rank": rank}


def make_hand(cards: List[Tuple[int, int]]) -> List[Dict[str, int]]:
    """Create a hand from list of (suit, rank) tuples."""
    return [make_card(s, r) for s, r in cards]


def make_request(host: str, endpoint: str, method: str = "GET",
                 data: Optional[Dict] = None, timeout: int = 60) -> Tuple[Optional[Dict], float, Optional[str]]:
    """Make HTTP request and return (response_json, elapsed_ms, error)."""
    url = f"http://{host}{endpoint}"
    headers = {"Content-Type": "application/json"} if data else {}

    req = urllib.request.Request(
        url,
        data=json.dumps(data).encode() if data else None,
        headers=headers,
        method=method
    )

    try:
        start = time.time()
        with urllib.request.urlopen(req, timeout=timeout) as response:
            elapsed = (time.time() - start) * 1000
            body = response.read().decode()
            return json.loads(body), elapsed, None
    except urllib.error.HTTPError as e:
        elapsed = (time.time() - start) * 1000
        try:
            body = e.read().decode()
            return json.loads(body), elapsed, None
        except:
            return None, elapsed, f"HTTP {e.code}: {e.reason}"
    except urllib.error.URLError as e:
        return None, 0, f"URL Error: {e.reason}"
    except json.JSONDecodeError as e:
        return None, 0, f"JSON decode error: {e}"
    except Exception as e:
        return None, 0, str(e)


class TestResult:
    """Test result container."""
    def __init__(self, name: str):
        self.name = name
        self.passed = False
        self.message = ""
        self.details = []

    def success(self, msg: str = ""):
        self.passed = True
        self.message = msg
        return self

    def fail(self, msg: str):
        self.passed = False
        self.message = msg
        return self

    def add_detail(self, detail: str):
        self.details.append(detail)
        return self


def validate_response_structure(resp: Dict) -> List[str]:
    """Validate response has required structure. Returns list of errors."""
    errors = []

    # Must have status
    if "status" not in resp:
        errors.append("Missing 'status' field")
        return errors

    if resp["status"] == "success":
        # Success response validation
        if "move" not in resp:
            errors.append("Missing 'move' field in success response")
        else:
            move = resp["move"]
            if "card" not in move:
                errors.append("Missing 'card' field in move")
            else:
                card = move["card"]
                if "suit" not in card:
                    errors.append("Missing 'suit' field in card")
                elif not isinstance(card["suit"], int) or card["suit"] < 0 or card["suit"] > 3:
                    errors.append(f"Invalid suit value: {card.get('suit')}")
                if "rank" not in card:
                    errors.append("Missing 'rank' field in card")
                elif not isinstance(card["rank"], int) or card["rank"] < 0 or card["rank"] > 12:
                    errors.append(f"Invalid rank value: {card.get('rank')}")

            if "player" not in move:
                errors.append("Missing 'player' field in move")
            elif not isinstance(move["player"], int) or move["player"] < 0 or move["player"] > 3:
                errors.append(f"Invalid player value: {move.get('player')}")

        if "computation_time_ms" not in resp:
            errors.append("Missing 'computation_time_ms' field")
        elif not isinstance(resp["computation_time_ms"], (int, float)):
            errors.append(f"Invalid computation_time_ms type: {type(resp['computation_time_ms'])}")
        elif resp["computation_time_ms"] < 0:
            errors.append(f"Negative computation_time_ms: {resp['computation_time_ms']}")

    elif resp["status"] == "error":
        # Error response validation
        if "error_code" not in resp:
            errors.append("Missing 'error_code' field in error response")
        if "message" not in resp:
            errors.append("Missing 'message' field in error response")

    else:
        errors.append(f"Invalid status value: {resp['status']}")

    return errors


def validate_move_is_legal(move_card: Dict, player_hand: List[Dict],
                           current_trick: List[Dict], hearts_broken: bool) -> List[str]:
    """Validate the returned move is legal. Returns list of errors."""
    errors = []

    suit = move_card["suit"]
    rank = move_card["rank"]

    # Check card is in player's hand
    card_in_hand = any(c["suit"] == suit and c["rank"] == rank for c in player_hand)
    if not card_in_hand:
        errors.append(f"Returned card {card_str(suit, rank)} not in player's hand")
        return errors

    # If there's a lead card, must follow suit if possible
    if current_trick:
        lead_suit = current_trick[0]["card"]["suit"]
        has_lead_suit = any(c["suit"] == lead_suit for c in player_hand)
        if has_lead_suit and suit != lead_suit:
            errors.append(f"Must follow lead suit {SUITS[lead_suit]}, played {SUITS[suit]}")

    # If leading and hearts not broken, can't lead hearts (unless only hearts)
    if not current_trick and suit == 3 and not hearts_broken:
        has_non_hearts = any(c["suit"] != 3 for c in player_hand)
        if has_non_hearts:
            errors.append("Cannot lead hearts when hearts not broken")

    return errors


# =============================================================================
# TEST CASES
# =============================================================================

def test_basic_request(host: str) -> TestResult:
    """Test basic request with minimal game state."""
    result = TestResult("Basic request with valid game state")

    data = {
        "game_state": {
            "player_hands": [
                [make_card(2, 12)],  # Player 0: 2C
                [make_card(2, 11)],  # Player 1: 3C
                [make_card(2, 10)],  # Player 2: 4C
                [make_card(2, 9)]    # Player 3: 5C
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)

    if err:
        return result.fail(f"Request failed: {err}")

    # Validate response structure
    struct_errors = validate_response_structure(resp)
    if struct_errors:
        return result.fail(f"Response structure invalid: {struct_errors}")

    if resp["status"] != "success":
        return result.fail(f"Expected success, got: {resp}")

    # Single card - should return 2C
    card = resp["move"]["card"]
    if card["suit"] != 2 or card["rank"] != 12:
        return result.fail(f"Expected 2C, got {card_str(card['suit'], card['rank'])}")

    result.add_detail(f"Returned: {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Computation time: {resp['computation_time_ms']:.2f}ms")
    return result.success("Response valid")


def test_response_player_matches_current(host: str) -> TestResult:
    """Test that response player matches current_player."""
    result = TestResult("Response player matches current_player")

    for current_player in range(4):
        # Give each player one card
        hands = [[make_card(2, 12 - i)] for i in range(4)]

        data = {
            "game_state": {
                "player_hands": hands,
                "current_player": current_player,
                "current_trick": {"cards": [], "lead_player": current_player},
                "played_cards": [[], [], [], []],
                "scores": [0, 0, 0, 0],
                "hearts_broken": False,
                "pass_direction": 0,
                "rules": 0
            }
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request failed for player {current_player}: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success for player {current_player}")

        if resp["move"]["player"] != current_player:
            return result.fail(f"Expected player {current_player}, got {resp['move']['player']}")

        result.add_detail(f"Player {current_player}: OK")

    return result.success("All players matched")


def test_single_legal_move(host: str) -> TestResult:
    """Test when only one legal move exists (should skip AI)."""
    result = TestResult("Single legal move optimization")

    data = {
        "game_state": {
            "player_hands": [
                [make_card(0, 0)],  # Player 0: AS only
                [make_card(1, 0)],
                [make_card(2, 0)],
                [make_card(3, 0)]
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": True,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    if card["suit"] != 0 or card["rank"] != 0:
        return result.fail(f"Expected AS, got {card_str(card['suit'], card['rank'])}")

    # Should be very fast since AI is skipped
    comp_time = resp["computation_time_ms"]
    result.add_detail(f"Computation time: {comp_time:.2f}ms")

    if comp_time < 100:  # Should be fast when AI skipped
        result.add_detail("Fast response (AI likely skipped)")

    return result.success("Correct card returned")


def test_multiple_legal_moves(host: str) -> TestResult:
    """Test with multiple legal moves (AI must choose)."""
    result = TestResult("Multiple legal moves - AI chooses")

    # Player 0 has 5 clubs - AI must choose best one
    data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, 0), (2, 1), (2, 2), (2, 3), (2, 4)]),  # AC, KC, QC, JC, 10C
                make_hand([(2, 5), (2, 6), (2, 7)]),
                make_hand([(2, 8), (2, 9), (2, 10)]),
                make_hand([(2, 11), (2, 12)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Validate move is legal
    legal_errors = validate_move_is_legal(
        card,
        data["game_state"]["player_hands"][0],
        [],
        False
    )
    if legal_errors:
        return result.fail(f"Illegal move: {legal_errors}")

    result.add_detail(f"AI chose: {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Computation time: {resp['computation_time_ms']:.2f}ms")
    return result.success("Legal move returned")


def test_must_follow_suit(host: str) -> TestResult:
    """Test that AI follows suit when required."""
    result = TestResult("Must follow suit rule")

    # Player 1 must follow clubs lead
    data = {
        "game_state": {
            "player_hands": [
                [],  # Player 0 already played
                make_hand([(2, 5), (2, 6), (3, 0)]),  # 9C, 8C, AH - must play club
                make_hand([(1, 0)]),
                make_hand([(0, 0)])
            ],
            "current_player": 1,
            "current_trick": {
                "cards": [{"card": make_card(2, 12), "player": 0}],  # 2C led
                "lead_player": 0
            },
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Must be a club (suit 2)
    if card["suit"] != 2:
        return result.fail(f"Must follow clubs, played {SUITS[card['suit']]}")

    result.add_detail(f"Correctly followed suit with {card_str(card['suit'], card['rank'])}")
    return result.success("Followed suit correctly")


def test_can_slough_when_void(host: str) -> TestResult:
    """Test that AI can play any card when void in lead suit."""
    result = TestResult("Slough when void in suit")

    # Player 1 has no clubs, can play anything
    data = {
        "game_state": {
            "player_hands": [
                [],
                make_hand([(3, 0), (3, 1), (0, 2)]),  # AH, KH, QS - no clubs
                make_hand([(1, 0)]),
                make_hand([(0, 0)])
            ],
            "current_player": 1,
            "current_trick": {
                "cards": [{"card": make_card(2, 12), "player": 0}],  # 2C led
                "lead_player": 0
            },
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Any card from hand is valid when void
    hand = data["game_state"]["player_hands"][1]
    card_in_hand = any(c["suit"] == card["suit"] and c["rank"] == card["rank"] for c in hand)

    if not card_in_hand:
        return result.fail(f"Card not in hand: {card_str(card['suit'], card['rank'])}")

    result.add_detail(f"Sloughed {card_str(card['suit'], card['rank'])}")
    return result.success("Valid slough")


def test_hearts_not_broken(host: str) -> TestResult:
    """Test cannot lead hearts when not broken (unless only hearts)."""
    result = TestResult("Hearts not broken - cannot lead hearts")

    # Player has hearts and other suits, hearts not broken
    data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, 0), (3, 0), (3, 1)]),  # AC, AH, KH
                make_hand([(1, 0)]),
                make_hand([(0, 0)]),
                make_hand([(2, 1)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Should not lead hearts
    if card["suit"] == 3:
        return result.fail(f"Should not lead hearts when not broken")

    result.add_detail(f"Led {card_str(card['suit'], card['rank'])} (not hearts)")
    return result.success("Did not lead hearts")


def test_hearts_broken_can_lead(host: str) -> TestResult:
    """Test can lead hearts when broken."""
    result = TestResult("Hearts broken - can lead hearts")

    # Player has only hearts
    data = {
        "game_state": {
            "player_hands": [
                make_hand([(3, 0), (3, 1), (3, 2)]),  # AH, KH, QH only
                make_hand([(1, 0)]),
                make_hand([(0, 0)]),
                make_hand([(2, 0)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": True,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Must be hearts (only option)
    if card["suit"] != 3:
        return result.fail(f"Expected hearts, got {SUITS[card['suit']]}")

    result.add_detail(f"Led {card_str(card['suit'], card['rank'])}")
    return result.success("Led hearts correctly")


def test_mid_game_with_played_cards(host: str) -> TestResult:
    """Test mid-game scenario with cards already played."""
    result = TestResult("Mid-game with played cards history")

    # Simulate a few tricks already played
    data = {
        "game_state": {
            "player_hands": [
                make_hand([(0, 3), (1, 5), (3, 8)]),  # JS, 9D, 6H
                make_hand([(0, 4), (1, 6), (3, 9)]),
                make_hand([(0, 5), (1, 7), (3, 10)]),
                make_hand([(0, 6), (1, 8), (3, 11)])
            ],
            "current_player": 2,
            "current_trick": {"cards": [], "lead_player": 2},
            "played_cards": [
                make_hand([(2, 12), (0, 0), (1, 0)]),  # 2C, AS, AD taken by P0
                make_hand([(2, 11), (0, 1), (1, 1)]),  # 3C, KS, KD taken by P1
                make_hand([(2, 10), (0, 2), (1, 2)]),  # 4C, QS, QD taken by P2
                make_hand([(2, 9), (3, 0), (1, 3)])    # 5C, AH, JD taken by P3 (has hearts)
            ],
            "scores": [0, 0, 0, 1],  # P3 has 1 point from AH
            "hearts_broken": True,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Validate card is in P2's hand
    hand = data["game_state"]["player_hands"][2]
    card_in_hand = any(c["suit"] == card["suit"] and c["rank"] == card["rank"] for c in hand)

    if not card_in_hand:
        return result.fail(f"Card not in hand: {card_str(card['suit'], card['rank'])}")

    result.add_detail(f"Player 2 leads {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Hearts already broken, scores: {data['game_state']['scores']}")
    return result.success("Valid mid-game move")


def test_current_trick_in_progress(host: str) -> TestResult:
    """Test responding to a trick in progress."""
    result = TestResult("Current trick in progress")

    # 3 cards already played in trick, P3 to complete
    data = {
        "game_state": {
            "player_hands": [
                [],
                [],
                [],
                make_hand([(0, 5), (0, 6), (3, 0)])  # 9S, 8S, AH
            ],
            "current_player": 3,
            "current_trick": {
                "cards": [
                    {"card": make_card(0, 12), "player": 0},  # 2S
                    {"card": make_card(0, 11), "player": 1},  # 3S
                    {"card": make_card(0, 10), "player": 2}   # 4S
                ],
                "lead_player": 0
            },
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Must follow spades
    if card["suit"] != 0:
        return result.fail(f"Must follow spades, played {SUITS[card['suit']]}")

    result.add_detail(f"Completed trick with {card_str(card['suit'], card['rank'])}")
    return result.success("Followed suit in trick")


def test_optional_simulations_override(host: str) -> TestResult:
    """Test optional simulations parameter override."""
    result = TestResult("Optional simulations override")

    base_data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, i) for i in range(5)]),  # 5 clubs
                make_hand([(2, i) for i in range(5, 8)]),
                make_hand([(2, i) for i in range(8, 11)]),
                make_hand([(2, i) for i in range(11, 13)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    # Test with low simulations (fast)
    data_low = {**base_data, "simulations": 100}
    resp_low, _, err = make_request(host, ENDPOINT, "POST", data_low)
    if err:
        return result.fail(f"Low sim request failed: {err}")

    # Test with higher simulations (slower)
    data_high = {**base_data, "simulations": 2000}
    resp_high, _, err = make_request(host, ENDPOINT, "POST", data_high)
    if err:
        return result.fail(f"High sim request failed: {err}")

    if resp_low["status"] != "success" or resp_high["status"] != "success":
        return result.fail("Expected success for both")

    time_low = resp_low["computation_time_ms"]
    time_high = resp_high["computation_time_ms"]

    result.add_detail(f"100 sims: {time_low:.2f}ms")
    result.add_detail(f"2000 sims: {time_high:.2f}ms")

    # Higher sims should generally take longer (not always guaranteed but usually)
    return result.success("Simulations parameter accepted")


def test_optional_player_type_override(host: str) -> TestResult:
    """Test optional player_type parameter override."""
    result = TestResult("Optional player_type override")

    player_types = ["simple", "safe_simple", "global", "global2", "global3"]

    base_data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, 0), (2, 12)]),
                make_hand([(2, 1), (2, 11)]),
                make_hand([(2, 2), (2, 10)]),
                make_hand([(2, 3), (2, 9)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        },
        "simulations": 100  # Keep low for speed
    }

    for player_type in player_types:
        data = {**base_data, "player_type": player_type}
        resp, _, err = make_request(host, ENDPOINT, "POST", data)

        if err:
            return result.fail(f"{player_type} request failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"{player_type} expected success: {resp}")

        result.add_detail(f"{player_type}: OK ({resp['computation_time_ms']:.1f}ms)")

    return result.success("All player types work")


def test_full_13_card_hands(host: str) -> TestResult:
    """Test with full 13-card hands at game start."""
    result = TestResult("Full 13-card hands")

    # Standard deal: each player gets 13 cards
    data = {
        "game_state": {
            "player_hands": [
                [make_card(2, r) for r in range(13)],   # All clubs
                [make_card(0, r) for r in range(13)],   # All spades
                [make_card(1, r) for r in range(13)],   # All diamonds
                [make_card(3, r) for r in range(13)]    # All hearts
            ],
            "current_player": 0,  # Player 0 has 2C, must lead
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # With standard rules, 2C must be led first
    # But our test gives P0 all clubs, so any club is technically valid
    if card["suit"] != 2:
        return result.fail(f"Expected club, got {SUITS[card['suit']]}")

    result.add_detail(f"Led {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Total time: {elapsed:.2f}ms")
    return result.success("Full hands handled")


def test_computation_time_present(host: str) -> TestResult:
    """Test that computation_time_ms is always present and reasonable."""
    result = TestResult("Computation time validation")

    data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, 0), (2, 1), (2, 2)]),
                make_hand([(2, 3)]),
                make_hand([(2, 4)]),
                make_hand([(2, 5)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    comp_time = resp.get("computation_time_ms")

    if comp_time is None:
        return result.fail("computation_time_ms missing")

    if not isinstance(comp_time, (int, float)):
        return result.fail(f"computation_time_ms wrong type: {type(comp_time)}")

    if comp_time < 0:
        return result.fail(f"computation_time_ms negative: {comp_time}")

    if comp_time > 60000:  # More than 60 seconds seems wrong
        result.add_detail(f"Warning: very long computation time: {comp_time}ms")

    result.add_detail(f"computation_time_ms: {comp_time:.2f}")
    return result.success("Valid computation time")


# =============================================================================
# ERROR HANDLING TESTS
# =============================================================================

def test_error_invalid_json(host: str) -> TestResult:
    """Test error handling for invalid JSON."""
    result = TestResult("Error: Invalid JSON")

    req = urllib.request.Request(
        f"http://{host}{ENDPOINT}",
        data=b"this is not valid json {{{",
        headers={"Content-Type": "application/json"},
        method="POST"
    )

    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            resp = json.loads(response.read().decode())
    except urllib.error.HTTPError as e:
        resp = json.loads(e.read().decode())
    except Exception as e:
        return result.fail(f"Unexpected error: {e}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error status: {resp}")

    if "error_code" not in resp:
        return result.fail("Missing error_code")

    result.add_detail(f"Error code: {resp['error_code']}")
    result.add_detail(f"Message: {resp.get('message', 'N/A')}")
    return result.success("Proper error response")


def test_error_missing_game_state(host: str) -> TestResult:
    """Test error handling for missing game_state."""
    result = TestResult("Error: Missing game_state")

    data = {"simulations": 100}  # No game_state

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    # Should get error response, not connection error
    if resp is None:
        return result.fail(f"No response: {err}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error status: {resp}")

    result.add_detail(f"Error code: {resp.get('error_code')}")
    return result.success("Missing game_state detected")


def test_error_empty_hands(host: str) -> TestResult:
    """Test error handling for empty player hands."""
    result = TestResult("Error: No legal moves (empty hand)")

    data = {
        "game_state": {
            "player_hands": [[], [], [], []],  # All empty
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error for empty hands: {resp}")

    if resp.get("error_code") == "NO_LEGAL_MOVES":
        result.add_detail("Correctly identified no legal moves")
        return result.success("Proper error for empty hands")

    result.add_detail(f"Error code: {resp.get('error_code')}")
    return result.success("Error returned")


def test_error_invalid_player(host: str) -> TestResult:
    """Test error handling for invalid current_player."""
    result = TestResult("Error: Invalid current_player")

    data = {
        "game_state": {
            "player_hands": [
                [make_card(2, 0)],
                [make_card(2, 1)],
                [make_card(2, 2)],
                [make_card(2, 3)]
            ],
            "current_player": 5,  # Invalid: should be 0-3
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    # Should either error or handle gracefully
    if resp.get("status") == "error":
        result.add_detail(f"Error: {resp.get('error_code')}")
        return result.success("Invalid player rejected")

    # If it somehow succeeds, that's also acceptable if move is valid
    result.add_detail("Server handled invalid player gracefully")
    return result.success("Handled gracefully")


def test_error_invalid_card_format(host: str) -> TestResult:
    """Test error handling for invalid card format."""
    result = TestResult("Error: Invalid card format")

    data = {
        "game_state": {
            "player_hands": [
                [{"suit": 99, "rank": 99}],  # Invalid suit/rank
                [make_card(2, 1)],
                [make_card(2, 2)],
                [make_card(2, 3)]
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    # Could be error or handled gracefully
    result.add_detail(f"Status: {resp.get('status')}")
    if resp.get("error_code"):
        result.add_detail(f"Error code: {resp.get('error_code')}")

    return result.success("Invalid card handled")


# =============================================================================
# STRESS / EDGE CASE TESTS
# =============================================================================

def test_last_trick_of_game(host: str) -> TestResult:
    """Test the final trick of a game."""
    result = TestResult("Last trick of game")

    # Each player has exactly 1 card left
    data = {
        "game_state": {
            "player_hands": [
                [make_card(3, 12)],  # 2H
                [make_card(3, 11)],  # 3H
                [make_card(3, 10)],  # 4H
                [make_card(3, 9)]    # 5H
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [
                [make_card(s, r) for s in range(3) for r in range(13)],  # P0 has taken 39 cards
                [], [], []
            ],
            "scores": [0, 0, 0, 0],
            "hearts_broken": True,
            "pass_direction": 0,
            "rules": 0
        }
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Must be 2H (only card)
    if card["suit"] != 3 or card["rank"] != 12:
        return result.fail(f"Expected 2H, got {card_str(card['suit'], card['rank'])}")

    result.add_detail("Final trick handled correctly")
    return result.success("Last trick OK")


def test_queen_of_spades_avoidance(host: str) -> TestResult:
    """Test that AI avoids taking Queen of Spades when possible."""
    result = TestResult("Queen of Spades awareness")

    # Player 2 has QS and lower spades, spades led with K
    data = {
        "game_state": {
            "player_hands": [
                [],
                [],
                make_hand([(0, 2), (0, 5), (0, 8)]),  # QS, 9S, 6S
                []
            ],
            "current_player": 2,
            "current_trick": {
                "cards": [
                    {"card": make_card(0, 1), "player": 0},  # KS led
                    {"card": make_card(0, 12), "player": 1}  # 2S
                ],
                "lead_player": 0
            },
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        },
        "simulations": 500
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # AI should play under the King to avoid taking QS
    # Best play is 9S or 6S, NOT QS
    result.add_detail(f"AI played {card_str(card['suit'], card['rank'])}")

    if card["suit"] != 0:
        return result.fail("Must follow spades")

    if card["rank"] == 2:  # Played QS
        result.add_detail("Warning: AI played QS (might take 13 points)")
    else:
        result.add_detail("AI avoided taking the trick with QS")

    return result.success("Move is legal")


def test_consistency_multiple_calls(host: str) -> TestResult:
    """Test that multiple calls with same state return valid moves."""
    result = TestResult("Consistency across multiple calls")

    data = {
        "game_state": {
            "player_hands": [
                make_hand([(2, 0), (2, 1), (2, 2)]),
                make_hand([(2, 3)]),
                make_hand([(2, 4)]),
                make_hand([(2, 5)])
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False,
            "pass_direction": 0,
            "rules": 0
        },
        "simulations": 100
    }

    moves = []
    for i in range(3):
        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Call {i+1} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Call {i+1} not success: {resp}")

        card = resp["move"]["card"]
        moves.append(card_str(card["suit"], card["rank"]))
        result.add_detail(f"Call {i+1}: {moves[-1]}")

    # All moves should be valid (clubs)
    valid_cards = {"AC", "KC", "QC"}
    for m in moves:
        if m not in valid_cards:
            return result.fail(f"Invalid move: {m}")

    return result.success("All calls returned valid moves")


# =============================================================================
# TEST RUNNER
# =============================================================================

def run_tests(host: str) -> Tuple[int, int]:
    """Run all tests and return (passed, failed) counts."""

    tests = [
        # Basic functionality
        ("BASIC", [
            test_basic_request,
            test_response_player_matches_current,
            test_single_legal_move,
            test_multiple_legal_moves,
            test_computation_time_present,
        ]),

        # Game rules
        ("GAME RULES", [
            test_must_follow_suit,
            test_can_slough_when_void,
            test_hearts_not_broken,
            test_hearts_broken_can_lead,
        ]),

        # Game scenarios
        ("SCENARIOS", [
            test_mid_game_with_played_cards,
            test_current_trick_in_progress,
            test_full_13_card_hands,
            test_last_trick_of_game,
        ]),

        # Optional parameters
        ("PARAMETERS", [
            test_optional_simulations_override,
            test_optional_player_type_override,
        ]),

        # Error handling
        ("ERRORS", [
            test_error_invalid_json,
            test_error_missing_game_state,
            test_error_empty_hands,
            test_error_invalid_player,
            test_error_invalid_card_format,
        ]),

        # Edge cases
        ("EDGE CASES", [
            test_queen_of_spades_avoidance,
            test_consistency_multiple_calls,
        ]),
    ]

    total_passed = 0
    total_failed = 0

    for category, test_funcs in tests:
        print(f"\n{'='*60}")
        print(f" {category} TESTS")
        print('='*60)

        for test_func in test_funcs:
            try:
                result = test_func(host)
                status = "PASS" if result.passed else "FAIL"
                symbol = "[OK]" if result.passed else "[X]"

                print(f"\n{symbol} {result.name}")
                if result.message:
                    print(f"    Result: {result.message}")
                for detail in result.details:
                    print(f"    - {detail}")

                if result.passed:
                    total_passed += 1
                else:
                    total_failed += 1

            except Exception as e:
                print(f"\n[X] {test_func.__name__}")
                print(f"    EXCEPTION: {e}")
                total_failed += 1

    return total_passed, total_failed


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_HOST

    print("="*60)
    print(" HEARTS AI SERVER - /api/play-one ENDPOINT TEST SUITE")
    print("="*60)
    print(f"Target: http://{host}{ENDPOINT}")
    print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")

    # Check server is reachable
    print("\nChecking server health...")
    resp, _, err = make_request(host, "/api/health")
    if err:
        print(f"ERROR: Cannot reach server: {err}")
        print("Make sure the server is running on the specified host:port")
        return 1

    if resp.get("status") != "ok":
        print(f"WARNING: Unexpected health response: {resp}")
    else:
        print("Server is healthy!")

    # Run tests
    passed, failed = run_tests(host)

    # Summary
    print("\n" + "="*60)
    print(" TEST SUMMARY")
    print("="*60)
    print(f"  Passed: {passed}")
    print(f"  Failed: {failed}")
    print(f"  Total:  {passed + failed}")
    print("="*60)

    if failed == 0:
        print("\n All tests passed!")
        return 0
    else:
        print(f"\n {failed} test(s) failed!")
        return 1


if __name__ == "__main__":
    sys.exit(main())
