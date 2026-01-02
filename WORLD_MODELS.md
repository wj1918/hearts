# 30 World Models Explained

## The Problem: Hidden Information

In Hearts, you only see your own 13 cards. The other 39 cards are hidden across 3 opponents.

```
You know:     ♠AKQ ♥J32 ♦T98 ♣7654  (13 cards)
You don't:    ??? ??? ???           (39 cards across 3 players)
```

The AI can't search a single game tree because it doesn't know the true state.

---

## The Solution: Sample Multiple "Worlds"

Instead of guessing one card distribution, generate **30 plausible distributions** based on:
- Cards played so far
- Who voided which suits
- Pass information
- Probability calculations

```
World 1:  Player B has ♠J, Player C has ♥Q, Player D has ♦K...
World 2:  Player B has ♥Q, Player C has ♦K, Player D has ♠J...
World 3:  Player B has ♦K, Player C has ♠J, Player D has ♥Q...
...
World 30: (another consistent distribution)
```

---

## How It Works

```
For each decision:
┌─────────────────────────────────────────────────────┐
│  Generate 30 consistent world models               │
│                                                     │
│  For each world (1-30):                            │
│    └── Run UCT with 333 simulations                │
│        └── Returns: "Play ♠K scores 2.5 points"    │
│                                                     │
│  Combine results across all 30 worlds:             │
│    ♠K: avg 2.5 pts across worlds                   │
│    ♥3: avg 4.1 pts across worlds                   │
│    ♦9: avg 3.2 pts across worlds                   │
│                                                     │
│  Pick: ♠K (lowest average score)                   │
└─────────────────────────────────────────────────────┘
```

---

## Why 30?

| Models | Accuracy | Speed |
|--------|----------|-------|
| 1 | Poor - single guess could be wrong | Fast |
| 10 | Moderate | Fast |
| **30** | **Good balance** | **Reasonable** |
| 100 | Excellent | Slow |
| 1000 | Diminishing returns | Very slow |

30 provides statistical coverage of likely card distributions without excessive computation.

---

## Code Location

```cpp
// iiMonteCarlo.cpp
iiMonteCarlo::iiMonteCarlo(Algorithm *a, iiGameState *s, int numModels)
{
    // numModels = 30 (typically)
    // For each model, generate consistent world and run algorithm
}
```

---

## Decision Rules

After running UCT on all 30 worlds, combine results using:

| Rule | Method |
|------|--------|
| `kMaxWeighted` | Weight by probability of each world |
| `kMaxAverage` | Simple average across worlds |
| `kMaxAvgVar` | Consider variance (prefer consistent moves) |
| `kMaxMinScore` | Pessimistic - assume worst case |

---

## Example

```
Deciding whether to play ♠Q (Queen of Spades):

World 1:  ♠Q loses trick → +0 pts  (someone else has higher spade)
World 2:  ♠Q wins trick  → +13 pts (you eat the queen)
World 3:  ♠Q loses trick → +0 pts
...
World 30: ♠Q wins trick  → +13 pts

Average: 6.5 pts (risky!)

Compare to ♠2:
All worlds: ♠2 loses trick → +0 pts
Average: 0 pts (safe)

Decision: Play ♠2
```

This is how the AI reasons about uncertainty without knowing the exact cards.

---

## Technical Details

### World Generation (iiGameState)

Three levels of opponent modeling determine how worlds are generated:

| Model | Class | Sophistication |
|-------|-------|----------------|
| OM-0 | `iiHeartsState` | Basic probability tracking |
| OM-1 | `simpleIIHeartsState` | Enhanced calculations |
| OM-2 | `advancedIIHeartsState` | Historical context |

### Consistency Constraints

Each generated world must satisfy:
1. Total cards = 52
2. Each player has correct card count
3. Played cards are not in hands
4. Void constraints respected (if player showed void in suit)
5. Pass constraints (cards passed are known)

### Probability Weighting

Worlds aren't equally likely. The algorithm tracks:
- P(player has card | observations)
- Updates after each trick
- Accounts for what cards players chose NOT to play

---

## Performance

```
30 worlds × 333 UCT simulations = 10,000 total simulations per move
```

With threading, worlds can be evaluated in parallel for faster decisions.

---

*This technique is called "Determinization" or "Perfect Information Monte Carlo Sampling" in game AI literature.*

---

## Related Documentation

- [ANALYSIS.md](ANALYSIS.md) - Complete codebase architecture
- [AI_ALGORITHMS.md](AI_ALGORITHMS.md) - Detailed algorithm explanation
- [AI_STRENGTH.md](AI_STRENGTH.md) - AI player strength assessment
- [SUIT_STRENGTH.md](SUIT_STRENGTH.md) - Proposed suit strength measurement system
- [server/API_SPEC.md](server/API_SPEC.md) - REST API specification

---

*Updated: January 2026*
