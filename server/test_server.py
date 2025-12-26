#!/usr/bin/env python3
"""
Hearts AI Server Test Suite

Usage:
    python test_server.py [host:port]

Example:
    python test_server.py localhost:8080
"""

import json
import sys
import time
import urllib.request
import urllib.error

DEFAULT_HOST = "localhost:8080"

def make_request(host, endpoint, method="GET", data=None):
    """Make HTTP request and return response."""
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
        with urllib.request.urlopen(req, timeout=60) as response:
            elapsed = (time.time() - start) * 1000
            return json.loads(response.read().decode()), elapsed, None
    except urllib.error.URLError as e:
        return None, 0, str(e)
    except json.JSONDecodeError as e:
        return None, 0, f"JSON decode error: {e}"

def test_health(host):
    """Test health endpoint."""
    print("Test 1: Health check")
    resp, elapsed, err = make_request(host, "/api/health")
    if err:
        print(f"  FAILED: {err}")
        return False
    if resp.get("status") == "ok":
        print(f"  PASSED: {resp}")
        return True
    print(f"  FAILED: Unexpected response: {resp}")
    return False

def test_single_card(host):
    """Test single card scenario."""
    print("Test 2: Single card (2C)")
    data = {
        "game_state": {
            "player_hands": [[{"suit": 2, "rank": 12}], [], [], []],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False
        },
        "ai_config": {"simulations": 100}
    }
    resp, elapsed, err = make_request(host, "/api/move", "POST", data)
    if err:
        print(f"  FAILED: {err}")
        return False
    if resp.get("status") == "success":
        move = resp.get("move", {}).get("card", {})
        if move.get("suit") == 2 and move.get("rank") == 12:
            print(f"  PASSED: Returned 2C in {elapsed:.1f}ms")
            return True
    print(f"  FAILED: Unexpected response: {resp}")
    return False

def test_multiple_cards(host, simulations=100):
    """Test multiple cards scenario."""
    print(f"Test 3: Multiple clubs ({simulations} simulations)")
    data = {
        "game_state": {
            "player_hands": [
                [{"suit": 2, "rank": 0}, {"suit": 2, "rank": 12}],
                [{"suit": 2, "rank": 1}, {"suit": 2, "rank": 4}],
                [{"suit": 2, "rank": 2}, {"suit": 2, "rank": 8}],
                [{"suit": 2, "rank": 3}, {"suit": 2, "rank": 10}]
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False
        },
        "ai_config": {"simulations": simulations}
    }
    resp, elapsed, err = make_request(host, "/api/move", "POST", data)
    if err:
        print(f"  FAILED: {err}")
        return False
    if resp.get("status") == "success":
        move = resp.get("move", {}).get("card", {})
        comp_time = resp.get("computation_time_ms", 0)
        print(f"  PASSED: Returned suit={move.get('suit')} rank={move.get('rank')} in {comp_time:.1f}ms (total: {elapsed:.1f}ms)")
        return True
    print(f"  FAILED: Unexpected response: {resp}")
    return False

def test_full_hands(host):
    """Test full 13-card hands."""
    print("Test 4: Full 13 cards per player")
    # Player 0: all clubs, Player 1: all spades, Player 2: all diamonds, Player 3: all hearts
    data = {
        "game_state": {
            "player_hands": [
                [{"suit": 2, "rank": r} for r in range(13)],  # Clubs
                [{"suit": 0, "rank": r} for r in range(13)],  # Spades
                [{"suit": 1, "rank": r} for r in range(13)],  # Diamonds
                [{"suit": 3, "rank": r} for r in range(13)],  # Hearts
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [[], [], [], []],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False
        },
        "ai_config": {"simulations": 100}
    }
    resp, elapsed, err = make_request(host, "/api/move", "POST", data)
    if err:
        print(f"  FAILED: {err}")
        return False
    if resp.get("status") == "success":
        move = resp.get("move", {}).get("card", {})
        comp_time = resp.get("computation_time_ms", 0)
        print(f"  PASSED: Returned suit={move.get('suit')} rank={move.get('rank')} in {comp_time:.1f}ms")
        return True
    print(f"  FAILED: Unexpected response: {resp}")
    return False

def test_mid_game(host):
    """Test mid-game scenario with played cards."""
    print("Test 5: Mid-game with played cards")
    data = {
        "game_state": {
            "player_hands": [
                [{"suit": 2, "rank": 0}, {"suit": 2, "rank": 1}],
                [{"suit": 2, "rank": 2}, {"suit": 2, "rank": 3}],
                [{"suit": 2, "rank": 4}, {"suit": 2, "rank": 5}],
                [{"suit": 2, "rank": 6}, {"suit": 2, "rank": 7}]
            ],
            "current_player": 0,
            "current_trick": {"cards": [], "lead_player": 0},
            "played_cards": [
                [{"suit": 2, "rank": 12}],
                [{"suit": 2, "rank": 11}],
                [{"suit": 2, "rank": 10}],
                [{"suit": 2, "rank": 9}]
            ],
            "scores": [0, 0, 0, 0],
            "hearts_broken": False
        },
        "ai_config": {"simulations": 500}
    }
    resp, elapsed, err = make_request(host, "/api/move", "POST", data)
    if err:
        print(f"  FAILED: {err}")
        return False
    if resp.get("status") == "success":
        move = resp.get("move", {}).get("card", {})
        comp_time = resp.get("computation_time_ms", 0)
        print(f"  PASSED: Returned suit={move.get('suit')} rank={move.get('rank')} in {comp_time:.1f}ms")
        return True
    print(f"  FAILED: Unexpected response: {resp}")
    return False

def test_player_types(host):
    """Test different player types."""
    print("Test 6: Different player types")
    player_types = ["simple", "safe_simple", "global", "global2", "global3"]
    all_passed = True

    for player_type in player_types:
        data = {
            "game_state": {
                "player_hands": [
                    [{"suit": 2, "rank": 0}, {"suit": 2, "rank": 12}],
                    [{"suit": 2, "rank": 1}, {"suit": 2, "rank": 4}],
                    [{"suit": 2, "rank": 2}, {"suit": 2, "rank": 8}],
                    [{"suit": 2, "rank": 3}, {"suit": 2, "rank": 10}]
                ],
                "current_player": 0,
                "current_trick": {"cards": [], "lead_player": 0},
                "played_cards": [[], [], [], []],
                "scores": [0, 0, 0, 0],
                "hearts_broken": False
            },
            "ai_config": {"simulations": 100, "player_type": player_type}
        }
        resp, elapsed, err = make_request(host, "/api/move", "POST", data)
        if err:
            print(f"  {player_type}: FAILED - {err}")
            all_passed = False
        elif resp.get("status") == "success":
            comp_time = resp.get("computation_time_ms", 0)
            print(f"  {player_type}: PASSED ({comp_time:.1f}ms)")
        else:
            print(f"  {player_type}: FAILED - {resp}")
            all_passed = False

    return all_passed

def test_error_handling(host):
    """Test error handling."""
    print("Test 7: Error handling")

    # Test invalid JSON
    print("  Testing invalid JSON...")
    req = urllib.request.Request(
        f"http://{host}/api/move",
        data=b"not valid json",
        headers={"Content-Type": "application/json"},
        method="POST"
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            resp = json.loads(response.read().decode())
    except urllib.error.HTTPError as e:
        resp = json.loads(e.read().decode())

    if resp.get("status") == "error":
        print(f"    PASSED: Got error response for invalid JSON")
    else:
        print(f"    FAILED: Expected error, got: {resp}")
        return False

    return True

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_HOST

    print("=" * 50)
    print("Hearts AI Server Test Suite")
    print(f"Testing server at: {host}")
    print("=" * 50)
    print()

    tests = [
        test_health,
        test_single_card,
        test_multiple_cards,
        test_full_hands,
        test_mid_game,
        test_player_types,
        test_error_handling,
    ]

    passed = 0
    failed = 0

    for test in tests:
        try:
            if test(host):
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"  EXCEPTION: {e}")
            failed += 1
        print()

    print("=" * 50)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 50)

    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
