# Suit Strength Measurement System

## Overview

A comprehensive suit strength evaluation system for Hearts that measures both **offensive** (ability to control) and **defensive** (ability to avoid points) strength for each suit in a player's hand.

---

## Core Data Structures

### SuitStrength (per suit metrics)

```cpp
struct SuitStrength {
    int suit;                    // SPADES, DIAMONDS, CLUBS, HEARTS

    // Primary metrics (0-100 scale)
    int length;                  // Card count (0-13)
    int dangerScore;             // How likely to take points
    int controlScore;            // Ability to win tricks when needed
    int voidPotential;           // How easy to void this suit

    // Derived metrics
    int overallStrength;         // Combined score
    bool isProtected;            // For spades: is Q protected?
    bool canFlush;               // Can safely lead high to flush Q?
};
```

### SpadeAnalysis (Q protection)

```cpp
struct SpadeAnalysis {
    bool hasQueen;
    bool isProtected;      // Has 4+ spades with Q
    bool canSafelyFlush;   // Has A/K without Q, length >= 3
    int escapeCards;       // Cards below Q that can duck
};
```

### HandStrength (complete hand evaluation)

```cpp
struct HandStrength {
    SuitStrength suits[4];
    int overallDanger;          // 0-100
    int overallControl;         // 0-100
    int voidOpportunities;      // Suits with voidPotential > 50
    SpadeAnalysis spades;
    MoonShootViability moon;    // Moon shoot analysis

    // Recommendations
    int bestSuitToVoid;         // Suit to prioritize voiding
    int bestSuitToKeep;         // Suit to keep for control
    bool recommendMoonAttempt;  // Should attempt moon shoot?
};
```

---

## Calculation Formulas

### 1. Danger Score (0-100)

Higher = more dangerous (likely to take points)

| Factor | Points |
|--------|--------|
| Ace | +20 |
| King | +18 |
| Queen | +15 |
| Jack | +10 |
| Ten | +5 |
| Q (Spades only) | +30 |
| High spades without Q | -10 |
| Hearts (per card) | +3 |
| Short suit (<=2) | -15 |
| Long suit (>=5) | +10 |

### 2. Control Score (0-100)

Higher = more control over the suit

| Factor | Points |
|--------|--------|
| Ace | +25 |
| King | +20 |
| Queen | +15 |
| Length (per card) | +5 |
| Connected sequence | +5 each |
| Void | 0 (no control) |
| Singleton | -20 |

### 3. Void Potential (0-100)

| Length | Score |
|--------|-------|
| 0 (void) | 100 |
| 1 | 90 |
| 2 | 70 |
| 3 | 50 |
| 4 | 25 |
| 5 | 10 |
| 6+ | 0 |

### 4. Overall Strength

```
overallStrength = controlScore - (dangerScore/2) + (voidPotential/4)
```

---

## Moon Shoot Support

### MoonShootViability (offensive)

Analyze if YOUR hand can shoot the moon.

```cpp
struct MoonShootViability {
    int viabilityScore;      // 0-100 (chance of success)
    bool shouldAttempt;      // Recommend attempting?
    int controlledSuits;     // Suits you can dominate
    int heartsControl;       // Control over hearts (0-100)
    int missingHighCards;    // Critical cards not in hand
};
```

**Viability Factors:**

| Factor | Points |
|--------|--------|
| Each Ace | +15 |
| Each King | +10 |
| Each Queen | +5 |
| A Hearts | +15 |
| K Hearts | +10 |
| 5+ Hearts | +15 |
| 7+ Hearts | +10 |
| Has Q Spades | +10 |
| Has A/K Spades (no Q) | +5 |
| No high spades | -20 |
| Each void suit | -15 |

**Attempt Threshold:**
- Viability >= 70
- Controlled suits >= 2
- Hearts control >= 50

### MoonShootThreat (defensive)

Detect when an OPPONENT is attempting to shoot.

```cpp
struct MoonShootThreat {
    int suspectedPlayer;     // -1 if none, 0-3 if detected
    int threatLevel;         // 0-100
    int heartsCollected;     // Hearts taken by suspect
    bool hasQueenSpades;     // Did they take Q?
    int tricksTaken;         // Total tricks by suspect
    bool shouldBlock;        // Recommend blocking?
};
```

**Threat Detection:**
- One player has 3+ hearts AND all points
- Threat level = (hearts * 7) + (hasQ ? 25 : 0) + (tricks>=5 ? 15 : 0)
- Late game bonus: +20 if <= 4 tricks remaining
- Block threshold: threatLevel >= 50

### Moon Prevention Strategy

When blocking a moon attempt:

| Situation | Action | Priority Boost |
|-----------|--------|----------------|
| Have high heart | Play to WIN trick | +50 |
| Can win trick with hearts | Take it | +40 |
| Leading, shooter void in suit | Lead that suit | +30 |


---

# Suit Control Index (SCI) in Card Games
## Concept, Interpretation, Formulas, Pseudo-code, and Worked Example

---

## 1. Definition

**Suit Control Index (SCI)** measures how much control a player has over a single suit during its full lifecycle (all 13 cards).

> SCI estimates the **expected number of tricks a player can control** if the suit is played to exhaustion.

---

## 2. Mathematical Formulation

Let:
- S be a suit
- C = {c₁, c₂, …, cₙ} be the player’s cards in suit S
- R be the set of remaining unseen cards in suit S
- W(cᵢ) be the probability that card cᵢ eventually wins a trick

\[
SCI(S) = \sum_{i=1}^{n} W(c_i)
\]

---

## 3. Fast Heuristic for Win Probability

Let:
- H = number of higher-ranked unseen cards than c
- U = total unseen cards remaining in the suit (excluding c)

\[
W(c) \approx \max\left(0, 1 - \frac{H}{U}\right)
\]

---

## 4. Worked Numeric Example (Step-by-Step)

### Game State
- Suit: **Spades**
- Your hand: **A, 9, 6**
- Cards already played in spades: **K, Q, J, 10**
- Remaining unseen spades: **8 cards**

Remaining unseen ranks:
```
2, 3, 4, 5, 7, 8, 10?, J?  (example abstraction)
```
(Exact distribution unknown; only rank counts matter.)

---

### Step 1: Evaluate ♠A

- Higher unseen cards than A: **0**
- Unseen cards U = 8

\[
W(A) = 1 - 0/8 = 1.00
\]

Ace is guaranteed to win **1 trick**.

---

### Step 2: Evaluate ♠9

- Higher unseen cards: 10, J, Q, K, A → already played  
- Remaining higher unseen than 9: **2** (e.g., 10, J not yet exhausted)
- Unseen cards U = 7

\[
W(9) = 1 - 2/7 \approx 0.71
\]

♠9 has a **71% chance** to win a late trick.

---

### Step 3: Evaluate ♠6

- Higher unseen cards: 7, 8, 9, 10, J, Q, K, A  
- Remaining higher unseen: **6**
- Unseen cards U = 6

\[
W(6) = 1 - 6/6 = 0
\]

♠6 is unlikely to ever win a trick.

---

### Step 4: Compute SCI

\[
SCI = W(A) + W(9) + W(6)
\]

\[
SCI = 1.00 + 0.71 + 0.00 = 1.71
\]

---

### Interpretation

- You can expect to **control about 1.7 tricks** in spades
- Ace gives guaranteed control
- 9 provides **timing / exhaustion leverage**
- 6 is effectively a losing card

---

## 5. Pseudo-code Implementation

```pseudo
function compute_SCI(hand, suit, unseen_cards):
    SCI = 0.0

    for card in hand[suit]:
        higher = count_higher_unseen(card, unseen_cards[suit])
        U = total_unseen(suit) - 1
        win_prob = max(0, 1 - (higher / U))
        SCI += win_prob

    return SCI
```

---

## 6. Practical Insight

This example explains why:
- **A + mid-card** is often stronger than it looks
- Long suits gain value late
- SCI reflects *timing*, not just raw power

---

## 7. References

- Trick-taking games overview  
  https://www.pagat.com/class/trick.html

- Suit establishment (Bridge)  
  https://www.bridgebum.com/suit_establishment.php

- Hand evaluation principles  
  https://www.acbl.org/learn_page/how-to-evaluate-a-bridge-hand/

---

*Document intended for analytical, strategic, and AI research use.*


---

## Suit Control Index (SCI) - Collecting All 13 Cards

### Mathematical Foundation

To collect all 13 cards of a suit during play, you must **win every trick containing a card of that suit**.

#### Initial Deal Probability

The probability of being **dealt** all 13 cards of a suit:

```
P = C(13,13) × C(39,0) / C(52,13)
P = 1 / 635,013,559,600
P ≈ 1.57 × 10⁻¹²
```

This is essentially impossible (~1 in 635 billion).

#### Trick Distribution

| Factor | Value |
|--------|-------|
| Cards in suit S | 13 |
| Tricks played | 13 |
| Max cards of S per trick | 4 |
| Min tricks with S | ⌈13/4⌉ = 4 |
| Expected distribution | 3.25 cards per player |
| Typical tricks with suit S | 7-10 |

### SCI Formula

The **Suit Control Index** measures ability to win tricks and capture all cards of a suit:

```
SCI = (H × w_h + M × w_m + L × w_l) / N
```

| Variable | Meaning | Cards | Weight |
|----------|---------|-------|--------|
| H | High cards held | A, K, Q, J | w_h = 3 |
| M | Mid cards held | 10, 9, 8 | w_m = 2 |
| L | Low cards held | 7, 6, 5, 4, 3, 2 | w_l = 1 |
| N | Total cards held in suit | - | - |

### SCI Calculation Examples

**Hand 1**: A♠ K♠ Q♠ 10♠ 5♠ (5 spades)
```
SCI = (3×3 + 1×2 + 1×1) / 5 = 12/5 = 2.4
```

**Hand 2**: J♠ 9♠ 7♠ 4♠ 2♠ (5 spades)
```
SCI = (1×3 + 1×2 + 3×1) / 5 = 8/5 = 1.6
```

**Hand 3**: 8♠ 6♠ 4♠ 3♠ 2♠ (5 spades)
```
SCI = (0×3 + 1×2 + 4×1) / 5 = 6/5 = 1.2
```

### SCI Interpretation Scale

| SCI Range | Control Level | Collect All 13? |
|-----------|---------------|-----------------|
| < 1.0 | Weak | Very unlikely |
| 1.0 - 1.5 | Moderate | Unlikely |
| 1.5 - 2.0 | Strong | Possible with luck |
| 2.0 - 2.5 | Very Strong | Good chance |
| > 2.5 | Dominant | High probability |

### Position-Adjusted SCI

Account for **card gaps** (missing cards above yours):

```
SCI_adj = SCI × (1 - G/13)
```

Where `G` = number of cards higher than your highest that you don't hold.

**Example**: You hold K♠ Q♠ 10♠ (missing A♠)
```
Base SCI = (2×3 + 1×2) / 3 = 8/3 = 2.67
G = 1 (the Ace)
SCI_adj = 2.67 × (1 - 1/13) = 2.67 × 0.923 = 2.46
```

### Probability Model

Let `p` = probability of winning a random trick ≈ 0.25 (baseline)

For a strong hand with shooting potential, `p` ≈ 0.6-0.8

If suit S appears in `t` tricks:
```
P(collect all S) ≈ p^t
```

| Tricks with S | p=0.25 | p=0.5 | p=0.75 |
|---------------|--------|-------|--------|
| 7 | 0.006% | 0.78% | 13.3% |
| 8 | 0.0015% | 0.39% | 10.0% |
| 9 | 0.0004% | 0.20% | 7.5% |

### SCI to Success Rate Mapping

Empirical relationship:

```
P(collect all) ≈ sigmoid(SCI - 1.8) × base_rate
```

Simplified lookup:

| SCI | P(success) |
|-----|------------|
| 1.0 | ~2% |
| 1.5 | ~8% |
| 2.0 | ~20% |
| 2.5 | ~40% |
| 3.0 | ~65% |

### Multi-Suit Aggregate Control

For shooting the moon, calculate **aggregate control**:

```
Total_Control = Σ(SCI_suit × cards_in_suit) / 13
```

**Threshold**: Total_Control > 2.0 suggests viable moon shot.

### Implementation Structure

```cpp
struct SuitControlIndex {
    int suit;
    double sci;              // Raw SCI value
    double sci_adjusted;     // Position-adjusted SCI
    int gaps;                // Missing high cards above top card
    double collectProb;      // Estimated probability to collect all 13
};

double calculateSCI(const std::vector<Card>& suitCards) {
    int H = 0, M = 0, L = 0;
    for (const auto& card : suitCards) {
        int rank = card.rank();
        if (rank >= JACK) H++;      // A, K, Q, J
        else if (rank >= 8) M++;    // 10, 9, 8
        else L++;                    // 7, 6, 5, 4, 3, 2
    }
    int N = suitCards.size();
    if (N == 0) return 0.0;
    return (H * 3.0 + M * 2.0 + L * 1.0) / N;
}
```

---

## Integration Points

### 1. Pass Card Selection

```cpp
void selectPassCards(int dir, card &a, card &b, card &c) {
    HandStrength hs = evaluateHand(cards[me]);

    if (hs.recommendMoonAttempt) {
        // Moon attempt: keep high cards, pass low cards
        passLowestCards();
    } else {
        // Defensive: pass dangerous cards
        if (hs.spades.hasQueen && !hs.spades.isProtected) {
            passCard(SPADES, QUEEN);  // Priority 1
        }
        passAllCardsFrom(hs.bestSuitToVoid);  // Priority 2
        passHighDangerCards();  // Priority 3
    }
}
```

### 2. Move Ranking

```cpp
int rankMove(Move *m) {
    HandStrength hs = evaluateHand(currentHand);

    // Queen flushing
    if (hs.spades.canSafelyFlush && queenNotPlayed) {
        if (card == ACE_SPADES || card == KING_SPADES)
            return 90;  // High priority to flush Q
    }

    // Moon prevention
    MoonShootThreat threat = detectMoonShootAttempt(gs);
    if (threat.shouldBlock) {
        return rankMoveWithMoonPrevention(m, threat);
    }

    return baseRanking;
}
```

---

## Example Analysis

### Hand Strength Example

```
Hand: ♠AK3 ♥72 ♦QJ98 ♣T654

Suit Analysis:
┌─────────┬────────┬────────┬─────────┬──────────┬──────────┐
│ Suit    │ Length │ Danger │ Control │ VoidPot  │ Overall  │
├─────────┼────────┼────────┼─────────┼──────────┼──────────┤
│ Spades  │ 3      │ 28     │ 50      │ 50       │ 48       │
│ Diamonds│ 4      │ 25     │ 35      │ 25       │ 29       │
│ Clubs   │ 4      │ 10     │ 15      │ 25       │ 17       │
│ Hearts  │ 2      │ 6      │ 10      │ 70       │ 25       │
└─────────┴────────┴────────┴─────────┴──────────┴──────────┘

Spade Analysis:
  Has Queen: No
  Can Flush: Yes (has A,K with 3+ length)

Recommendations:
  Best to void: Hearts (2 cards, 70% void potential)
  Best to keep: Spades (high control, can flush Q)
```

### Moon Shoot Example

```
Hand: ♠AKQ2 ♥AKQJ5 ♦A3 ♣K2

Moon Shoot Analysis:
┌────────────────────┬─────────┐
│ Metric             │ Value   │
├────────────────────┼─────────┤
│ Viability Score    │ 85      │
│ Controlled Suits   │ 3       │
│ Hearts Control     │ 95      │
│ Missing High Cards │ 2       │
│ Should Attempt     │ YES     │
└────────────────────┴─────────┘

Rationale:
  ✓ AKQJ Hearts - Strong hearts control
  ✓ AKQ Spades - Controls spades, has Q
  ✓ A Diamonds - Can win diamond tricks
  ! Only K Clubs - moderate risk

Recommendation: ATTEMPT MOON SHOOT
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `Hearts.h` | Add all structs (SuitStrength, SpadeAnalysis, HandStrength, MoonShootViability, MoonShootThreat) |
| `Hearts.cpp` | Add calculation functions, integrate into selectPassCards, rankMove |
| `iiGameState.cpp` | Track opponent points for moon detection |

---

## Implementation Steps

1. Add structs to `Hearts.h`
2. Implement suit strength calculations in `Hearts.cpp`
3. Implement moon shoot viability analysis
4. Implement moon shoot detection for opponents
5. Add moon prevention to move ranking
6. Integrate into `selectPassCards()` with moon-aware passing
7. Integrate into `rankMove()` with moon prevention
8. Add queen flushing logic using `canSafelyFlush`
9. Write tests for strength and moon calculations
10. Benchmark against current AI to measure improvement

---

## Success Criteria

1. Measurable improvement in win rate vs current AI
2. Better passing decisions (create useful voids)
3. Queen flushing when appropriate
4. Moon shoot attempts when hand is strong enough
5. Moon shoot prevention when opponent is collecting
6. Cleaner code with reusable strength metrics

---

## Related Documentation

- [ANALYSIS.md](ANALYSIS.md) - Codebase architecture
- [AI_ALGORITHMS.md](AI_ALGORITHMS.md) - Algorithm details
- [AI_STRENGTH.md](AI_STRENGTH.md) - AI player strength assessment
- [WORLD_MODELS.md](WORLD_MODELS.md) - Imperfect information handling

---

*Document created: January 2026*
