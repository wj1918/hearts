# AI Algorithms & Imperfect Info Models

## The Core Challenge

```
Perfect Information Games (Chess):     Imperfect Information Games (Hearts):
┌─────────────────────────────┐        ┌─────────────────────────────┐
│ You see everything          │        │ You see only your cards     │
│ One true game state         │        │ Many possible game states   │
│ Search one tree             │        │ Must reason about beliefs   │
└─────────────────────────────┘        └─────────────────────────────┘
```

---

## Part 1: The AI Algorithm Stack

```
┌─────────────────────────────────────────────────────────────────┐
│                         DECISION                                │
│                           ▲                                     │
│                           │                                     │
│              ┌────────────┴────────────┐                        │
│              │      iiMonteCarlo       │  ← Handles uncertainty │
│              │   (30 world samples)    │                        │
│              └────────────┬────────────┘                        │
│                           │                                     │
│         ┌─────────────────┼─────────────────┐                   │
│         ▼                 ▼                 ▼                   │
│    ┌─────────┐       ┌─────────┐       ┌─────────┐              │
│    │   UCT   │       │   UCT   │  ...  │   UCT   │ ← Tree search│
│    │ World 1 │       │ World 2 │       │World 30 │              │
│    └────┬────┘       └────┬────┘       └────┬────┘              │
│         │                 │                 │                   │
│         ▼                 ▼                 ▼                   │
│    ┌─────────┐       ┌─────────┐       ┌─────────┐              │
│    │ Playout │       │ Playout │  ...  │ Playout │ ← Simulation │
│    └─────────┘       └─────────┘       └─────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 2: Imperfect Information Models

### What You Know vs Don't Know

```
YOUR VIEW (Player 0):
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  Your Hand (KNOWN):     ♠AKQ ♥J32 ♦T98 ♣7654                   │
│                                                                 │
│  Played Cards (KNOWN):  ♠2 ♥A ♦K ♣Q (from previous tricks)     │
│                                                                 │
│  Passed Cards (KNOWN):  You passed ♥QK9, received ♦T98         │
│                                                                 │
│  Player 1 Hand:         ? ? ? ? ? ? ? ? ? ? ? ? ?  (13 cards)  │
│  Player 2 Hand:         ? ? ? ? ? ? ? ? ? ? ? ? ?  (13 cards)  │
│  Player 3 Hand:         ? ? ? ? ? ? ? ? ? ? ? ? ?  (13 cards)  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Three Opponent Models

```
┌─────────────────────────────────────────────────────────────────┐
│                        OM-0 (Basic)                             │
│                       iiHeartsState                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Logic: "I don't have it + not played = someone else has it"   │
│                                                                 │
│  Card Distribution:                                             │
│    Unknown cards split equally among opponents                  │
│    P(Player 1 has ♠J) = P(Player 2 has ♠J) = P(Player 3 has ♠J)│
│                                                                 │
│  Weakness: Ignores behavioral clues                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      OM-1 (Simple)                              │
│                   simpleIIHeartsState                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Logic: "Track voids - if they didn't follow suit, they can't  │
│          have that suit"                                        │
│                                                                 │
│  Example:                                                       │
│    Trick: ♠A led, Player 2 plays ♥5                            │
│    Inference: Player 2 has NO spades                            │
│    Update: P(Player 2 has any ♠) = 0                           │
│                                                                 │
│  Card Probability Matrix:                                       │
│    ┌─────────┬─────────┬─────────┬─────────┐                   │
│    │  Card   │ Player1 │ Player2 │ Player3 │                   │
│    ├─────────┼─────────┼─────────┼─────────┤                   │
│    │   ♠J    │  0.45   │  0.00   │  0.55   │ ← P2 voided       │
│    │   ♠T    │  0.50   │  0.00   │  0.50   │                   │
│    │   ♥Q    │  0.33   │  0.33   │  0.33   │                   │
│    └─────────┴─────────┴─────────┴─────────┘                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     OM-2 (Advanced)                             │
│                  advancedIIHeartsState                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Logic: "Why did they play THAT card?"                          │
│                                                                 │
│  Inferences:                                                    │
│    • Player led low spade → probably doesn't have ♠Q           │
│    • Player dumped ♥K early → probably trying to void hearts   │
│    • Player passed ♣A to me → probably short in clubs          │
│                                                                 │
│  Tracks:                                                        │
│    • Pass history (who passed what)                             │
│    • Play patterns (aggressive vs defensive)                    │
│    • Shooting attempts (collecting all hearts)                  │
│                                                                 │
│  Bayesian Updates:                                              │
│    P(card | action) ∝ P(action | card) × P(card)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 3: How They Work Together

### Step-by-Step Decision Process

```
SITUATION: Your turn, must play a card

STEP 1: Query Belief State
┌─────────────────────────────────────────────────────────────────┐
│  iiHeartsState.GetProbabilities()                               │
│                                                                 │
│  Returns:                                                       │
│    ♠Q: P1=0.4, P2=0.0, P3=0.6  (P2 voided spades)              │
│    ♥A: P1=0.3, P2=0.5, P3=0.2                                  │
│    ...                                                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
STEP 2: Generate 30 Consistent Worlds
┌─────────────────────────────────────────────────────────────────┐
│  for (i = 0; i < 30; i++) {                                     │
│      world[i] = beliefState.SampleConsistentWorld()             │
│  }                                                              │
│                                                                 │
│  World 1:              World 2:              World 3:           │
│  P1: ♠QJ ♥32 ♦A ♣K    P1: ♠J ♥A32 ♦K ♣Q    P1: ♠T ♥32 ♦AK ♣Q  │
│  P2: ♥AK ♦QJ ♣T9      P2: ♥K ♦QJT ♣K9      P2: ♥AK ♦QJ ♣K9    │
│  P3: ♠T ♥Q ♦T9 ♣AQ    P3: ♠QT ♥Q ♦9 ♣AT    P3: ♠QJ ♥AQ ♦T9 ♣AT│
│                                                                 │
│  (Each world consistent with observations)                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
STEP 3: Run UCT on Each World
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  World 1 + UCT(333 sims):  Best=♠K, Score=2.1                  │
│  World 2 + UCT(333 sims):  Best=♠K, Score=1.8                  │
│  World 3 + UCT(333 sims):  Best=♦9, Score=3.2                  │
│  ...                                                            │
│  World 30 + UCT(333 sims): Best=♠K, Score=2.4                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
STEP 4: Aggregate Results
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  ♠K: chosen 22/30 worlds, avg score = 2.1                      │
│  ♦9: chosen 5/30 worlds,  avg score = 3.4                      │
│  ♣7: chosen 3/30 worlds,  avg score = 2.8                      │
│                                                                 │
│  DECISION: Play ♠K (most robust across possible worlds)        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 4: UCT Deep Dive

### The Search Tree

```
                         Root: Current State
                        /          |          \
                       /           |           \
                   Play ♠K      Play ♦9      Play ♣7
                  /    \           |          /    \
                 /      \          |         /      \
            Opp ♠A   Opp ♠2    Opp ♥3    Opp ♣Q   Opp ♣2
               |        |         |         |        |
              ...      ...       ...       ...      ...
                        |
                    PLAYOUT
                   (random to end)
                        |
                    Score: 3 pts
                        |
                   BACKPROPAGATE
                   (update all ancestors)
```

### UCT Selection Formula

```
At each node, pick child maximizing:

         ─────────────────────────────────────────
        │                      ┌─────────────────┐│
        │                      │ ln(parent.visits)││
UCT  =  │  average_reward  +  C│─────────────────││
        │                      │   child.visits   ││
        │                      └─────────────────┘│
         ─────────────────────────────────────────
              EXPLOIT              EXPLORE

C = exploration constant (typically 0.5 - 2.0)
```

### Example Calculation

```
Parent visits: 1000
Child A: 400 visits, total reward 800 (avg 2.0)
Child B: 100 visits, total reward 150 (avg 1.5)

UCT(A) = 2.0 + 1.0 × √(ln(1000)/400) = 2.0 + 0.13 = 2.13
UCT(B) = 1.5 + 1.0 × √(ln(1000)/100) = 1.5 + 0.26 = 1.76

Select Child A (higher UCT)

But if Child B had only 10 visits:
UCT(B) = 1.5 + 1.0 × √(ln(1000)/10) = 1.5 + 0.83 = 2.33

Would select Child B (under-explored)
```

---

## Part 5: World Generation Algorithm

### Consistent World Sampling

```cpp
World SampleConsistentWorld() {
    // Start with known information
    for each player p:
        hand[p] = known_cards[p]  // Cards we've seen them play/pass

    // Distribute unknown cards by probability
    unknown_cards = all_cards - played - my_hand

    for each card c in unknown_cards:
        // Sample player based on probability distribution
        probs = GetProbabilities(c)  // e.g., [0.4, 0.0, 0.6]

        player = WeightedRandomChoice(probs)

        // Validate constraints
        if (hand[player].size < max_hand_size AND
            !player_voided_suit[player][suit(c)]):
            hand[player].add(c)
        else:
            // Resample or redistribute

    return World(hand[0], hand[1], hand[2], hand[3])
}
```

### Probability Update After Trick

```
BEFORE TRICK:
  ♠Q: P1=0.33, P2=0.33, P3=0.33

TRICK PLAYED:
  Lead: ♠A (by you)
  P1 plays: ♠5 (followed suit)
  P2 plays: ♥2 (didn't follow - VOID!)
  P3 plays: ♠9 (followed suit)

AFTER TRICK:
  ♠Q: P1=0.50, P2=0.00, P3=0.50
       ↑           ↑         ↑
   Still possible  Impossible  Still possible
```

---

## Part 6: Playout Strategies

### Standard Playout (Random)

```
while game not over:
    legal_moves = get_legal_moves()
    move = random_choice(legal_moves)
    apply(move)
return final_score
```

### Epsilon-Greedy Playout

```
while game not over:
    legal_moves = get_legal_moves()

    if random() < epsilon:
        move = random_choice(legal_moves)     # Explore
    else:
        move = heuristic_best(legal_moves)    # Exploit

    apply(move)
return final_score
```

### Hearts-Specific Heuristics

```cpp
Move heuristic_best(moves) {
    // Avoid taking points
    if (can_play_low_card)
        return lowest_card

    // Dump dangerous cards when safe
    if (not_winning_trick AND have_high_hearts)
        return highest_heart

    // Avoid Queen of Spades
    if (spades_led AND have_queen AND have_lower_spade)
        return lower_spade

    return random_choice(moves)
}
```

---

## Part 7: Complete Example

```
GAME STATE:
  You (P0):  ♠AK3 ♥72 ♦J98 ♣T654
  Trick:     ♠Q led by P3
  Must play: A spade (following suit)

YOUR OPTIONS:
  ♠A - Will win trick (take ♠Q = 13 points!)
  ♠K - Will win trick (take ♠Q = 13 points!)
  ♠3 - Might lose trick (avoid ♠Q)

AI PROCESS:

1. Generate 30 worlds with different P1/P2 spade holdings

2. For each world, UCT searches:

   World 1 (P1 has ♠J, P2 has ♠T):
     Play ♠3 → P1 plays ♠J → P1 wins → You avoid 13 pts!
     UCT score for ♠3: 0 pts

   World 2 (P1 has ♠2, P2 has ♠T):
     Play ♠3 → P2 plays ♠T → P2 wins → You avoid 13 pts!
     UCT score for ♠3: 0 pts

   World 3 (P1 has ♠2, P2 has ♠4):
     Play ♠3 → highest is ♠Q → P3 wins own trick
     UCT score for ♠3: 0 pts

3. Aggregate across 30 worlds:
   ♠A: avg 13.0 pts (always wins, always takes ♠Q)
   ♠K: avg 13.0 pts (always wins, always takes ♠Q)
   ♠3: avg 0.2 pts  (usually someone else takes it)

4. DECISION: Play ♠3
```

---

## Summary Table

| Component | Purpose | Key Insight |
|-----------|---------|-------------|
| **iiMonteCarlo** | Handle hidden cards | Sample multiple possible worlds |
| **UCT** | Search game tree | Balance explore/exploit with UCB |
| **Playout** | Evaluate leaf nodes | Simulate to end randomly |
| **OM-0** | Basic beliefs | Equal probability for unknowns |
| **OM-1** | Track voids | Update probs when suit not followed |
| **OM-2** | Behavioral inference | "Why did they play that?" |

---

## Code References

### Key Files

| File | Class | Purpose |
|------|-------|---------|
| `UCT.h/cpp` | `UCT` | Monte Carlo Tree Search |
| `iiMonteCarlo.h/cpp` | `iiMonteCarlo` | World sampling wrapper |
| `iiGameState.h/cpp` | `iiHeartsState` | OM-0 basic model |
| `iiGameState.h/cpp` | `simpleIIHeartsState` | OM-1 void tracking |
| `iiGameState.h/cpp` | `advancedIIHeartsState` | OM-2 behavioral |
| `Hearts.h/cpp` | `HeartsPlayout` | Random playout |
| `Hearts.h/cpp` | `HeartsPlayoutCheckShoot` | Shoot-aware playout |

### Key Parameters

```cpp
// Typical configuration
UCT *uct = new UCT();
uct->SetNumSamples(333);      // Simulations per world
uct->SetC1(0.5);              // Exploration constant

iiMonteCarlo *ai = new iiMonteCarlo(
    uct,                      // Base algorithm
    new iiHeartsState(),      // Belief model (OM-0, OM-1, or OM-2)
    30                        // Number of world samples
);
```

---

*This technique is known as "Information Set Monte Carlo Tree Search" (ISMCTS) or "Determinization" in game AI research.*

---

## Related Documentation

- [ANALYSIS.md](ANALYSIS.md) - Complete codebase architecture
- [WORLD_MODELS.md](WORLD_MODELS.md) - Simplified world model explanation
- [AI_STRENGTH.md](AI_STRENGTH.md) - AI player strength assessment
- [SUIT_STRENGTH.md](SUIT_STRENGTH.md) - Proposed suit strength measurement system
- [server/API_SPEC.md](server/API_SPEC.md) - REST API specification

---

*Updated: January 2026*
