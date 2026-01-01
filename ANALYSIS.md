# Hearts Game AI Codebase Analysis

## Overview

A sophisticated C++ implementation of Hearts card game with advanced AI using **Monte Carlo Tree Search (MCTS)** and **Imperfect Information handling**. Originally created by Nathan Sturtevant (University of Alberta/University of Denver).

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
├─────────────────────────────────────────────────────────┤
│  Game Engine          │  AI Algorithms                  │
│  ├── Game             │  ├── UCT (MCTS)                 │
│  ├── GameState        │  ├── iiMonteCarlo               │
│  └── HeartsGameState  │  └── Algorithm (base)           │
├─────────────────────────────────────────────────────────┤
│  Players              │  Imperfect Info Models          │
│  ├── SimpleHearts     │  ├── iiHeartsState (OM-0)       │
│  ├── GlobalHearts     │  ├── simpleIIHeartsState (OM-1) │
│  └── SafeSimpleHearts │  └── advancedIIHeartsState(OM-2)│
├─────────────────────────────────────────────────────────┤
│  REST API Server      │  Utilities                      │
│  ├── HeartsAIServer   │  ├── hash, mt_random            │
│  ├── JsonProtocol     │  ├── statistics, Timer          │
│  └── AIRequestHandler │  └── CardProbabilityData        │
└─────────────────────────────────────────────────────────┘
```

---

## Key AI Algorithms

| Algorithm | Purpose |
|-----------|---------|
| **UCT** | Monte Carlo Tree Search with Upper Confidence Bounds |
| **iiMonteCarlo** | Wrapper for imperfect information - generates multiple "world models" |
| **Opponent Modeling** | 3 levels (OM-0, OM-1, OM-2) of card probability tracking |

### How it works:
1. Generate 30 possible "worlds" (card distributions)
2. Run UCT (333 simulations) on each world
3. Combine results to pick best move under uncertainty

---

## Card Representation

Efficient 64-bit packed format:
```cpp
uint64_t deck;              // All 52 cards in 64 bits
card = (suit << 4) + rank;  // 1 byte per card
```

---

## Class Hierarchy

```
GameState (abstract)
└── CardGameState
    └── HeartsGameState

Algorithm (abstract)
├── UCT
└── iiMonteCarlo

Player (abstract)
├── SimpleHeartsPlayer
├── GlobalHeartsPlayer (1,2,3)
├── SafeSimpleHeartsPlayer
└── HeartsCardPlayer
```

---

## Code Metrics

| Metric | Value |
|--------|-------|
| Source files | 50 .cpp/.h |
| Lines of code | ~14,700 |
| Main classes | ~30 |
| AI algorithms | 2 (UCT, iiMonteCarlo) |
| Opponent models | 3 (OM-0, OM-1, OM-2) |
| Test suite | ~900 lines |
| REST API server | Full HTTP/JSON support |

---

## Strengths

| Aspect | Details |
|--------|---------|
| **Performance** | Bit-packed cards, hash tables, threading |
| **Modularity** | Clean separation of game/AI/players |
| **Extensibility** | Virtual base classes for easy extension |
| **AI sophistication** | State-of-the-art MCTS with imperfect info |

---

## Weaknesses

| Issue | Risk |
|-------|------|
| Raw pointers | Memory leaks possible |
| Fixed-size buffers | Buffer overflow risk |
| Minimal error handling | `exit(1)` on errors |
| Magic numbers | Hardcoded constants |
| Legacy C patterns | `FILE*`, `sprintf` |

---

## Configuration (main.cpp)

```cpp
// Default AI setup:
- 4 players with varying epsilon (exploration)
- 30 world models per decision
- 333 UCT simulations per world
- ~10,000 total simulations per move
```

---

## File Summary

### Core Game Engine
| File | Purpose |
|------|---------|
| `Hearts.cpp/h` | Game rules, scoring, card passing, player types |
| `CardGameState.cpp/h` | Card game framework with Deck class |
| `GameState.cpp/h` | Abstract game state base |
| `Game.cpp/h` | Game orchestration |

### AI Algorithms
| File | Purpose |
|------|---------|
| `UCT.cpp/h` | Monte Carlo Tree Search algorithm |
| `iiMonteCarlo.cpp/h` | Imperfect information wrapper |
| `Algorithm.cpp/h` | Algorithm base class |
| `iiGameState.cpp/h` | Belief state / opponent modeling (OM-0, OM-1, OM-2) |

### REST API Server
| File | Purpose |
|------|---------|
| `server/HeartsAIServer.cpp/h` | HTTP server implementation |
| `server/JsonProtocol.cpp/h` | JSON request/response handling |
| `server/AIRequestHandler.cpp/h` | API endpoint handlers |
| `server/ServerMain.cpp` | Server entry point |
| `server/API_SPEC.md` | Complete API documentation |

### Utilities & Testing
| File | Purpose |
|------|---------|
| `tests.cpp` | Comprehensive test suite (~900 lines) |
| `benchmark.cpp` | Performance benchmarks |
| `statistics.cpp/h` | Game statistics collection |
| `mt_random.cpp/h` | Mersenne Twister RNG |
| `hash.cpp/h` | Hash table implementation |
| `Timer.cpp/h` | Performance timing |

### Entry Points
| File | Purpose |
|------|---------|
| `main.cpp` | Main game simulator (100 AI games) |

---

## Hearts Game Rules (Implemented)

### Pass Directions
```cpp
enum tPassDir {
  kLeftDir=1,    // Pass 3 cards left
  kRightDir=-1,  // Pass 3 cards right
  kAcrossDir=2,  // Pass 3 cards across
  kHold=0        // No passing
};
```

### Configurable Rules (Flags)
| Flag | Description |
|------|-------------|
| `kQueenPenalty` | Queen of Spades = 13 points |
| `kJackBonus` | Jack of Diamonds = -10 points |
| `kNoTrickBonus` | No tricks = -5 points |
| `kQueenBreaksHearts` | Queen breaks hearts |
| `kMustBreakHearts` | Hearts must be broken first |
| `kLead2Clubs` | Game starts with 2 of Clubs |
| `kNoHeartsFirstTrick` | Hearts banned first trick |
| `kNoQueenFirstTrick` | Queen banned first trick |

---

## Build System

- **CMake** based cross-platform build
- **C++11** standard required
- **Threading** via pthreads
- **Platforms**: Windows (MSVC), Linux/macOS (GCC/Clang)

### Build Commands
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Executables
| Executable | Description |
|------------|-------------|
| `hearts` | Main game simulator (100 AI games) |
| `hearts_tests` | Comprehensive test suite |
| `hearts_benchmark` | Performance benchmark |
| `hearts_server` | REST API server |

### Server Usage
```bash
./hearts_server              # Start on port 8080
./hearts_server 3000         # Custom port
./hearts_server 8080 0.0.0.0 # Bind to all interfaces
```

---

## REST API Server

The project includes a production-quality HTTP REST API for remote AI queries.

### Endpoints
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Health check |
| `/api/move` | POST | Compute AI move with full configuration |
| `/api/play-one` | POST | Simplified single-move endpoint |

### Quick Test
```bash
# Health check
curl http://localhost:8080/api/health

# Get AI move
curl -X POST http://localhost:8080/api/move \
  -H "Content-Type: application/json" \
  -d '{"game_state":{"player_hand":["2C","5C","AS"],...},"ai_config":{"simulations":1000}}'
```

See `server/API_SPEC.md` for complete documentation.

---

## Design Patterns Used

| Pattern | Usage |
|---------|-------|
| **Strategy** | Pluggable algorithms and playout strategies |
| **Template Method** | Algorithm::Play() framework |
| **State** | Different GameState subclasses |
| **Factory** | GetPlayer() creates configured players |
| **Adapter** | UCTModule adapts algorithms to playout interface |
| **Bridge** | iiMonteCarlo bridges imperfect info to deterministic UCT |

---

## Third-Party Libraries

| Library | File | Purpose |
|---------|------|---------|
| cpp-httplib | `third_party/httplib.h` | Single-header HTTP server |
| nlohmann/json | `third_party/json.hpp` | JSON parsing/serialization |

---

## Related Documentation

- [AI_ALGORITHMS.md](AI_ALGORITHMS.md) - Detailed AI algorithm explanation
- [WORLD_MODELS.md](WORLD_MODELS.md) - Imperfect information handling
- [server/API_SPEC.md](server/API_SPEC.md) - REST API specification
- [Readme.txt](Readme.txt) - Build instructions

---

*Analysis generated: January 2026*
