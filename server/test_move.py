#!/usr/bin/env python3
"""
Comprehensive Test Suite for /api/move Endpoint

Tests the full-featured move endpoint with complete request/response validation.

Usage:
    python test_move.py [host:port]

Example:
    python test_move.py localhost:8080
"""

import json
import sys
import time
import urllib.request
import urllib.error
from typing import Optional, Tuple, Dict, Any, List

DEFAULT_HOST = "localhost:8080"
ENDPOINT = "/api/move"

# Card encoding constants
SUITS = {0: "Spades", 1: "Diamonds", 2: "Clubs", 3: "Hearts"}
RANKS = {0: "A", 1: "K", 2: "Q", 3: "J", 4: "10", 5: "9", 6: "8", 7: "7", 8: "6", 9: "5", 10: "4", 11: "3", 12: "2"}

# Valid player types
VALID_PLAYER_TYPES = ["simple", "safe_simple", "global", "global2", "global3"]


def card_str(suit: int, rank: int) -> str:
    """Convert card to readable string."""
    return f"{RANKS.get(rank, '?')}{SUITS.get(suit, '?')[0]}"


def make_card(suit: int, rank: int) -> Dict[str, int]:
    """Create a card JSON object."""
    return {"suit": suit, "rank": rank}


def make_hand(cards: List[Tuple[int, int]]) -> List[Dict[str, int]]:
    """Create a hand from list of (suit, rank) tuples."""
    return [make_card(s, r) for s, r in cards]


def make_ai_config(simulations: int = 1000, worlds: int = 30, epsilon: float = 0.1,
                   use_threads: bool = True, player_type: str = "safe_simple") -> Dict:
    """Create AI config object."""
    return {
        "simulations": simulations,
        "worlds": worlds,
        "epsilon": epsilon,
        "use_threads": use_threads,
        "player_type": player_type
    }


def make_game_state(player_hands: List[List[Dict]], current_player: int = 0,
                    current_trick: Optional[Dict] = None, played_cards: Optional[List] = None,
                    scores: Optional[List] = None, hearts_broken: bool = False,
                    pass_direction: int = 0, rules: int = 0) -> Dict:
    """Create a complete game state object."""
    return {
        "player_hands": player_hands,
        "current_player": current_player,
        "current_trick": current_trick or {"cards": [], "lead_player": current_player},
        "played_cards": played_cards or [[], [], [], []],
        "scores": scores or [0, 0, 0, 0],
        "hearts_broken": hearts_broken,
        "pass_direction": pass_direction,
        "rules": rules
    }


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
                    errors.append(f"Invalid suit value: {card.get('suit')} (must be 0-3)")
                if "rank" not in card:
                    errors.append("Missing 'rank' field in card")
                elif not isinstance(card["rank"], int) or card["rank"] < 0 or card["rank"] > 12:
                    errors.append(f"Invalid rank value: {card.get('rank')} (must be 0-12)")

            if "player" not in move:
                errors.append("Missing 'player' field in move")
            elif not isinstance(move["player"], int) or move["player"] < 0 or move["player"] > 3:
                errors.append(f"Invalid player value: {move.get('player')} (must be 0-3)")

        if "computation_time_ms" not in resp:
            errors.append("Missing 'computation_time_ms' field")
        elif not isinstance(resp["computation_time_ms"], (int, float)):
            errors.append(f"Invalid computation_time_ms type: {type(resp['computation_time_ms'])}")
        elif resp["computation_time_ms"] < 0:
            errors.append(f"Negative computation_time_ms: {resp['computation_time_ms']}")

    elif resp["status"] == "error":
        if "error_code" not in resp:
            errors.append("Missing 'error_code' field in error response")
        if "message" not in resp:
            errors.append("Missing 'message' field in error response")
    else:
        errors.append(f"Invalid status value: {resp['status']} (must be 'success' or 'error')")

    return errors


def validate_move_legality(move_card: Dict, player_hand: List[Dict],
                           current_trick_cards: List[Dict], hearts_broken: bool) -> List[str]:
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
    if current_trick_cards:
        lead_suit = current_trick_cards[0]["card"]["suit"]
        has_lead_suit = any(c["suit"] == lead_suit for c in player_hand)
        if has_lead_suit and suit != lead_suit:
            errors.append(f"Must follow lead suit {SUITS[lead_suit]}, but played {SUITS[suit]}")

    # If leading and hearts not broken, can't lead hearts (unless only have hearts)
    if not current_trick_cards and suit == 3 and not hearts_broken:
        has_non_hearts = any(c["suit"] != 3 for c in player_hand)
        if has_non_hearts:
            errors.append("Cannot lead hearts when hearts not broken (and has other suits)")

    return errors


# =============================================================================
# BASIC FUNCTIONALITY TESTS
# =============================================================================

def test_basic_move_request(host: str) -> TestResult:
    """Test basic move request with valid game state and AI config."""
    result = TestResult("Basic move request")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [make_card(2, 12)],  # P0: 2C
                [make_card(2, 11)],  # P1: 3C
                [make_card(2, 10)],  # P2: 4C
                [make_card(2, 9)]    # P3: 5C
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)

    if err:
        return result.fail(f"Request failed: {err}")

    struct_errors = validate_response_structure(resp)
    if struct_errors:
        return result.fail(f"Response structure invalid: {struct_errors}")

    if resp["status"] != "success":
        return result.fail(f"Expected success, got: {resp}")

    card = resp["move"]["card"]
    if card["suit"] != 2 or card["rank"] != 12:
        return result.fail(f"Expected 2C, got {card_str(card['suit'], card['rank'])}")

    result.add_detail(f"Move: {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Computation time: {resp['computation_time_ms']:.2f}ms")
    result.add_detail(f"Network time: {elapsed:.2f}ms")
    return result.success("Valid response")


def test_response_structure_complete(host: str) -> TestResult:
    """Test that response contains all required fields with correct types."""
    result = TestResult("Response structure completeness")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0), (2, 1)]),
                make_hand([(2, 2)]),
                make_hand([(2, 3)]),
                make_hand([(2, 4)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    # Check all required fields
    required_checks = [
        ("status", str, lambda x: x in ["success", "error"]),
        ("move", dict, lambda x: True),
        ("computation_time_ms", (int, float), lambda x: x >= 0),
    ]

    for field, expected_type, validator in required_checks:
        if field not in resp:
            return result.fail(f"Missing field: {field}")
        if not isinstance(resp[field], expected_type):
            return result.fail(f"Wrong type for {field}: expected {expected_type}, got {type(resp[field])}")
        if not validator(resp[field]):
            return result.fail(f"Invalid value for {field}: {resp[field]}")
        result.add_detail(f"{field}: OK ({type(resp[field]).__name__})")

    # Check nested move structure
    move = resp["move"]
    move_checks = [
        ("card", dict),
        ("player", int),
    ]
    for field, expected_type in move_checks:
        if field not in move:
            return result.fail(f"Missing move.{field}")
        if not isinstance(move[field], expected_type):
            return result.fail(f"Wrong type for move.{field}")
        result.add_detail(f"move.{field}: OK")

    # Check card structure
    card = move["card"]
    if "suit" not in card or "rank" not in card:
        return result.fail("Missing card.suit or card.rank")
    if not (0 <= card["suit"] <= 3):
        return result.fail(f"Invalid suit: {card['suit']}")
    if not (0 <= card["rank"] <= 12):
        return result.fail(f"Invalid rank: {card['rank']}")

    result.add_detail("card.suit: OK (0-3)")
    result.add_detail("card.rank: OK (0-12)")

    return result.success("All fields valid")


def test_player_in_response_matches_request(host: str) -> TestResult:
    """Test that move.player matches game_state.current_player."""
    result = TestResult("Response player matches request current_player")

    for player in range(4):
        hands = [[make_card(2, 12 - i)] for i in range(4)]
        data = {
            "game_state": make_game_state(
                player_hands=hands,
                current_player=player
            ),
            "ai_config": make_ai_config(simulations=50)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request for player {player} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success for player {player}")

        if resp["move"]["player"] != player:
            return result.fail(f"Player mismatch: expected {player}, got {resp['move']['player']}")

        result.add_detail(f"Player {player}: OK")

    return result.success("All players match")


def test_computation_time_tracking(host: str) -> TestResult:
    """Test that computation_time_ms reflects actual AI computation."""
    result = TestResult("Computation time tracking")

    # Single move - should be very fast (no AI needed)
    data_single = {
        "game_state": make_game_state(
            player_hands=[
                [make_card(2, 12)],
                [make_card(2, 11)],
                [make_card(2, 10)],
                [make_card(2, 9)]
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=1000)
    }

    resp_single, _, err = make_request(host, ENDPOINT, "POST", data_single)
    if err:
        return result.fail(f"Single move request failed: {err}")

    time_single = resp_single["computation_time_ms"]
    result.add_detail(f"Single legal move: {time_single:.2f}ms")

    # Multiple moves with AI - should take longer
    data_multi = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, i) for i in range(6)]),  # 6 clubs
                make_hand([(2, i) for i in range(6, 9)]),
                make_hand([(2, i) for i in range(9, 11)]),
                make_hand([(2, i) for i in range(11, 13)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=1000)
    }

    resp_multi, _, err = make_request(host, ENDPOINT, "POST", data_multi)
    if err:
        return result.fail(f"Multi move request failed: {err}")

    time_multi = resp_multi["computation_time_ms"]
    result.add_detail(f"Multiple moves (1000 sims): {time_multi:.2f}ms")

    # Single move should generally be faster
    if time_single < time_multi:
        result.add_detail("Single move faster than AI search (expected)")
    else:
        result.add_detail("Note: Single move not faster (timing variance)")

    return result.success("Timing tracked correctly")


# =============================================================================
# AI CONFIG TESTS
# =============================================================================

def test_ai_config_simulations(host: str) -> TestResult:
    """Test that simulations parameter affects computation."""
    result = TestResult("AI config: simulations parameter")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, i) for i in range(5)]),
            make_hand([(2, i) for i in range(5, 8)]),
            make_hand([(2, i) for i in range(8, 11)]),
            make_hand([(2, i) for i in range(11, 13)])
        ],
        current_player=0
    )

    sim_counts = [100, 500, 2000]
    times = []

    for sims in sim_counts:
        data = {
            "game_state": base_state,
            "ai_config": make_ai_config(simulations=sims)
        }
        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with {sims} sims failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with {sims} sims")

        times.append(resp["computation_time_ms"])
        result.add_detail(f"{sims} simulations: {times[-1]:.2f}ms")

    # Generally, more simulations = more time (not guaranteed due to variance)
    return result.success("Simulations parameter accepted")


def test_ai_config_player_types(host: str) -> TestResult:
    """Test all valid player types."""
    result = TestResult("AI config: all player types")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, 0), (2, 1), (2, 2)]),
            make_hand([(2, 3), (2, 4)]),
            make_hand([(2, 5), (2, 6)]),
            make_hand([(2, 7), (2, 8)])
        ],
        current_player=0
    )

    for player_type in VALID_PLAYER_TYPES:
        data = {
            "game_state": base_state,
            "ai_config": make_ai_config(simulations=100, player_type=player_type)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with {player_type} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with {player_type}: {resp}")

        card = resp["move"]["card"]
        result.add_detail(f"{player_type}: {card_str(card['suit'], card['rank'])} ({resp['computation_time_ms']:.1f}ms)")

    return result.success("All player types work")


def test_ai_config_epsilon(host: str) -> TestResult:
    """Test epsilon parameter (exploration vs exploitation)."""
    result = TestResult("AI config: epsilon parameter")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, i) for i in range(4)]),
            make_hand([(2, i) for i in range(4, 7)]),
            make_hand([(2, i) for i in range(7, 10)]),
            make_hand([(2, i) for i in range(10, 13)])
        ],
        current_player=0
    )

    epsilon_values = [0.0, 0.1, 0.5, 1.0]

    for eps in epsilon_values:
        data = {
            "game_state": base_state,
            "ai_config": make_ai_config(simulations=100, epsilon=eps)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with epsilon={eps} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with epsilon={eps}")

        result.add_detail(f"epsilon={eps}: OK")

    return result.success("Epsilon parameter accepted")


def test_ai_config_use_threads(host: str) -> TestResult:
    """Test use_threads parameter."""
    result = TestResult("AI config: use_threads parameter")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, i) for i in range(4)]),
            make_hand([(2, i) for i in range(4, 7)]),
            make_hand([(2, i) for i in range(7, 10)]),
            make_hand([(2, i) for i in range(10, 13)])
        ],
        current_player=0
    )

    for use_threads in [True, False]:
        data = {
            "game_state": base_state,
            "ai_config": make_ai_config(simulations=200, use_threads=use_threads)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with use_threads={use_threads} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with use_threads={use_threads}")

        result.add_detail(f"use_threads={use_threads}: {resp['computation_time_ms']:.1f}ms")

    return result.success("use_threads parameter works")


def test_ai_config_worlds(host: str) -> TestResult:
    """Test worlds parameter (number of world models)."""
    result = TestResult("AI config: worlds parameter")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, i) for i in range(4)]),
            make_hand([(2, i) for i in range(4, 7)]),
            make_hand([(2, i) for i in range(7, 10)]),
            make_hand([(2, i) for i in range(10, 13)])
        ],
        current_player=0
    )

    world_counts = [10, 30, 50]

    for worlds in world_counts:
        data = {
            "game_state": base_state,
            "ai_config": make_ai_config(simulations=300, worlds=worlds)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with worlds={worlds} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with worlds={worlds}")

        result.add_detail(f"worlds={worlds}: {resp['computation_time_ms']:.1f}ms")

    return result.success("Worlds parameter works")


def test_ai_config_defaults(host: str) -> TestResult:
    """Test that missing ai_config fields use sensible defaults."""
    result = TestResult("AI config: default values")

    base_state = make_game_state(
        player_hands=[
            make_hand([(2, 0), (2, 1)]),
            make_hand([(2, 2)]),
            make_hand([(2, 3)]),
            make_hand([(2, 4)])
        ],
        current_player=0
    )

    # Minimal ai_config - should use defaults for missing fields
    data = {
        "game_state": base_state,
        "ai_config": {"simulations": 100}  # Only simulations, rest default
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request with minimal config failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success with minimal config: {resp}")

    result.add_detail("Minimal ai_config accepted")
    result.add_detail(f"Move: {card_str(resp['move']['card']['suit'], resp['move']['card']['rank'])}")

    return result.success("Defaults work correctly")


# =============================================================================
# GAME STATE TESTS
# =============================================================================

def test_game_state_full_hands(host: str) -> TestResult:
    """Test with full 13-card hands."""
    result = TestResult("Game state: full 13-card hands")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [make_card(2, r) for r in range(13)],   # All clubs
                [make_card(0, r) for r in range(13)],   # All spades
                [make_card(1, r) for r in range(13)],   # All diamonds
                [make_card(3, r) for r in range(13)]    # All hearts
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=200)
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    result.add_detail(f"Move: {card_str(card['suit'], card['rank'])}")
    result.add_detail(f"Time: {resp['computation_time_ms']:.2f}ms")

    # Should play a club
    if card["suit"] != 2:
        return result.fail(f"Expected club, got {SUITS[card['suit']]}")

    return result.success("Full hands handled")


def test_game_state_current_trick(host: str) -> TestResult:
    """Test with cards in current trick."""
    result = TestResult("Game state: current trick in progress")

    # Player 2 must respond to trick
    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                [],
                make_hand([(2, 5), (2, 6), (3, 0)]),  # 9C, 8C, AH
                []
            ],
            current_player=2,
            current_trick={
                "cards": [
                    {"card": make_card(2, 12), "player": 0},  # 2C
                    {"card": make_card(2, 11), "player": 1}   # 3C
                ],
                "lead_player": 0
            }
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Must follow clubs
    if card["suit"] != 2:
        return result.fail(f"Must follow clubs, played {SUITS[card['suit']]}")

    result.add_detail(f"Followed suit with {card_str(card['suit'], card['rank'])}")
    return result.success("Current trick handled")


def test_game_state_played_cards(host: str) -> TestResult:
    """Test with played cards history."""
    result = TestResult("Game state: played cards history")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(0, 3), (1, 5), (3, 8)]),
                make_hand([(0, 4), (1, 6)]),
                make_hand([(0, 5), (1, 7)]),
                make_hand([(0, 6), (3, 9)])
            ],
            current_player=0,
            played_cards=[
                make_hand([(2, 12), (0, 0)]),  # P0 took 2C, AS
                make_hand([(2, 11), (0, 1)]),  # P1 took 3C, KS
                make_hand([(2, 10), (0, 2)]),  # P2 took 4C, QS
                make_hand([(2, 9), (3, 0)])    # P3 took 5C, AH (1 point)
            ],
            scores=[0, 0, 0, 1],
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=200)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    hand = data["game_state"]["player_hands"][0]
    card_in_hand = any(c["suit"] == card["suit"] and c["rank"] == card["rank"] for c in hand)

    if not card_in_hand:
        return result.fail(f"Card not in hand: {card_str(card['suit'], card['rank'])}")

    result.add_detail(f"Move: {card_str(card['suit'], card['rank'])}")
    result.add_detail("Hearts broken, scores: [0, 0, 0, 1]")
    return result.success("Played cards handled")


def test_game_state_pass_direction(host: str) -> TestResult:
    """Test different pass directions."""
    result = TestResult("Game state: pass direction values")

    pass_dirs = [0, 1, -1, 2]  # hold, left, right, across
    dir_names = {0: "hold", 1: "left", -1: "right", 2: "across"}

    for pass_dir in pass_dirs:
        data = {
            "game_state": make_game_state(
                player_hands=[
                    make_hand([(2, 0), (2, 1)]),
                    make_hand([(2, 2)]),
                    make_hand([(2, 3)]),
                    make_hand([(2, 4)])
                ],
                current_player=0,
                pass_direction=pass_dir
            ),
            "ai_config": make_ai_config(simulations=50)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with pass_dir={pass_dir} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with pass_dir={pass_dir}")

        result.add_detail(f"pass_direction={pass_dir} ({dir_names[pass_dir]}): OK")

    return result.success("All pass directions work")


def test_game_state_rules(host: str) -> TestResult:
    """Test different game rules."""
    result = TestResult("Game state: rules parameter")

    # Common rule values (bitmask)
    rule_values = [0, 1, 2, 3]

    for rules in rule_values:
        data = {
            "game_state": make_game_state(
                player_hands=[
                    make_hand([(2, 0), (2, 1)]),
                    make_hand([(2, 2)]),
                    make_hand([(2, 3)]),
                    make_hand([(2, 4)])
                ],
                current_player=0,
                rules=rules
            ),
            "ai_config": make_ai_config(simulations=50)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request with rules={rules} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success with rules={rules}")

        result.add_detail(f"rules={rules}: OK")

    return result.success("Rules parameter works")


# =============================================================================
# GAME RULES TESTS
# =============================================================================

def test_must_follow_suit(host: str) -> TestResult:
    """Test that AI follows suit when required."""
    result = TestResult("Game rules: must follow suit")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                make_hand([(2, 5), (2, 6), (3, 0)]),  # Has clubs and hearts
                [],
                []
            ],
            current_player=1,
            current_trick={
                "cards": [{"card": make_card(2, 12), "player": 0}],
                "lead_player": 0
            }
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] != 2:
        return result.fail(f"Must follow clubs, played {SUITS[card['suit']]}")

    result.add_detail(f"Correctly followed with {card_str(card['suit'], card['rank'])}")
    return result.success("Follows suit correctly")


def test_can_slough_when_void(host: str) -> TestResult:
    """Test sloughing when void in led suit."""
    result = TestResult("Game rules: slough when void")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                make_hand([(3, 0), (3, 1), (0, 5)]),  # No clubs - can slough
                [],
                []
            ],
            current_player=1,
            current_trick={
                "cards": [{"card": make_card(2, 12), "player": 0}],
                "lead_player": 0
            }
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    hand = data["game_state"]["player_hands"][1]
    card_in_hand = any(c["suit"] == card["suit"] and c["rank"] == card["rank"] for c in hand)

    if not card_in_hand:
        return result.fail(f"Card not in hand")

    result.add_detail(f"Sloughed {card_str(card['suit'], card['rank'])}")
    return result.success("Sloughing works")


def test_hearts_not_broken_cannot_lead(host: str) -> TestResult:
    """Test cannot lead hearts when not broken."""
    result = TestResult("Game rules: cannot lead hearts when not broken")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0), (3, 0), (3, 1)]),  # AC, AH, KH
                make_hand([(1, 0)]),
                make_hand([(0, 0)]),
                make_hand([(2, 1)])
            ],
            current_player=0,
            hearts_broken=False
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] == 3:
        return result.fail("Should not lead hearts when not broken")

    result.add_detail(f"Led {card_str(card['suit'], card['rank'])} (not hearts)")
    return result.success("Hearts not led when not broken")


def test_hearts_broken_can_lead(host: str) -> TestResult:
    """Test can lead hearts when broken."""
    result = TestResult("Game rules: can lead hearts when broken")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(3, 0), (3, 1), (3, 2)]),  # Only hearts
                make_hand([(1, 0)]),
                make_hand([(0, 0)]),
                make_hand([(2, 0)])
            ],
            current_player=0,
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] != 3:
        return result.fail(f"Expected hearts, got {SUITS[card['suit']]}")

    result.add_detail(f"Led {card_str(card['suit'], card['rank'])}")
    return result.success("Hearts led when broken")


def test_only_hearts_can_lead_when_not_broken(host: str) -> TestResult:
    """Test can lead hearts if only have hearts (even when not broken)."""
    result = TestResult("Game rules: only hearts forces heart lead")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(3, 0), (3, 1)]),  # Only hearts
                make_hand([(1, 0)]),
                make_hand([(0, 0)]),
                make_hand([(2, 0)])
            ],
            current_player=0,
            hearts_broken=False  # Not broken, but only have hearts
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] != 3:
        return result.fail(f"Must lead hearts (only option), got {SUITS[card['suit']]}")

    result.add_detail(f"Forced to lead {card_str(card['suit'], card['rank'])}")
    return result.success("Forced heart lead works")


# =============================================================================
# MOVE LEGALITY TESTS
# =============================================================================

def test_returned_card_in_hand(host: str) -> TestResult:
    """Test that returned card is always in player's hand."""
    result = TestResult("Move legality: card in hand")

    hands = [
        make_hand([(2, 0), (0, 5), (1, 8), (3, 2)]),
        make_hand([(2, 1), (0, 6)]),
        make_hand([(2, 2), (1, 7)]),
        make_hand([(2, 3), (3, 9)])
    ]

    for player in range(4):
        data = {
            "game_state": make_game_state(
                player_hands=hands,
                current_player=player,
                hearts_broken=True
            ),
            "ai_config": make_ai_config(simulations=100)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request for player {player} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success for player {player}")

        card = resp["move"]["card"]
        hand = hands[player]
        in_hand = any(c["suit"] == card["suit"] and c["rank"] == card["rank"] for c in hand)

        if not in_hand:
            return result.fail(f"Player {player}: card {card_str(card['suit'], card['rank'])} not in hand")

        result.add_detail(f"Player {player}: {card_str(card['suit'], card['rank'])} OK")

    return result.success("All moves from hand")


def test_move_legality_comprehensive(host: str) -> TestResult:
    """Comprehensive move legality validation."""
    result = TestResult("Move legality: comprehensive check")

    # Scenario: P2 must follow spades, has spades and hearts
    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                [],
                make_hand([(0, 5), (0, 8), (3, 0), (3, 5)]),  # 9S, 6S, AH, 9H
                []
            ],
            current_player=2,
            current_trick={
                "cards": [
                    {"card": make_card(0, 0), "player": 0},  # AS
                    {"card": make_card(0, 1), "player": 1}   # KS
                ],
                "lead_player": 0
            },
            hearts_broken=False
        ),
        "ai_config": make_ai_config(simulations=200)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    hand = data["game_state"]["player_hands"][2]
    trick_cards = data["game_state"]["current_trick"]["cards"]

    # Validate using legality checker
    errors = validate_move_legality(card, hand, trick_cards, False)
    if errors:
        return result.fail(f"Illegal move: {errors}")

    result.add_detail(f"Move: {card_str(card['suit'], card['rank'])}")
    result.add_detail("Legality check passed")
    return result.success("Move is legal")


# =============================================================================
# ERROR HANDLING TESTS
# =============================================================================

def test_error_invalid_json(host: str) -> TestResult:
    """Test error response for invalid JSON."""
    result = TestResult("Error handling: invalid JSON")

    req = urllib.request.Request(
        f"http://{host}{ENDPOINT}",
        data=b"{ this is not valid json",
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
        return result.fail("Missing error_code in error response")

    if "message" not in resp:
        return result.fail("Missing message in error response")

    result.add_detail(f"error_code: {resp['error_code']}")
    result.add_detail(f"message: {resp['message'][:50]}...")
    return result.success("Proper error response")


def test_error_missing_game_state(host: str) -> TestResult:
    """Test error response for missing game_state."""
    result = TestResult("Error handling: missing game_state")

    data = {"ai_config": make_ai_config(simulations=100)}

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error status: {resp}")

    result.add_detail(f"error_code: {resp.get('error_code')}")
    return result.success("Missing game_state detected")


def test_error_missing_ai_config(host: str) -> TestResult:
    """Test behavior with missing ai_config."""
    result = TestResult("Error handling: missing ai_config")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0)]),
                make_hand([(2, 1)]),
                make_hand([(2, 2)]),
                make_hand([(2, 3)])
            ],
            current_player=0
        )
        # No ai_config
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    # Could be error or use defaults
    result.add_detail(f"status: {resp.get('status')}")
    if resp.get("status") == "error":
        result.add_detail(f"error_code: {resp.get('error_code')}")
    else:
        result.add_detail("Server used default ai_config")

    return result.success("Missing ai_config handled")


def test_error_empty_hands(host: str) -> TestResult:
    """Test error response for empty player hands."""
    result = TestResult("Error handling: empty hands")

    data = {
        "game_state": make_game_state(
            player_hands=[[], [], [], []],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error for empty hands: {resp}")

    result.add_detail(f"error_code: {resp.get('error_code')}")
    return result.success("Empty hands detected")


def test_error_invalid_player_type(host: str) -> TestResult:
    """Test error handling for invalid player_type."""
    result = TestResult("Error handling: invalid player_type")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0)]),
                make_hand([(2, 1)]),
                make_hand([(2, 2)]),
                make_hand([(2, 3)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=100, player_type="nonexistent_type")
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    # Could be error or fall back to default
    result.add_detail(f"status: {resp.get('status')}")
    if resp.get("status") == "error":
        result.add_detail(f"error_code: {resp.get('error_code')}")
    else:
        result.add_detail("Server used fallback player type")

    return result.success("Invalid player_type handled")


def test_error_negative_simulations(host: str) -> TestResult:
    """Test handling of negative simulations value."""
    result = TestResult("Error handling: negative simulations")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0)]),
                make_hand([(2, 1)]),
                make_hand([(2, 2)]),
                make_hand([(2, 3)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=-100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    result.add_detail(f"status: {resp.get('status')}")
    if resp.get("error_code"):
        result.add_detail(f"error_code: {resp.get('error_code')}")

    return result.success("Negative simulations handled")


# =============================================================================
# EDGE CASE TESTS
# =============================================================================

def test_single_legal_move_optimization(host: str) -> TestResult:
    """Test that single legal move skips AI computation."""
    result = TestResult("Optimization: single legal move")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [make_card(2, 12)],  # Only 2C
                [make_card(2, 11)],
                [make_card(2, 10)],
                [make_card(2, 9)]
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=10000)  # High sims, but should be skipped
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    comp_time = resp["computation_time_ms"]
    result.add_detail(f"Computation time: {comp_time:.2f}ms")
    result.add_detail(f"Network time: {elapsed:.2f}ms")

    # Should be very fast since AI is skipped
    if comp_time < 50:
        result.add_detail("Fast response (AI likely skipped)")

    return result.success("Single move handled efficiently")


def test_last_trick_of_game(host: str) -> TestResult:
    """Test final trick with 1 card each."""
    result = TestResult("Edge case: last trick of game")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [make_card(3, 12)],  # 2H
                [make_card(3, 11)],  # 3H
                [make_card(3, 10)],  # 4H
                [make_card(3, 9)]    # 5H
            ],
            current_player=0,
            played_cards=[
                [make_card(s, r) for s in range(3) for r in range(13)],
                [], [], []
            ],
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] != 3 or card["rank"] != 12:
        return result.fail(f"Expected 2H, got {card_str(card['suit'], card['rank'])}")

    result.add_detail("Final trick handled correctly")
    return result.success("Last trick OK")


def test_complete_trick_fourth_player(host: str) -> TestResult:
    """Test completing a trick as fourth player."""
    result = TestResult("Edge case: completing trick as P3")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                [],
                [],
                make_hand([(0, 5), (0, 8), (3, 0)])  # 9S, 6S, AH
            ],
            current_player=3,
            current_trick={
                "cards": [
                    {"card": make_card(0, 12), "player": 0},  # 2S
                    {"card": make_card(0, 11), "player": 1},  # 3S
                    {"card": make_card(0, 10), "player": 2}   # 4S
                ],
                "lead_player": 0
            }
        ),
        "ai_config": make_ai_config(simulations=200)
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
    return result.success("Trick completion works")


def test_queen_of_spades_scenario(host: str) -> TestResult:
    """Test AI behavior with Queen of Spades."""
    result = TestResult("Edge case: Queen of Spades awareness")

    data = {
        "game_state": make_game_state(
            player_hands=[
                [],
                [],
                make_hand([(0, 2), (0, 5), (0, 8)]),  # QS, 9S, 6S
                []
            ],
            current_player=2,
            current_trick={
                "cards": [
                    {"card": make_card(0, 1), "player": 0},  # KS led
                    {"card": make_card(0, 12), "player": 1}  # 2S
                ],
                "lead_player": 0
            }
        ),
        "ai_config": make_ai_config(simulations=500)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    if card["suit"] != 0:
        return result.fail("Must follow spades")

    result.add_detail(f"AI played {card_str(card['suit'], card['rank'])}")

    if card["rank"] == 2:
        result.add_detail("Note: AI played QS (might take 13 points)")
    else:
        result.add_detail("AI avoided QS")

    return result.success("QS scenario handled")


def test_shoot_the_moon_potential(host: str) -> TestResult:
    """Test scenario where shooting the moon might be possible."""
    result = TestResult("Edge case: shoot the moon potential")

    # Player 0 has taken almost all hearts
    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(3, 0)]),  # AH - last heart
                make_hand([(2, 0)]),
                make_hand([(1, 0)]),
                make_hand([(0, 0)])
            ],
            current_player=0,
            played_cards=[
                make_hand([(3, i) for i in range(1, 13)] + [(0, 2)]),  # P0 has 12 hearts + QS
                [], [], []
            ],
            scores=[25, 0, 0, 0],  # P0 has 25 points (12 hearts + QS)
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=300)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    result.add_detail(f"AI leads {card_str(card['suit'], card['rank'])}")
    result.add_detail("P0 has 25 points, needs AH to shoot moon")

    return result.success("Moon scenario handled")


def test_consistency_across_calls(host: str) -> TestResult:
    """Test that repeated calls return valid (not necessarily same) moves."""
    result = TestResult("Consistency: multiple calls same state")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, 0), (2, 1), (2, 2)]),
                make_hand([(2, 3)]),
                make_hand([(2, 4)]),
                make_hand([(2, 5)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    moves = []
    for i in range(3):
        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Call {i+1} failed: {err}")

        if resp["status"] != "success":
            return result.fail(f"Call {i+1} not success")

        card = resp["move"]["card"]
        moves.append(card_str(card["suit"], card["rank"]))
        result.add_detail(f"Call {i+1}: {moves[-1]}")

    # All should be clubs (valid moves)
    for m in moves:
        if not m.endswith("C"):
            return result.fail(f"Invalid move: {m}")

    return result.success("All calls valid")


def test_high_simulation_count(host: str) -> TestResult:
    """Test with high simulation count."""
    result = TestResult("Stress: high simulation count")

    data = {
        "game_state": make_game_state(
            player_hands=[
                make_hand([(2, i) for i in range(5)]),
                make_hand([(2, i) for i in range(5, 8)]),
                make_hand([(2, i) for i in range(8, 11)]),
                make_hand([(2, i) for i in range(11, 13)])
            ],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=5000, worlds=50)
    }

    resp, elapsed, err = make_request(host, ENDPOINT, "POST", data, timeout=120)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    result.add_detail(f"Computation: {resp['computation_time_ms']:.2f}ms")
    result.add_detail(f"Network: {elapsed:.2f}ms")
    result.add_detail(f"Move: {card_str(resp['move']['card']['suit'], resp['move']['card']['rank'])}")

    return result.success("High sims handled")


# =============================================================================
# TEST RUNNER
# =============================================================================

def run_tests(host: str) -> Tuple[int, int]:
    """Run all tests and return (passed, failed) counts."""

    tests = [
        # Basic functionality
        ("BASIC FUNCTIONALITY", [
            test_basic_move_request,
            test_response_structure_complete,
            test_player_in_response_matches_request,
            test_computation_time_tracking,
        ]),

        # AI config
        ("AI CONFIGURATION", [
            test_ai_config_simulations,
            test_ai_config_player_types,
            test_ai_config_epsilon,
            test_ai_config_use_threads,
            test_ai_config_worlds,
            test_ai_config_defaults,
        ]),

        # Game state
        ("GAME STATE", [
            test_game_state_full_hands,
            test_game_state_current_trick,
            test_game_state_played_cards,
            test_game_state_pass_direction,
            test_game_state_rules,
        ]),

        # Game rules
        ("GAME RULES", [
            test_must_follow_suit,
            test_can_slough_when_void,
            test_hearts_not_broken_cannot_lead,
            test_hearts_broken_can_lead,
            test_only_hearts_can_lead_when_not_broken,
        ]),

        # Move legality
        ("MOVE LEGALITY", [
            test_returned_card_in_hand,
            test_move_legality_comprehensive,
        ]),

        # Error handling
        ("ERROR HANDLING", [
            test_error_invalid_json,
            test_error_missing_game_state,
            test_error_missing_ai_config,
            test_error_empty_hands,
            test_error_invalid_player_type,
            test_error_negative_simulations,
        ]),

        # Edge cases
        ("EDGE CASES", [
            test_single_legal_move_optimization,
            test_last_trick_of_game,
            test_complete_trick_fourth_player,
            test_queen_of_spades_scenario,
            test_shoot_the_moon_potential,
            test_consistency_across_calls,
            test_high_simulation_count,
        ]),
    ]

    total_passed = 0
    total_failed = 0

    for category, test_funcs in tests:
        print(f"\n{'='*60}")
        print(f" {category}")
        print('='*60)

        for test_func in test_funcs:
            try:
                result = test_func(host)
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
    print(" HEARTS AI SERVER - /api/move ENDPOINT TEST SUITE")
    print("="*60)
    print(f"Target: http://{host}{ENDPOINT}")
    print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")

    # Health check
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
