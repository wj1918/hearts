# Hearts AI Server API Specification

## Overview

The Hearts AI Server provides a REST API for computing optimal moves in the card game Hearts using Monte Carlo Tree Search (MCTS) with Upper Confidence bounds applied to Trees (UCT).

**Base URL:** `http://localhost:8080`

**Content-Type:** `application/json`

---

## Endpoints

### GET /api/health

Health check endpoint to verify the server is running.

#### Request

```
GET /api/health
```

#### Response

**Status:** `200 OK`

```json
{
  "status": "ok"
}
```

---

### POST /api/move

Compute the optimal AI move for a given game state.

#### Request

```
POST /api/move
Content-Type: application/json
```

#### Request Body

```json
{
  "game_state": {
    "player_hand": ["AS", "KS", "QS", "2C", ...],
    "current_player": 0,
    "current_trick": {
      "cards": [{"card": "7D", "player": 1}, ...],
      "lead_player": 1
    },
    "trick_history": [...],
    "played_cards": [[], [], [], []],
    "scores": [0, 0, 0, 0],
    "hearts_broken": false,
    "pass_direction": 0,
    "rules": 2465
  },
  "ai_config": {
    "simulations": 10000,
    "worlds": 30,
    "epsilon": 0.1,
    "use_threads": true,
    "player_type": "safe_simple"
  }
}
```

#### Response

**Status:** `200 OK`

```json
{
  "status": "success",
  "move": {
    "card": "2C",
    "player": 0
  },
  "computation_time_ms": 15.623
}
```

---

### POST /api/play-one

Simplified endpoint for computing a single move with default AI settings.

#### Request

```
POST /api/play-one
Content-Type: application/json
```

#### Request Body

```json
{
  "game_state": {
    "player_hand": ["AS", "KS", "2C", ...],
    "current_player": 0,
    "current_trick": {"cards": [], "lead_player": 0},
    "trick_history": [],
    "scores": [0, 0, 0, 0],
    "hearts_broken": false,
    "pass_direction": 0,
    "rules": 2465
  },
  "simulations": 1000,
  "player_type": "safe_simple"
}
```

**Note:** `simulations` and `player_type` are optional overrides. Defaults: simulations=1000, player_type="safe_simple".

#### Response

Same format as `/api/move`.

---

## Data Types

### Card

Cards are represented as strings in the format `{rank}{suit}`.

#### Card String Format

| Component | Values | Description |
|-----------|--------|-------------|
| Rank | A, K, Q, J, 10, 9, 8, 7, 6, 5, 4, 3, 2 | Card rank |
| Suit | S, D, C, H | Spades, Diamonds, Clubs, Hearts |

#### Card Examples

| Card | String |
|------|--------|
| 2 of Clubs | `"2C"` |
| Ace of Spades | `"AS"` |
| Queen of Spades | `"QS"` |
| King of Hearts | `"KH"` |
| 10 of Diamonds | `"10D"` |
| Jack of Clubs | `"JC"` |

---

### GameState

The current state of the Hearts game.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `player_hand` | string[] | Yes | Array of card strings for the current player's hand |
| `current_player` | integer | No | Always 0 (AI is always player 0 from server's perspective) |
| `current_trick` | Trick | No | The current trick being played |
| `trick_history` | CompletedTrick[] | No | All completed tricks in this round |
| `played_cards` | string[][] | No | Array of 4 arrays, cards each player has won (for scoring) |
| `scores` | number[] | No | Array of 4 scores for each player (default: [0,0,0,0]) |
| `hearts_broken` | boolean | No | Whether hearts have been played (default: false) |
| `pass_direction` | integer | No | Pass direction (default: 0) |
| `rules` | integer or object | No | Game rules bitmask or object (see below) |

**Important:** The server always computes moves for "player 0". The client must translate player indices so that the current player becomes player 0.

### Trick

The current trick being played.

| Field | Type | Description |
|-------|------|-------------|
| `cards` | TrickCard[] | Cards played in this trick so far |
| `lead_player` | integer | Player who led this trick (0-3) |

### TrickCard

A card played in a trick.

| Field | Type | Description |
|-------|------|-------------|
| `card` | string | The card played (e.g., "QS") |
| `player` | integer | The player who played it (0-3) |

### CompletedTrick

A completed trick from the trick history.

| Field | Type | Description |
|-------|------|-------------|
| `cards` | TrickCard[] | All 4 cards played in order |
| `lead_player` | integer | Player who led this trick |
| `winner` | integer | Player who won this trick |

---

### AIConfig

Configuration for the AI player.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `simulations` | integer | 10000 | Total number of MCTS simulations to run |
| `worlds` | integer | 30 | Number of world models for information set MCTS |
| `epsilon` | float | 0.1 | Epsilon for epsilon-greedy playouts |
| `use_threads` | boolean | true | Enable multi-threaded search |
| `player_type` | string | "safe_simple" | AI player type |

**Note:** Simulations are distributed across worlds. Each world gets `simulations / worlds` iterations.

#### Player Types

| Type | Description |
|------|-------------|
| `"simple"` | Basic MCTS player |
| `"safe_simple"` | Conservative player that avoids taking points (recommended) |
| `"global"` | Global evaluation player (version 1) |
| `"global2"` | Global evaluation player (version 2) |
| `"global3"` | Global evaluation player (version 3) |

---

### Pass Direction

| Value | Direction |
|-------|-----------|
| 0 | Hold (no passing) |
| 1 | Pass left |
| -1 | Pass right |
| 2 | Pass across |

---

### Rules

The rules field can be either an integer bitmask or an object with individual flags.

#### Integer Bitmask

| Flag | Value | Description |
|------|-------|-------------|
| `kQueenPenalty` | 0x0001 | Queen of Spades worth 13 points |
| `kJackBonus` | 0x0002 | Jack of Diamonds worth -10 points |
| `kNoTrickBonus` | 0x0004 | No tricks taken worth -5 points |
| `kEndScoreBonus` | 0x0008 | End score bonus |
| `kShootingNeedsJack` | 0x0010 | Must have Jack to shoot the moon |
| `kLead2Clubs` | 0x0020 | 2 of Clubs must lead first trick |
| `kLeadClubs` | 0x0040 | Any club can lead first trick |
| `kNoHeartsFirstTrick` | 0x0080 | No hearts on first trick |
| `kNoQueenFirstTrick` | 0x0100 | No Queen of Spades on first trick |
| `kQueenBreaksHearts` | 0x0200 | Queen of Spades breaks hearts |
| `kDoPassCards` | 0x0400 | Enable card passing |
| `kMustBreakHearts` | 0x0800 | Must break hearts before leading them |
| `kHeartsArentPoints` | 0x1000 | Hearts are worth 1 point each |
| `kNoShooting` | 0x2000 | Disable shooting the moon |

**Standard Rules:** `2465` (0x09A1) = kQueenPenalty | kLead2Clubs | kNoHeartsFirstTrick | kNoQueenFirstTrick | kMustBreakHearts

#### Object Format

Alternatively, rules can be specified as an object:

```json
{
  "rules": {
    "queen_penalty": true,
    "jack_bonus": false,
    "no_trick_bonus": false,
    "must_break_hearts": true,
    "queen_breaks_hearts": true,
    "do_pass_cards": false,
    "no_hearts_first_trick": true,
    "no_queen_first_trick": true,
    "lead_clubs": true,
    "lead_2_clubs": false
  }
}
```

**Default rules** (if not specified): queen_penalty, must_break_hearts, queen_breaks_hearts, no_hearts_first_trick, no_queen_first_trick, lead_clubs

---

## Error Responses

All error responses have the following format:

```json
{
  "status": "error",
  "error_code": "ERROR_CODE",
  "message": "Human-readable error message"
}
```

**Status:** `400 Bad Request` or `500 Internal Server Error`

### Error Codes

| Code | Description |
|------|-------------|
| `PARSE_ERROR` | Invalid JSON in request body |
| `INVALID_GAME_STATE` | Game state is malformed or invalid |
| `NO_LEGAL_MOVES` | No legal moves available in game state |
| `AI_CONFIG_ERROR` | Invalid AI configuration |
| `INTERNAL_ERROR` | Internal server error |
| `UNKNOWN_ERROR` | Unknown error occurred |
| `HTTP_ERROR` | HTTP-level error (404, 405, etc.) |

---

## Examples

### Example 1: First Move of Game

Player has the 2 of Clubs and must lead.

**Request:**
```json
{
  "game_state": {
    "player_hand": ["2C", "5C", "8D", "JH", "AS", "3S", "7D", "9H", "KC", "4D", "6S", "10H", "QS"],
    "current_player": 0,
    "current_trick": {"cards": [], "lead_player": 0},
    "trick_history": [],
    "scores": [0, 0, 0, 0],
    "hearts_broken": false,
    "pass_direction": 0,
    "rules": 2465
  },
  "ai_config": {"simulations": 1000}
}
```

**Response:**
```json
{
  "status": "success",
  "move": {
    "card": "2C",
    "player": 0
  },
  "computation_time_ms": 0.15
}
```

### Example 2: Following Suit

Player must follow clubs.

**Request:**
```json
{
  "game_state": {
    "player_hand": ["5C", "8C", "JD", "AS"],
    "current_player": 0,
    "current_trick": {
      "cards": [
        {"card": "2C", "player": 1},
        {"card": "AC", "player": 2},
        {"card": "7C", "player": 3}
      ],
      "lead_player": 1
    },
    "trick_history": [],
    "scores": [0, 0, 0, 0],
    "hearts_broken": false,
    "rules": 2465
  },
  "ai_config": {"simulations": 1000}
}
```

**Response:**
```json
{
  "status": "success",
  "move": {
    "card": "8C",
    "player": 0
  },
  "computation_time_ms": 8.5
}
```

### Example 3: With Trick History

Game in progress with completed tricks.

**Request:**
```json
{
  "game_state": {
    "player_hand": ["2D", "AD", "3D", "3H", "6C", "5D", "6H", "10D", "9H", "KD"],
    "current_player": 0,
    "current_trick": {"cards": [], "lead_player": 0},
    "trick_history": [
      {
        "cards": [
          {"card": "2C", "player": 2},
          {"card": "KC", "player": 3},
          {"card": "9C", "player": 0},
          {"card": "JC", "player": 1}
        ],
        "lead_player": 2,
        "winner": 3
      },
      {
        "cards": [
          {"card": "9S", "player": 3},
          {"card": "4S", "player": 0},
          {"card": "5S", "player": 1},
          {"card": "6S", "player": 2}
        ],
        "lead_player": 3,
        "winner": 3
      },
      {
        "cards": [
          {"card": "8S", "player": 3},
          {"card": "AS", "player": 0},
          {"card": "QS", "player": 1},
          {"card": "2S", "player": 2}
        ],
        "lead_player": 3,
        "winner": 0
      }
    ],
    "scores": [13, 0, 0, 0],
    "hearts_broken": false,
    "pass_direction": 1,
    "rules": 2465
  },
  "ai_config": {
    "simulations": 5000,
    "use_threads": true
  }
}
```

---

## CORS

The server includes CORS headers to allow cross-origin requests:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: POST, OPTIONS
Access-Control-Allow-Headers: Content-Type
```

---

## Performance Notes

- **Single legal move:** When only one card can legally be played, the AI returns immediately without running simulations (~0.1-0.5ms).
- **Simulations:** More simulations generally produce better moves but take longer. Recommended range: 1000-10000.
- **Threading:** Enable `use_threads` for faster computation on multi-core systems.
- **Computation time:** The `computation_time_ms` field in the response indicates actual AI thinking time.

---

## Server Usage

```bash
# Start server on default port 8080
./hearts_server

# Start server on custom port
./hearts_server 3000

# Start server on specific host and port
./hearts_server 8080 127.0.0.1

# Show help
./hearts_server --help
```

---

## Quick Start

1. Build the server:
   ```bash
   cd hearts
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

2. Start the server:
   ```bash
   ./Release/hearts_server 8080
   ```

3. Test with curl:
   ```bash
   # Health check
   curl http://localhost:8080/api/health

   # Get AI move
   curl -X POST http://localhost:8080/api/move \
     -H "Content-Type: application/json" \
     -d '{"game_state":{"player_hand":["2C","5C","8D","JH","AS"],"current_player":0,"current_trick":{"cards":[],"lead_player":0},"trick_history":[],"scores":[0,0,0,0],"hearts_broken":false,"rules":2465},"ai_config":{"simulations":100}}'
   ```
