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
