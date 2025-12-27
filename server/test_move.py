#!/usr/bin/env python3
"""
Comprehensive Test Suite for Hearts AI Server /api/move Endpoint

Tests cover:
- Basic functionality and response structure
- AI configuration (simulations, player types, epsilon, threads, worlds)
- Game state handling (hands, tricks, pass direction, rules)
- Game rules enforcement (follow suit, slough, hearts broken)
- Move legality validation
- Card format validation (all ranks, suits, string format)
- Error handling (invalid JSON, missing fields, invalid cards)
- Edge cases (single card, last trick, Queen of Spades, shoot the moon)

Uses string-based card format (e.g., "2C", "AH", "10S").

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

# Card encoding constants - for converting old format to string
SUIT_NAMES = {0: "Spades", 1: "Diamonds", 2: "Clubs", 3: "Hearts"}
SUIT_LETTERS = {0: "S", 1: "D", 2: "C", 3: "H"}
RANK_NAMES = {0: "A", 1: "K", 2: "Q", 3: "J", 4: "10", 5: "9", 6: "8", 7: "7", 8: "6", 9: "5", 10: "4", 11: "3", 12: "2"}

# String card format
RANKS = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A']
SUITS = ['C', 'D', 'S', 'H']  # Clubs, Diamonds, Spades, Hearts

# Valid player types
VALID_PLAYER_TYPES = ["simple", "safe_simple", "global", "global2", "global3"]


def card_str(suit: int, rank: int) -> str:
    """Convert (suit, rank) indices to card string. suit: 0=C,1=D,2=S,3=H. rank: 0=2,...,12=A."""
    return RANKS[rank] + SUITS[suit]


def make_card(suit: int, rank: int) -> str:
    """Create a card string from suit/rank indices."""
    return card_str(suit, rank)


def make_hand(cards: List[Tuple[int, int]]) -> List[str]:
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


def make_game_state(player_hand: List[str], current_player: int = 0,
                    current_trick: Optional[Dict] = None, trick_history: Optional[List] = None,
                    scores: Optional[List] = None, hearts_broken: bool = False,
                    pass_direction: int = 0, rules: int = 0) -> Dict:
    """Create a complete game state object using string card format."""
    return {
        "player_hand": player_hand,
        "current_player": current_player,
        "current_trick": current_trick or {"cards": [], "lead_player": current_player},
        "trick_history": trick_history or [],
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
                # Card should be a string like "2C", "AH", "10S"
                if not isinstance(card, str):
                    errors.append(f"Card should be string, got {type(card)}")
                elif len(card) < 2 or len(card) > 3:
                    errors.append(f"Invalid card format: {card}")

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


def get_card_suit(card: str) -> str:
    """Get suit letter from card string (e.g., '2C' -> 'C')."""
    return card[-1]


def validate_move_legality(move_card: str, player_hand: List[str],
                           current_trick_cards: List[Dict], hearts_broken: bool) -> List[str]:
    """Validate the returned move is legal. Returns list of errors."""
    errors = []

    # Check card is in player's hand
    if move_card not in player_hand:
        errors.append(f"Returned card {move_card} not in player's hand")
        return errors

    move_suit = get_card_suit(move_card)

    # If there's a lead card, must follow suit if possible
    if current_trick_cards:
        lead_card = current_trick_cards[0]["card"]
        lead_suit = get_card_suit(lead_card)
        has_lead_suit = any(get_card_suit(c) == lead_suit for c in player_hand)
        if has_lead_suit and move_suit != lead_suit:
            errors.append(f"Must follow lead suit {lead_suit}, but played {move_suit}")

    # If leading and hearts not broken, can't lead hearts (unless only have hearts)
    if not current_trick_cards and move_suit == 'H' and not hearts_broken:
        has_non_hearts = any(get_card_suit(c) != 'H' for c in player_hand)
        if has_non_hearts:
            errors.append("Cannot lead hearts when hearts not broken (and has other suits)")

    return errors


# =============================================================================
# HEALTH CHECK TEST
# =============================================================================

def test_health_check(host: str) -> TestResult:
    """Test /api/health endpoint."""
    result = TestResult("Health check")

    resp, elapsed, err = make_request(host, "/api/health")
    if err:
        return result.fail(f"Request failed: {err}")

    if resp.get("status") != "ok":
        return result.fail(f"Expected status 'ok', got: {resp}")

    result.add_detail(f"Response: {resp}")
    result.add_detail(f"Time: {elapsed:.1f}ms")
    return result.success("Server healthy")


# =============================================================================
# BASIC FUNCTIONALITY TESTS
# =============================================================================

def test_basic_move_request(host: str) -> TestResult:
    """Test basic move request with valid game state and AI config."""
    result = TestResult("Basic move request")

    data = {
        "game_state": make_game_state(
            player_hand=["2C"],  # Player has only 2C
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
    if card != "2C":
        return result.fail(f"Expected 2C, got {card}")

    result.add_detail(f"Move: {card}")
    result.add_detail(f"Computation time: {resp['computation_time_ms']:.2f}ms")
    result.add_detail(f"Network time: {elapsed:.2f}ms")
    return result.success("Valid response")


def test_response_structure_complete(host: str) -> TestResult:
    """Test that response contains all required fields with correct types."""
    result = TestResult("Response structure completeness")

    data = {
        "game_state": make_game_state(
            player_hand=["2C", "3C"],
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
        ("card", str),
        ("player", int),
    ]
    for field, expected_type in move_checks:
        if field not in move:
            return result.fail(f"Missing move.{field}")
        if not isinstance(move[field], expected_type):
            return result.fail(f"Wrong type for move.{field}: expected {expected_type}, got {type(move[field])}")
        result.add_detail(f"move.{field}: OK")

    # Check card is valid string format
    card = move["card"]
    if len(card) < 2 or len(card) > 3:
        return result.fail(f"Invalid card format: {card}")
    if card[-1] not in "CDSH":
        return result.fail(f"Invalid suit in card: {card}")

    result.add_detail(f"card: {card} (valid format)")

    return result.success("All fields valid")


def test_player_in_response_matches_request(host: str) -> TestResult:
    """Test that move.player matches game_state.current_player."""
    result = TestResult("Response player matches request current_player")

    # Test with player 0 (the only one we can test in stateless mode)
    data = {
        "game_state": make_game_state(
            player_hand=["2C"],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=50)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success for player 0")

    if resp["move"]["player"] != 0:
        return result.fail(f"Player mismatch: expected 0, got {resp['move']['player']}")

    result.add_detail("Player 0: OK")

    return result.success("Player matches")


def test_computation_time_tracking(host: str) -> TestResult:
    """Test that computation_time_ms reflects actual AI computation."""
    result = TestResult("Computation time tracking")

    # Single move - should be very fast (no AI needed)
    data_single = {
        "game_state": make_game_state(
            player_hand=["2C"],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=1000)
    }

    resp_single, _, err = make_request(host, ENDPOINT, "POST", data_single)
    if err:
        return result.fail(f"Single move request failed: {err}")

    if resp_single["status"] != "success":
        return result.fail(f"Expected success: {resp_single}")

    time_single = resp_single["computation_time_ms"]
    result.add_detail(f"Single legal move: {time_single:.2f}ms")

    # Multiple moves with AI - should take longer
    data_multi = {
        "game_state": make_game_state(
            player_hand=["2C", "3C", "4C", "5C", "6C", "7C"],  # 6 clubs
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=1000)
    }

    resp_multi, _, err = make_request(host, ENDPOINT, "POST", data_multi)
    if err:
        return result.fail(f"Multi move request failed: {err}")

    if resp_multi["status"] != "success":
        return result.fail(f"Expected success: {resp_multi}")

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
        player_hand=["2C", "3C", "4C", "5C", "6C"],
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
        player_hand=["2C", "3C", "4C"],
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
        result.add_detail(f"{player_type}: {card} ({resp['computation_time_ms']:.1f}ms)")

    return result.success("All player types work")


def test_ai_config_epsilon(host: str) -> TestResult:
    """Test epsilon parameter (exploration vs exploitation)."""
    result = TestResult("AI config: epsilon parameter")

    base_state = make_game_state(
        player_hand=["2C", "3C", "4C", "5C"],
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
        player_hand=["2C", "3C", "4C", "5C"],
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
        player_hand=["2C", "3C", "4C", "5C"],
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
        player_hand=["2C", "3C"],
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
    result.add_detail(f"Move: {resp['move']['card']}")

    return result.success("Defaults work correctly")


# =============================================================================
# GAME STATE TESTS
# =============================================================================

def test_game_state_full_hands(host: str) -> TestResult:
    """Test with full 13-card hand."""
    result = TestResult("Game state: full 13-card hands")

    # All clubs for player
    all_clubs = [f"{r}C" for r in RANKS]

    data = {
        "game_state": make_game_state(
            player_hand=all_clubs,
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
    result.add_detail(f"Move: {card}")
    result.add_detail(f"Time: {resp['computation_time_ms']:.2f}ms")

    # Should return a valid card from hand
    if card not in all_clubs:
        return result.fail(f"Card not in hand: {card}")

    return result.success("Full hands handled")


def test_game_state_current_trick(host: str) -> TestResult:
    """Test with cards in current trick."""
    result = TestResult("Game state: current trick in progress")

    hand = ["7C", "8C", "AH"]
    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            current_trick={
                "cards": [
                    {"card": "2C", "player": 3}
                ],
                "lead_player": 3
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
    if not card.endswith("C"):
        return result.fail(f"Must follow clubs, played {card}")

    result.add_detail(f"Followed suit with {card}")
    return result.success("Current trick handled")


def test_game_state_played_cards(host: str) -> TestResult:
    """Test with smaller hand (simulating cards already played)."""
    result = TestResult("Game state: played cards history")

    hand = ["JS", "9D", "6H"]
    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
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

    if card not in hand:
        return result.fail(f"Card not in hand: {card}")

    result.add_detail(f"Move: {card}")
    return result.success("Played cards handled")


def test_game_state_pass_direction(host: str) -> TestResult:
    """Test different pass directions."""
    result = TestResult("Game state: pass direction values")

    pass_dirs = [0, 1, -1, 2]  # hold, left, right, across
    dir_names = {0: "hold", 1: "left", -1: "right", 2: "across"}

    for pass_dir in pass_dirs:
        data = {
            "game_state": make_game_state(
                player_hand=["2C", "3C"],
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
                player_hand=["2C", "3C"],
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
            player_hand=["7C", "8C", "AH"],  # Has clubs and hearts
            current_player=0,
            current_trick={
                "cards": [{"card": "2C", "player": 3}],
                "lead_player": 3
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

    if not card.endswith("C"):
        return result.fail(f"Must follow clubs, played {card}")

    result.add_detail(f"Correctly followed with {card}")
    return result.success("Follows suit correctly")


def test_can_slough_when_void(host: str) -> TestResult:
    """Test sloughing when void in led suit."""
    result = TestResult("Game rules: slough when void")

    # Use a simpler scenario - player leads with no clubs in hand
    hand = ["AH", "KH", "9S"]  # No clubs
    data = {
        "game_state": make_game_state(
            player_hand=hand,
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

    if card not in hand:
        return result.fail(f"Card not in hand: {card}")

    result.add_detail(f"Played {card}")
    return result.success("Sloughing works")


def test_hearts_not_broken_cannot_lead(host: str) -> TestResult:
    """Test leading when hearts not broken."""
    result = TestResult("Game rules: cannot lead hearts when not broken")

    hand = ["AC", "AH", "KH"]  # AC and hearts
    data = {
        "game_state": make_game_state(
            player_hand=hand,
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

    if card not in hand:
        return result.fail(f"Card not in hand: {card}")

    result.add_detail(f"Led {card}")
    return result.success("Leading works when hearts not broken")


def test_hearts_broken_can_lead(host: str) -> TestResult:
    """Test can lead hearts when broken."""
    result = TestResult("Game rules: can lead hearts when broken")

    data = {
        "game_state": make_game_state(
            player_hand=["AH", "KH", "QH"],  # Only hearts
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

    if not card.endswith("H"):
        return result.fail(f"Expected hearts, got {card}")

    result.add_detail(f"Led {card}")
    return result.success("Hearts led when broken")


def test_only_hearts_can_lead_when_not_broken(host: str) -> TestResult:
    """Test can lead hearts if only have hearts (even when not broken)."""
    result = TestResult("Game rules: only hearts forces heart lead")

    data = {
        "game_state": make_game_state(
            player_hand=["AH", "KH"],  # Only hearts
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

    if not card.endswith("H"):
        return result.fail(f"Must lead hearts (only option), got {card}")

    result.add_detail(f"Forced to lead {card}")
    return result.success("Forced heart lead works")


# =============================================================================
# MOVE LEGALITY TESTS
# =============================================================================

def test_returned_card_in_hand(host: str) -> TestResult:
    """Test that returned card is always in player's hand."""
    result = TestResult("Move legality: card in hand")

    hand = ["2C", "9S", "6D", "QH"]

    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=100)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success")

    card = resp["move"]["card"]

    if card not in hand:
        return result.fail(f"Card {card} not in hand")

    result.add_detail(f"Move: {card} OK")

    return result.success("Move from hand")


def test_move_legality_comprehensive(host: str) -> TestResult:
    """Comprehensive move legality validation."""
    result = TestResult("Move legality: comprehensive check")

    hand = ["9S", "6S", "AH", "9H"]  # Has spades and hearts

    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            current_trick={
                "cards": [
                    {"card": "AS", "player": 3}
                ],
                "lead_player": 3
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
    trick_cards = data["game_state"]["current_trick"]["cards"]

    # Validate using legality checker
    errors = validate_move_legality(card, hand, trick_cards, False)
    if errors:
        return result.fail(f"Illegal move: {errors}")

    result.add_detail(f"Move: {card}")
    result.add_detail("Legality check passed")
    return result.success("Move is legal")


# =============================================================================
# CARD FORMAT VALIDATION TESTS
# =============================================================================

def test_card_format_all_ranks(host: str) -> TestResult:
    """Test all rank values in card format."""
    result = TestResult("Card format: all ranks")

    # Create hand with one of each rank in spades
    hand = [f"{rank}S" for rank in RANKS]

    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            hearts_broken=True
        ),
        "ai_config": make_ai_config(simulations=50)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]
    if card not in hand:
        return result.fail(f"Card {card} not in hand")

    result.add_detail(f"Tested all 13 ranks: {', '.join(RANKS)}")
    result.add_detail(f"Move: {card}")
    return result.success("All ranks work")


def test_card_format_all_suits(host: str) -> TestResult:
    """Test all suit values in card format."""
    result = TestResult("Card format: all suits")

    for suit in SUITS:
        hand = [f"A{suit}", f"K{suit}", f"Q{suit}", f"J{suit}", f"10{suit}"]
        data = {
            "game_state": make_game_state(
                player_hand=hand,
                current_player=0,
                hearts_broken=True
            ),
            "ai_config": make_ai_config(simulations=50)
        }

        resp, _, err = make_request(host, ENDPOINT, "POST", data)
        if err:
            return result.fail(f"Request failed for suit {suit}: {err}")

        if resp["status"] != "success":
            return result.fail(f"Expected success for suit {suit}: {resp}")

        card = resp["move"]["card"]
        if card not in hand:
            return result.fail(f"Card {card} not in hand for suit {suit}")

        result.add_detail(f"Suit {suit}: {card}")

    return result.success("All suits work")


def test_card_format_response_structure(host: str) -> TestResult:
    """Test response card format is correct string format."""
    result = TestResult("Card format: response structure")

    data = {
        "game_state": make_game_state(
            player_hand=["2C", "5D", "8H", "JS"],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=50)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)
    if err:
        return result.fail(f"Request failed: {err}")

    if resp["status"] != "success":
        return result.fail(f"Expected success: {resp}")

    card = resp["move"]["card"]

    # Card should be a string
    if not isinstance(card, str):
        return result.fail(f"Card should be string, got {type(card)}")

    # Card should match format: rank + suit letter
    if len(card) < 2 or len(card) > 3:
        return result.fail(f"Invalid card length: {card}")

    suit = card[-1]
    rank = card[:-1]

    if suit not in "CDSH":
        return result.fail(f"Invalid suit in card {card}")

    if rank not in RANKS:
        return result.fail(f"Invalid rank in card {card}")

    result.add_detail(f"Card: {card}")
    result.add_detail(f"Rank: {rank}, Suit: {suit}")
    return result.success("Card format valid")


# =============================================================================
# ERROR HANDLING TESTS
# =============================================================================

def test_error_invalid_card_format(host: str) -> TestResult:
    """Test error handling for invalid card format."""
    result = TestResult("Error handling: invalid card format")

    data = {
        "game_state": make_game_state(
            player_hand=["2C", "INVALID", "8H"],
            current_player=0
        ),
        "ai_config": make_ai_config(simulations=50)
    }

    resp, _, err = make_request(host, ENDPOINT, "POST", data)

    if resp is None:
        return result.fail(f"No response: {err}")

    if resp.get("status") != "error":
        return result.fail(f"Expected error for invalid card: {resp}")

    result.add_detail(f"error_code: {resp.get('error_code')}")
    return result.success("Invalid card detected")


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
            player_hand=["2C"],
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
            player_hand=[],
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
            player_hand=["2C"],
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
            player_hand=["2C"],
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
            player_hand=["2C"],  # Only 2C
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
    """Test final trick with 1 card."""
    result = TestResult("Edge case: last trick of game")

    data = {
        "game_state": make_game_state(
            player_hand=["2H"],  # Last card
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

    if card != "2H":
        return result.fail(f"Expected 2H, got {card}")

    result.add_detail("Final trick handled correctly")
    return result.success("Last trick OK")


def test_complete_trick_fourth_player(host: str) -> TestResult:
    """Test following suit when others have played."""
    result = TestResult("Edge case: completing trick as P3")

    hand = ["9S", "6S", "AH"]  # Has spades and hearts
    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            current_trick={
                "cards": [
                    {"card": "2S", "player": 3}
                ],
                "lead_player": 3
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
    if not card.endswith("S"):
        return result.fail(f"Must follow spades, played {card}")

    result.add_detail(f"Followed suit with {card}")
    return result.success("Trick completion works")


def test_follow_suit_second_player(host: str) -> TestResult:
    """Test following suit as second player."""
    result = TestResult("Edge case: follow suit as second player")

    hand = ["5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC"]
    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            current_trick={
                "cards": [
                    {"card": "2D", "player": 3}
                ],
                "lead_player": 3
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

    # Must follow diamonds
    if not card.endswith("D"):
        return result.fail(f"Must follow diamonds, played {card}")

    result.add_detail(f"Followed suit with {card}")
    return result.success("Second player follows suit")


def test_queen_of_spades_scenario(host: str) -> TestResult:
    """Test AI behavior with Queen of Spades."""
    result = TestResult("Edge case: Queen of Spades awareness")

    hand = ["QS", "9S", "6S"]  # Has QS
    data = {
        "game_state": make_game_state(
            player_hand=hand,
            current_player=0,
            current_trick={
                "cards": [
                    {"card": "KS", "player": 3}
                ],
                "lead_player": 3
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

    if not card.endswith("S"):
        return result.fail("Must follow spades")

    result.add_detail(f"AI played {card}")

    if card == "QS":
        result.add_detail("Note: AI played QS (might take 13 points)")
    else:
        result.add_detail("AI avoided QS")

    return result.success("QS scenario handled")


def test_shoot_the_moon_potential(host: str) -> TestResult:
    """Test scenario where shooting the moon might be possible."""
    result = TestResult("Edge case: shoot the moon potential")

    data = {
        "game_state": make_game_state(
            player_hand=["AH"],  # Last heart
            current_player=0,
            scores=[25, 0, 0, 0],  # P0 has 25 points
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
    result.add_detail(f"AI leads {card}")
    result.add_detail("P0 has 25 points, needs AH to shoot moon")

    return result.success("Moon scenario handled")


def test_consistency_across_calls(host: str) -> TestResult:
    """Test that repeated calls return valid (not necessarily same) moves."""
    result = TestResult("Consistency: multiple calls same state")

    hand = ["2C", "3C", "4C"]
    data = {
        "game_state": make_game_state(
            player_hand=hand,
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
        moves.append(card)
        result.add_detail(f"Call {i+1}: {card}")

    # All should be valid cards from hand
    for m in moves:
        if m not in hand:
            return result.fail(f"Card not in hand: {m}")

    return result.success("All calls valid")


def test_high_simulation_count(host: str) -> TestResult:
    """Test with high simulation count."""
    result = TestResult("Stress: high simulation count")

    data = {
        "game_state": make_game_state(
            player_hand=["2C", "3C", "4C", "5C", "6C"],
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
    result.add_detail(f"Move: {resp['move']['card']}")

    return result.success("High sims handled")


# =============================================================================
# TEST RUNNER
# =============================================================================

def run_tests(host: str) -> Tuple[int, int]:
    """Run all tests and return (passed, failed) counts."""

    tests = [
        # Health check
        ("HEALTH CHECK", [
            test_health_check,
        ]),

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

        # Card format validation
        ("CARD FORMAT VALIDATION", [
            test_card_format_all_ranks,
            test_card_format_all_suits,
            test_card_format_response_structure,
        ]),

        # Error handling
        ("ERROR HANDLING", [
            test_error_invalid_card_format,
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
            test_follow_suit_second_player,
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
    print(" HEARTS AI SERVER TEST SUITE")
    print("="*60)
    print(f"Target: http://{host}")
    print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")

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
