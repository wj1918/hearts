# Hearts Game AI Codebase Analysis

## Overview

A sophisticated C++ implementation of Hearts card game with advanced AI using **Monte Carlo Tree Search (MCTS)** and **Imperfect Information handling**.

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
│  Utilities: hash, mt_random, statistics, Timer          │
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
| Source files | ~20 .cpp/.h |
| Lines of code | ~8,000+ |
| Main classes | ~25 |
| AI algorithms | 2 (UCT, iiMonteCarlo) |
| Opponent models | 3 (OM-0, OM-1, OM-2) |

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

| File | Purpose |
|------|---------|
| `Hearts.cpp` | Game rules, scoring, card passing |
| `UCT.cpp` | Monte Carlo Tree Search |
| `iiMonteCarlo.cpp` | Imperfect information wrapper |
| `iiGameState.cpp` | Belief state / opponent modeling |
| `CardGameState.cpp` | Card game framework |
| `Player.cpp` | AI player implementations |
| `main.cpp` | Entry point, game loop |

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

---

## Design Patterns Used

| Pattern | Usage |
|---------|-------|
| **Strategy** | Pluggable algorithms and playout strategies |
| **Template Method** | Algorithm::Play() framework |
| **State** | Different GameState subclasses |
| **Factory** | GetPlayer() creates configured players |
| **Adapter** | UCTModule adapts algorithms to playout interface |

---

*Analysis generated: December 2024*
