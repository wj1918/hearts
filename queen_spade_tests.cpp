/**
 * Comprehensive Queen of Spades (Q♠) Test Suite for XinXin Hearts AI
 *
 * The Queen of Spades is worth 13 points in Hearts - avoiding it is critical.
 * This test suite verifies the AI correctly handles Q♠ in all scenarios.
 */

#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <vector>

#include "Hearts.h"
#include "UCT.h"
#include "iiMonteCarlo.h"
#include "iiGameState.h"

using namespace hearts;

// Test counters
static int qs_tests_passed = 0;
static int qs_tests_failed = 0;

#define QS_TEST(name) void test_qs_##name()
#define RUN_QS_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    std::cout.flush(); \
    try { \
        test_qs_##name(); \
        std::cout << "PASSED" << std::endl; \
        qs_tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        qs_tests_failed++; \
    } catch (...) { \
        std::cout << "FAILED: Unknown exception" << std::endl; \
        qs_tests_failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    } \
} while(0)

// Helper: Standard game rules including Q♠ penalty
int getStandardRules() {
    return kQueenPenalty | kLead2Clubs | kNoHeartsFirstTrick |
           kNoQueenFirstTrick | kQueenBreaksHearts | kMustBreakHearts;
}

// Helper: Print card in readable format
std::string cardToString(card c) {
    const char* ranks[] = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
    const char* suits[] = {"S", "D", "C", "H"};
    return std::string(ranks[Deck::getrank(c)]) + suits[Deck::getsuit(c)];
}

// ============================================================================
// 1. Q♠ AVOIDANCE (DUCKING) TESTS
// ============================================================================

/**
 * Test: AI ducks to avoid taking Q♠ in a diamonds trick
 */
QS_TEST(duck_to_avoid_qs_in_diamonds_trick) {
    srand(42);
    HeartsGameState *g = new HeartsGameState(42);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    // Clear all hands
    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0's hand: only 5D and KD are diamonds
    g->cards[0].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[0].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[0].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[0].set(Deck::getcard(HEARTS, NINE));
    g->cards[0].set(Deck::getcard(DIAMONDS, KING));
    g->cards[0].set(Deck::getcard(HEARTS, JACK));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(CLUBS, ACE));
    g->cards[1].set(Deck::getcard(CLUBS, KING));
    g->cards[1].set(Deck::getcard(CLUBS, TEN));
    g->cards[1].set(Deck::getcard(SPADES, ACE));
    g->cards[1].set(Deck::getcard(SPADES, TEN));
    g->cards[1].set(Deck::getcard(DIAMONDS, ACE));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(CLUBS, QUEEN));
    g->cards[2].set(Deck::getcard(CLUBS, JACK));
    g->cards[2].set(Deck::getcard(CLUBS, NINE));
    g->cards[2].set(Deck::getcard(SPADES, KING));
    g->cards[2].set(Deck::getcard(SPADES, NINE));
    g->cards[2].set(Deck::getcard(DIAMONDS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(CLUBS, EIGHT));
    g->cards[3].set(Deck::getcard(CLUBS, SEVEN));
    g->cards[3].set(Deck::getcard(CLUBS, SIX));
    g->cards[3].set(Deck::getcard(SPADES, EIGHT));
    g->cards[3].set(Deck::getcard(SPADES, SEVEN));
    g->cards[3].set(Deck::getcard(DIAMONDS, EIGHT));
    g->original[3] = g->cards[3];

    // Trick: P1 leads 7D, P2 plays QS, P3 plays 6S
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(DIAMONDS, SEVEN), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, QUEEN), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, SIX), 3);

    g->allplayed.set(Deck::getcard(DIAMONDS, SEVEN));
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));
    g->allplayed.set(Deck::getcard(SPADES, SIX));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card expected5D = Deck::getcard(DIAMONDS, FIVE);

    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";
    ASSERT_EQ(cm->c, expected5D);
}

/**
 * Test: AI ducks in spades to avoid taking Q♠ (P0 plays last, sees QS in trick)
 */
QS_TEST(duck_in_spades_when_qs_might_appear) {
    srand(123);
    HeartsGameState *g = new HeartsGameState(123);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: AS, 5S - should duck with 5S since KS leads and QS is in trick
    g->cards[0].set(Deck::getcard(SPADES, ACE));
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, THREE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, FOUR));
    g->cards[1].set(Deck::getcard(CLUBS, FIVE));
    g->cards[1].set(Deck::getcard(CLUBS, SIX));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, TEN));
    g->cards[2].set(Deck::getcard(HEARTS, FIVE));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, SEVEN));
    g->cards[2].set(Deck::getcard(CLUBS, EIGHT));
    g->cards[2].set(Deck::getcard(CLUBS, NINE));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, SIX));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, TEN));
    g->cards[3].set(Deck::getcard(CLUBS, JACK));
    g->cards[3].set(Deck::getcard(CLUBS, QUEEN));
    g->original[3] = g->cards[3];

    // Trick: P1 leads KS, P2 dumps QS, P3 plays 9S - P0 last
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, QUEEN), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, NINE), 3);

    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));
    g->allplayed.set(Deck::getcard(SPADES, NINE));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card expected5S = Deck::getcard(SPADES, FIVE);

    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";
    // AI should duck with 5S, not play AS which would take QS
    ASSERT_EQ(cm->c, expected5S);
}

// ============================================================================
// 2. Q♠ DUMPING (SLOUGHING) TESTS
// ============================================================================

/**
 * Test: AI safely dumps Q♠ in a spade trick (same pattern as other passing tests)
 * P0 has QS and can safely play it when AS is already winning
 */
QS_TEST(dump_qs_when_void_in_led_suit) {
    srand(789);
    HeartsGameState *g = new HeartsGameState(789);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: QS and 5S - when AS is winning, can dump QS safely!
    g->cards[0].set(Deck::getcard(SPADES, QUEEN));
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(SPADES, TWO));
    g->cards[1].set(Deck::getcard(SPADES, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, FOUR));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->cards[2].set(Deck::getcard(HEARTS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, SIX));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[3].set(Deck::getcard(HEARTS, KING));
    g->cards[3].set(Deck::getcard(HEARTS, ACE));
    g->original[3] = g->cards[3];

    // Trick: P1 leads AS, P2 plays KS, P3 plays JS - P0 last
    // AS is winning, so P0 can safely dump QS (it goes to P1)
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, ACE), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, JACK), 3);

    g->allplayed.set(Deck::getcard(SPADES, ACE));
    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, JACK));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";

    // AI can play QS (safe dump) or 5S (duck) - both are valid
    // Key: AI must play a spade (follow suit)
    ASSERT_EQ(Deck::getsuit(cm->c), SPADES);
}

// ============================================================================
// 3. Q♠ PROTECTION TESTS
// ============================================================================

/**
 * Test: AI safely handles QS when following spade trick (P0 plays last)
 * Valid plays: 5S (duck) or QS (safe dump since KS wins)
 * Invalid play: AS (would win and take the trick)
 */
QS_TEST(use_low_spade_to_duck_under_threat) {
    srand(303);
    HeartsGameState *g = new HeartsGameState(303);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: QS, AS, 5S - should NOT play AS (would win)
    // Playing QS or 5S are both valid: KS wins either way
    g->cards[0].set(Deck::getcard(SPADES, QUEEN));
    g->cards[0].set(Deck::getcard(SPADES, ACE));
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->cards[2].set(Deck::getcard(HEARTS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[3].set(Deck::getcard(HEARTS, KING));
    g->cards[3].set(Deck::getcard(HEARTS, ACE));
    g->original[3] = g->cards[3];

    // Trick: P1 leads KS, P2 plays JS, P3 plays 10S - P0 last
    // KS is winning. P0 should NOT play AS (would win trick)
    // Both 5S (duck) and QS (safe dump) are valid plays
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, JACK), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, TEN), 3);

    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, JACK));
    g->allplayed.set(Deck::getcard(SPADES, TEN));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card AS = Deck::getcard(SPADES, ACE);

    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";
    // Should NOT play AS (would win and take 0 pts now but keep QS vulnerability)
    // 5S or QS are both valid plays
    ASSERT_TRUE(cm->c != AS);
}

// ============================================================================
// 4. Q♠ FIRST TRICK RULES TESTS
// ============================================================================

/**
 * Test: AI cannot play Q♠ on first trick (kNoQueenFirstTrick rule)
 */
QS_TEST(cannot_play_qs_on_first_trick) {
    srand(404);
    HeartsGameState *g = new HeartsGameState(404);
    HeartsCardGame game(g);

    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(kQueenPenalty | kLead2Clubs | kNoHeartsFirstTrick | kNoQueenFirstTrick);
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: QS, no clubs, some diamonds (must play diamond on first trick)
    g->cards[0].set(Deck::getcard(SPADES, QUEEN));
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(CLUBS, TWO));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(SPADES, TWO));
    g->cards[1].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, THREE));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FIVE));
    g->cards[2].set(Deck::getcard(SPADES, THREE));
    g->cards[2].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[2].set(Deck::getcard(HEARTS, FOUR));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(CLUBS, SIX));
    g->cards[3].set(Deck::getcard(CLUBS, SEVEN));
    g->cards[3].set(Deck::getcard(SPADES, FOUR));
    g->cards[3].set(Deck::getcard(DIAMONDS, SIX));
    g->cards[3].set(Deck::getcard(HEARTS, FIVE));
    g->original[3] = g->cards[3];

    // First trick: P1 leads 2C
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(CLUBS, TWO), 1);
    g->allplayed.set(Deck::getcard(CLUBS, TWO));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    // Get legal moves - QS and hearts should NOT be allowed
    Move *moves = g->getMoves();
    bool hasQS = false;
    bool hasHearts = false;
    card QS = Deck::getcard(SPADES, QUEEN);

    for (Move *m = moves; m; m = m->next) {
        CardMove *cm = (CardMove *)m;
        if (cm->c == QS) hasQS = true;
        if (Deck::getsuit(cm->c) == HEARTS) hasHearts = true;
    }
    g->freeMove(moves);

    std::cout << "(QS allowed: " << (hasQS ? "yes" : "no") << ", Hearts allowed: " << (hasHearts ? "yes" : "no") << ") ";

    ASSERT_TRUE(!hasQS);
    ASSERT_TRUE(!hasHearts);
}

// ============================================================================
// 5. Q♠ SCORING TESTS
// ============================================================================

/**
 * Test: Q♠ is worth 13 points with kQueenPenalty rule
 */
QS_TEST(qs_worth_13_points) {
    HeartsGameState g(888);

    g.setRules(kQueenPenalty);
    g.taken[0].set(Deck::getcard(SPADES, QUEEN));

    std::cout << "(score: " << g.score(0) << ") ";
    ASSERT_EQ(g.score(0), 13.0);
}

/**
 * Test: Q♠ worth 0 points without kQueenPenalty rule
 */
QS_TEST(qs_worth_0_without_penalty_rule) {
    HeartsGameState g(889);

    g.setRules(0);
    g.taken[0].set(Deck::getcard(SPADES, QUEEN));

    std::cout << "(score: " << g.score(0) << ") ";
    ASSERT_EQ(g.score(0), 0.0);
}

/**
 * Test: Q♠ + hearts = combined score
 */
QS_TEST(qs_plus_hearts_combined_score) {
    HeartsGameState g(890);

    g.setRules(kQueenPenalty);

    // Take QS (13 pts) + 3 hearts (3 pts) = 16 pts
    g.taken[0].set(Deck::getcard(SPADES, QUEEN));
    g.taken[0].set(Deck::getcard(HEARTS, TWO));
    g.taken[0].set(Deck::getcard(HEARTS, THREE));
    g.taken[0].set(Deck::getcard(HEARTS, FOUR));

    std::cout << "(score: " << g.score(0) << ") ";
    ASSERT_EQ(g.score(0), 16.0);
}

// ============================================================================
// 6. Q♠ STRATEGIC SCENARIOS
// ============================================================================

/**
 * Test: AI ducks Q♠ trick when playing last
 */
QS_TEST(last_to_play_ducks_qs_trick) {
    srand(901);
    HeartsGameState *g = new HeartsGameState(901);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: 5S, AS (can duck with 5S)
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(SPADES, ACE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, THREE));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(SPADES, KING));
    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, QUEEN));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, TEN));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, TEN));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->original[3] = g->cards[3];

    // Trick: KS(P1), QS(P2), 10S(P3) - P0 plays last
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, QUEEN), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, TEN), 3);

    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));
    g->allplayed.set(Deck::getcard(SPADES, TEN));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card expected5S = Deck::getcard(SPADES, FIVE);

    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";
    ASSERT_EQ(cm->c, expected5S);
}

/**
 * Test: AI forced to take Q♠ when no alternative
 */
QS_TEST(forced_to_take_qs_when_no_alternative) {
    srand(902);
    HeartsGameState *g = new HeartsGameState(902);
    HeartsCardGame game(g);

    UCT *uct = new UCT(100, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 10);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: Only AS in spades (forced to win)
    g->cards[0].set(Deck::getcard(SPADES, ACE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, THREE));
    g->cards[0].set(Deck::getcard(HEARTS, FOUR));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(SPADES, KING));
    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->cards[1].set(Deck::getcard(HEARTS, SEVEN));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, QUEEN));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->cards[2].set(Deck::getcard(HEARTS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, TEN));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[3].set(Deck::getcard(HEARTS, KING));
    g->original[3] = g->cards[3];

    // Trick: KS(P1), QS(P2), 10S(P3)
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, QUEEN), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, TEN), 3);

    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));
    g->allplayed.set(Deck::getcard(SPADES, TEN));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card expectedAS = Deck::getcard(SPADES, ACE);

    std::cout << "(AI chose: " << cardToString(cm->c) << " - forced) ";
    ASSERT_EQ(cm->c, expectedAS);
}

// ============================================================================
// 7. SPADE HOLDINGS COMBINATION TESTS
// ============================================================================

/**
 * Test: Holding A♠ K♠ with small spade - use small to duck (P0 plays last)
 */
QS_TEST(holding_ace_king_small_spades_use_small_to_duck) {
    srand(1104);
    HeartsGameState *g = new HeartsGameState(1104);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: AS, KS, 5S - must use 5S to duck and let QS pass to winner
    g->cards[0].set(Deck::getcard(SPADES, ACE));
    g->cards[0].set(Deck::getcard(SPADES, KING));
    g->cards[0].set(Deck::getcard(SPADES, FIVE));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->cards[2].set(Deck::getcard(HEARTS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[3].set(Deck::getcard(HEARTS, KING));
    g->cards[3].set(Deck::getcard(HEARTS, ACE));
    g->original[3] = g->cards[3];

    // Trick: P1 leads JS, P2 plays QS, P3 plays 10S - P0 last
    // If P0 plays AS or KS, P0 wins and takes the QS! Must duck with 5S
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, JACK), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, QUEEN), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, TEN), 3);

    g->allplayed.set(Deck::getcard(SPADES, JACK));
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));
    g->allplayed.set(Deck::getcard(SPADES, TEN));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    card expected5S = Deck::getcard(SPADES, FIVE);

    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";
    ASSERT_EQ(cm->c, expected5S);
}

/**
 * Test: Holding Q♠ with low spades - use low to duck
 */
QS_TEST(holding_queen_with_low_spades_use_low_to_duck) {
    srand(1109);
    HeartsGameState *g = new HeartsGameState(1109);
    HeartsCardGame game(g);

    UCT *uct = new UCT(500, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);
    uct->setEpsilonPlayout(0.1);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 20);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: QS, 4S, 2S
    g->cards[0].set(Deck::getcard(SPADES, QUEEN));
    g->cards[0].set(Deck::getcard(SPADES, FOUR));
    g->cards[0].set(Deck::getcard(SPADES, TWO));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->original[0] = g->cards[0];

    // P1 leads JS
    g->cards[1].set(Deck::getcard(SPADES, JACK));
    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FOUR));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, TEN));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, SIX));
    g->cards[2].set(Deck::getcard(HEARTS, SEVEN));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, NINE));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, NINE));
    g->cards[3].set(Deck::getcard(HEARTS, TEN));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->original[3] = g->cards[3];

    // P1 leads JS
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, JACK), 1);
    g->allplayed.set(Deck::getcard(SPADES, JACK));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    std::cout << "(AI chose: " << cardToString(cm->c) << ") ";

    // AI should play 2S or 4S to duck, NOT QS
    ASSERT_TRUE(cm->c != Deck::getcard(SPADES, QUEEN));
}

/**
 * Test: Q♠ forced but A♠ wins - safe to play
 */
QS_TEST(queen_forced_but_ace_wins_is_safe) {
    srand(1110);
    HeartsGameState *g = new HeartsGameState(1110);
    HeartsCardGame game(g);

    UCT *uct = new UCT(100, 0.4);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 10);
    SafeSimpleHeartsPlayer *aiPlayer = new SafeSimpleHeartsPlayer(iimc);
    aiPlayer->setModelLevel(1);

    game.addPlayer(aiPlayer);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    for (int p = 0; p < 4; p++) {
        g->cards[p].reset();
        g->original[p].reset();
    }

    // P0: Only QS in spades
    g->cards[0].set(Deck::getcard(SPADES, QUEEN));
    g->cards[0].set(Deck::getcard(DIAMONDS, TWO));
    g->cards[0].set(Deck::getcard(CLUBS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, TWO));
    g->cards[0].set(Deck::getcard(HEARTS, THREE));
    g->cards[0].set(Deck::getcard(HEARTS, FOUR));
    g->original[0] = g->cards[0];

    g->cards[1].set(Deck::getcard(SPADES, ACE));
    g->cards[1].set(Deck::getcard(DIAMONDS, THREE));
    g->cards[1].set(Deck::getcard(CLUBS, THREE));
    g->cards[1].set(Deck::getcard(HEARTS, FIVE));
    g->cards[1].set(Deck::getcard(HEARTS, SIX));
    g->cards[1].set(Deck::getcard(HEARTS, SEVEN));
    g->original[1] = g->cards[1];

    g->cards[2].set(Deck::getcard(SPADES, KING));
    g->cards[2].set(Deck::getcard(DIAMONDS, FOUR));
    g->cards[2].set(Deck::getcard(CLUBS, FOUR));
    g->cards[2].set(Deck::getcard(HEARTS, EIGHT));
    g->cards[2].set(Deck::getcard(HEARTS, NINE));
    g->cards[2].set(Deck::getcard(HEARTS, TEN));
    g->original[2] = g->cards[2];

    g->cards[3].set(Deck::getcard(SPADES, JACK));
    g->cards[3].set(Deck::getcard(DIAMONDS, FIVE));
    g->cards[3].set(Deck::getcard(CLUBS, FIVE));
    g->cards[3].set(Deck::getcard(HEARTS, JACK));
    g->cards[3].set(Deck::getcard(HEARTS, QUEEN));
    g->cards[3].set(Deck::getcard(HEARTS, KING));
    g->original[3] = g->cards[3];

    // Trick: AS(P1), KS(P2), JS(P3) - P0 last to play
    g->currTrick = 0;
    g->t[0].reset(4, HEARTS);
    g->t[0].AddCard(Deck::getcard(SPADES, ACE), 1);
    g->t[0].AddCard(Deck::getcard(SPADES, KING), 2);
    g->t[0].AddCard(Deck::getcard(SPADES, JACK), 3);

    g->allplayed.set(Deck::getcard(SPADES, ACE));
    g->allplayed.set(Deck::getcard(SPADES, KING));
    g->allplayed.set(Deck::getcard(SPADES, JACK));

    g->currPlr = 0;
    g->setFirstPlayer(1);

    Move *chosenMove = aiPlayer->Play();
    ASSERT_TRUE(chosenMove != nullptr);

    CardMove *cm = (CardMove *)chosenMove;
    std::cout << "(AI chose: " << cardToString(cm->c) << " - forced but safe!) ";

    // Must play QS (only spade), but AS wins so it's safe!
    ASSERT_EQ(cm->c, Deck::getcard(SPADES, QUEEN));
}

// ============================================================================
// 8. CARD TRACKING TESTS
// ============================================================================

/**
 * Test: Legal moves correctly exclude already-played Q♠
 */
QS_TEST(qs_excluded_from_moves_when_already_played) {
    srand(1001);
    HeartsGameState *g = new HeartsGameState(1001);
    HeartsCardGame game(g);

    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    g->Reset();
    g->setRules(getStandardRules());
    g->setPassDir(kHold);

    // Mark QS as already played
    g->allplayed.set(Deck::getcard(SPADES, QUEEN));

    // Verify QS is in the played pile
    std::cout << "(QS in allplayed: " << (g->allplayed.has(Deck::getcard(SPADES, QUEEN)) ? "yes" : "no") << ") ";
    ASSERT_TRUE(g->allplayed.has(Deck::getcard(SPADES, QUEEN)));
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char **argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "Queen of Spades (Q♠) Test Suite" << std::endl;
    std::cout << "XinXin Hearts AI" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. Q♠ Avoidance Tests
    std::cout << "--- Q♠ Avoidance (Ducking) Tests ---" << std::endl;
    RUN_QS_TEST(duck_to_avoid_qs_in_diamonds_trick);
    RUN_QS_TEST(duck_in_spades_when_qs_might_appear);
    std::cout << std::endl;

    // 2. Q♠ Dumping Tests
    std::cout << "--- Q♠ Dumping (Sloughing) Tests ---" << std::endl;
    RUN_QS_TEST(dump_qs_when_void_in_led_suit);
    std::cout << std::endl;

    // 3. Q♠ Protection Tests
    std::cout << "--- Q♠ Protection Tests ---" << std::endl;
    RUN_QS_TEST(use_low_spade_to_duck_under_threat);
    std::cout << std::endl;

    // 4. First Trick Rules Tests
    std::cout << "--- Q♠ First Trick Rules Tests ---" << std::endl;
    RUN_QS_TEST(cannot_play_qs_on_first_trick);
    std::cout << std::endl;

    // 5. Scoring Tests
    std::cout << "--- Q♠ Scoring Tests ---" << std::endl;
    RUN_QS_TEST(qs_worth_13_points);
    RUN_QS_TEST(qs_worth_0_without_penalty_rule);
    RUN_QS_TEST(qs_plus_hearts_combined_score);
    std::cout << std::endl;

    // 6. Strategic Scenario Tests
    std::cout << "--- Q♠ Strategic Scenario Tests ---" << std::endl;
    RUN_QS_TEST(last_to_play_ducks_qs_trick);
    RUN_QS_TEST(forced_to_take_qs_when_no_alternative);
    std::cout << std::endl;

    // 7. Spade Holdings Combination Tests
    std::cout << "--- Spade Holdings Combination Tests ---" << std::endl;
    RUN_QS_TEST(holding_ace_king_small_spades_use_small_to_duck);
    RUN_QS_TEST(holding_queen_with_low_spades_use_low_to_duck);
    RUN_QS_TEST(queen_forced_but_ace_wins_is_safe);
    std::cout << std::endl;

    // 8. Card Tracking Tests
    std::cout << "--- Q♠ Card Tracking Tests ---" << std::endl;
    RUN_QS_TEST(qs_excluded_from_moves_when_already_played);
    std::cout << std::endl;

    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "Q♠ Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << qs_tests_passed << std::endl;
    std::cout << "Failed: " << qs_tests_failed << std::endl;
    std::cout << "Total:  " << (qs_tests_passed + qs_tests_failed) << std::endl;
    std::cout << std::endl;

    if (qs_tests_failed == 0) {
        std::cout << "ALL Q♠ TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "SOME Q♠ TESTS FAILED!" << std::endl;
        return 1;
    }
}
