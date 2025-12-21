/**
 * Comprehensive Test Suite for Hearts AI
 *
 * Tests cover:
 * 1. Card representation and deck operations
 * 2. Game state and rules
 * 3. Move generation
 * 4. AI algorithms (UCT, iiMonteCarlo)
 * 5. Multi-threading
 * 6. Game simulation
 */

#include <iostream>
#include <cassert>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include <thread>

#include "Hearts.h"
#include "UCT.h"
#include "iiMonteCarlo.h"
#include "iiGameState.h"
#include "Timer.h"
#include "statistics.h"

using namespace hearts;

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        tests_failed++; \
    } catch (...) { \
        std::cout << "FAILED: Unknown exception" << std::endl; \
        tests_failed++; \
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

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b); \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    if ((a) <= (b)) { \
        throw std::runtime_error("Assertion failed: " #a " > " #b); \
    } \
} while(0)

// ============================================================================
// 1. CARD REPRESENTATION TESTS
// ============================================================================

TEST(card_creation)
{
    // Test card encoding using Deck static methods
    card c1 = Deck::getcard(SPADES, ACE);
    ASSERT_EQ(Deck::getsuit(c1), SPADES);
    ASSERT_EQ(Deck::getrank(c1), ACE);

    card c2 = Deck::getcard(HEARTS, QUEEN);
    ASSERT_EQ(Deck::getsuit(c2), HEARTS);
    ASSERT_EQ(Deck::getrank(c2), QUEEN);

    card c3 = Deck::getcard(DIAMONDS, TWO);
    ASSERT_EQ(Deck::getsuit(c3), DIAMONDS);
    ASSERT_EQ(Deck::getrank(c3), TWO);

    card c4 = Deck::getcard(CLUBS, KING);
    ASSERT_EQ(Deck::getsuit(c4), CLUBS);
    ASSERT_EQ(Deck::getrank(c4), KING);
}

TEST(card_comparison)
{
    // ACE=0, KING=1, ..., TWO=12 (lower number = higher rank)
    card aceSpades = Deck::getcard(SPADES, ACE);
    card kingSpades = Deck::getcard(SPADES, KING);
    card twoSpades = Deck::getcard(SPADES, TWO);

    ASSERT_TRUE(Deck::getrank(aceSpades) < Deck::getrank(kingSpades));
    ASSERT_TRUE(Deck::getrank(kingSpades) < Deck::getrank(twoSpades));
}

TEST(deck_operations)
{
    Deck d;
    d.reset();
    ASSERT_EQ(d.count(), 0);

    // Add cards using set()
    card aceSpades = Deck::getcard(SPADES, ACE);
    card queenHearts = Deck::getcard(HEARTS, QUEEN);

    d.set(aceSpades);
    ASSERT_EQ(d.count(), 1);
    ASSERT_TRUE(d.has(aceSpades));
    ASSERT_TRUE(!d.has(queenHearts));

    d.set(queenHearts);
    ASSERT_EQ(d.count(), 2);
    ASSERT_TRUE(d.has(queenHearts));

    // Remove card using clear()
    d.clear(aceSpades);
    ASSERT_EQ(d.count(), 1);
    ASSERT_TRUE(!d.has(aceSpades));
    ASSERT_TRUE(d.has(queenHearts));
}

TEST(deck_suit_operations)
{
    Deck d;
    d.reset();

    // Add all spades
    for (int rank = ACE; rank <= TWO; rank++)
    {
        d.set(Deck::getcard(SPADES, rank));
    }

    ASSERT_EQ(d.count(), 13);
    ASSERT_EQ(d.suitCount(SPADES), 13);
    ASSERT_EQ(d.suitCount(HEARTS), 0);
    ASSERT_EQ(d.suitCount(DIAMONDS), 0);
    ASSERT_EQ(d.suitCount(CLUBS), 0);

    // Check has suit
    ASSERT_TRUE(d.hasSuit(SPADES));
    ASSERT_TRUE(!d.hasSuit(HEARTS));
}

TEST(full_deck)
{
    Deck d;
    d.fill();

    ASSERT_EQ(d.count(), 52);

    // All 4 suits should have 13 cards
    ASSERT_EQ(d.suitCount(SPADES), 13);
    ASSERT_EQ(d.suitCount(HEARTS), 13);
    ASSERT_EQ(d.suitCount(DIAMONDS), 13);
    ASSERT_EQ(d.suitCount(CLUBS), 13);

    // Every card should be present
    for (int suit = SPADES; suit <= HEARTS; suit++)
    {
        for (int rank = ACE; rank <= TWO; rank++)
        {
            ASSERT_TRUE(d.has(Deck::getcard(suit, rank)));
        }
    }
}

// ============================================================================
// 2. GAME STATE TESTS
// ============================================================================

TEST(game_state_creation)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players to properly initialize the game
    HeartsDucker *p0 = new HeartsDucker();
    HeartsDucker *p1 = new HeartsDucker();
    HeartsDucker *p2 = new HeartsDucker();
    HeartsDucker *p3 = new HeartsDucker();

    game.addPlayer(p0);
    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);

    ASSERT_EQ(g->getNumPlayers(), 4);
    ASSERT_TRUE(!g->Done());

    // Game owns the players and game state, no need to delete them separately
}

TEST(game_state_deal)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players to properly initialize the game
    HeartsDucker *p0 = new HeartsDucker();
    HeartsDucker *p1 = new HeartsDucker();
    HeartsDucker *p2 = new HeartsDucker();
    HeartsDucker *p3 = new HeartsDucker();

    game.addPlayer(p0);
    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    // Each player should have 13 cards after deal
    for (unsigned int p = 0; p < g->getNumPlayers(); p++)
    {
        ASSERT_EQ(g->cards[p].count(), 13);
    }

    // Total cards should be 52
    unsigned int totalCards = 0;
    for (unsigned int p = 0; p < g->getNumPlayers(); p++)
    {
        totalCards += g->cards[p].count();
    }
    ASSERT_EQ(totalCards, 52);

    // Game owns the players and game state
}

TEST(pass_directions)
{
    // Test pass direction enum values
    ASSERT_EQ(kLeftDir, 1);
    ASSERT_EQ(kRightDir, -1);
    ASSERT_EQ(kAcrossDir, 2);
    ASSERT_EQ(kHold, 0);
}

// ============================================================================
// 3. MOVE GENERATION TESTS
// ============================================================================

TEST(move_generation_basic)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players to properly initialize the game
    HeartsDucker *p0 = new HeartsDucker();
    HeartsDucker *p1 = new HeartsDucker();
    HeartsDucker *p2 = new HeartsDucker();
    HeartsDucker *p3 = new HeartsDucker();

    game.addPlayer(p0);
    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    Move *moves = g->getMoves();
    ASSERT_NE(moves, nullptr);

    // Count moves
    int moveCount = 0;
    for (Move *m = moves; m != nullptr; m = m->next)
    {
        moveCount++;
    }

    // Should have at least 1 legal move
    ASSERT_GT(moveCount, 0);

    // Game owns the players and game state
}

// ============================================================================
// 4. AI ALGORITHM TESTS
// ============================================================================

TEST(uct_creation)
{
    UCT *uct = new UCT(100, 1.0);  // 100 samples, C=1.0

    ASSERT_NE(uct, nullptr);
    ASSERT_NE(uct->getName(), nullptr);

    delete uct;
}

TEST(uct_with_playout_module)
{
    UCT *uct = new UCT(100, 1.0);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    ASSERT_NE(uct, nullptr);

    delete uct;
}

TEST(uct_clone)
{
    UCT *uct = new UCT(200, 1.5);

    Algorithm *clone = uct->clone();
    ASSERT_NE(clone, nullptr);

    delete clone;
    delete uct;
}

TEST(iiMonteCarlo_creation)
{
    UCT *uct = new UCT(10, 1.0);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 5);  // 5 world models

    ASSERT_NE(iimc, nullptr);
    ASSERT_EQ(iimc->getNumModels(), 5);

    delete iimc;
}

TEST(iiMonteCarlo_decision_rules)
{
    UCT *uct = new UCT(10, 1.0);
    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 5);

    // Test setting different decision rules
    iimc->setDecisionRule(kMaxWeighted);
    iimc->setDecisionRule(kMaxAverage);
    iimc->setDecisionRule(kMaxAvgVar);
    iimc->setDecisionRule(kMaxMinScore);

    delete iimc;
}

// ============================================================================
// 5. MULTI-THREADING TESTS
// ============================================================================

TEST(threading_enabled)
{
    unsigned int numCPU = std::thread::hardware_concurrency();
    std::cout << "(detected " << numCPU << " CPUs) ";
    ASSERT_GT(numCPU, 0);
}

TEST(threaded_iiMonteCarlo)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    UCT *uct = new UCT(20, 1.0);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 4);  // 4 world models
    iimc->setUseThreads(true);  // Enable threading on iiMonteCarlo

    SimpleHeartsPlayer *player = new SimpleHeartsPlayer(iimc);
    player->setModelLevel(1);

    // Add players to game - player under test is player 0
    game.addPlayer(player);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    // Time the execution
    auto start = std::chrono::high_resolution_clock::now();

    Move *move = player->Play();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    ASSERT_NE(move, nullptr);
    std::cout << "(" << duration.count() << "ms) ";

    // Game owns all players and game state
}

TEST(single_threaded_iiMonteCarlo)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    UCT *uct = new UCT(20, 1.0);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    iiMonteCarlo *iimc = new iiMonteCarlo(uct, 4);
    iimc->setUseThreads(false);  // Disable threading

    SimpleHeartsPlayer *player = new SimpleHeartsPlayer(iimc);
    player->setModelLevel(1);

    // Add players to game
    game.addPlayer(player);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    Move *move = player->Play();
    ASSERT_NE(move, nullptr);

    // Game owns all players and game state
}

// ============================================================================
// 6. PLAYER TESTS
// ============================================================================

TEST(simple_hearts_player)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    UCT *uct = new UCT(30, 1.0);
    HeartsPlayout *playout = new HeartsPlayout();
    uct->setPlayoutModule(playout);

    SimpleHeartsPlayer *player = new SimpleHeartsPlayer(uct);
    player->setModelLevel(1);

    // Add players to game
    game.addPlayer(player);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    ASSERT_NE(player->getName(), nullptr);

    Move *move = player->Play();
    ASSERT_NE(move, nullptr);

    // Game owns all players and game state
}

TEST(hearts_ducker_player)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    HeartsDucker *player = new HeartsDucker();

    // Add players to game
    game.addPlayer(player);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    Move *move = player->Play();
    ASSERT_NE(move, nullptr);

    // Game owns all players and game state
}

TEST(hearts_shooter_player)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    HeartsShooter *player = new HeartsShooter();

    // Add players to game
    game.addPlayer(player);
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    Move *move = player->Play();
    ASSERT_NE(move, nullptr);

    // Game owns all players and game state
}

// ============================================================================
// 7. GAME SIMULATION TESTS
// ============================================================================

TEST(full_game_simulation)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Create 4 simple players
    HeartsDucker *p0 = new HeartsDucker();
    HeartsDucker *p1 = new HeartsDucker();
    HeartsDucker *p2 = new HeartsDucker();
    HeartsDucker *p3 = new HeartsDucker();

    game.addPlayer(p0);
    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);

    g->setPassDir(kHold);

    // Play until done
    int maxMoves = 200;
    int moves = 0;
    while (!game.Done() && moves < maxMoves)
    {
        game.doOnePlay();
        moves++;
    }

    ASSERT_TRUE(game.Done());
    std::cout << "(" << moves << " moves) ";
}

TEST(multiple_hands_simulation)
{
    srand((unsigned int)time(NULL));

    int numHands = 3;
    for (int hand = 0; hand < numHands; hand++)
    {
        HeartsGameState *g = new HeartsGameState(rand());
        HeartsCardGame game(g);

        HeartsDucker *p0 = new HeartsDucker();
        HeartsDucker *p1 = new HeartsDucker();
        HeartsDucker *p2 = new HeartsDucker();
        HeartsDucker *p3 = new HeartsDucker();

        game.addPlayer(p0);
        game.addPlayer(p1);
        game.addPlayer(p2);
        game.addPlayer(p3);

        g->setPassDir(kHold);

        int maxMoves = 200;
        int moves = 0;
        while (!game.Done() && moves < maxMoves)
        {
            game.doOnePlay();
            moves++;
        }

        ASSERT_TRUE(game.Done());
    }
    std::cout << "(" << numHands << " hands) ";
}

// ============================================================================
// 8. TIMER TESTS
// ============================================================================

TEST(timer_basic)
{
    Timer t;
    t.StartTimer();

    // Do some work
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++)
        sum += i;

    double elapsed = t.EndTimer();

    ASSERT_GT(elapsed, 0.0);
    std::cout << "(" << elapsed << "s) ";
}

// ============================================================================
// 9. IMPERFECT INFORMATION STATE TESTS
// ============================================================================

TEST(ii_state_creation)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    // Get imperfect information state for player 0
    iiGameState *iiState = g->getiiGameState(true, 0, nullptr);
    ASSERT_NE(iiState, nullptr);

    delete iiState;
    // Game owns players and game state
}

TEST(ii_state_world_generation)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    iiGameState *iiState = g->getiiGameState(true, 0, nullptr);

    // Generate a world model
    double prob;
    GameState *world = iiState->getGameState(prob);

    ASSERT_NE(world, nullptr);
    ASSERT_GT(prob, 0.0);

    delete world;
    delete iiState;
    // Game owns players and game state
}

TEST(ii_state_multiple_worlds)
{
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    // Add 4 players
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());
    game.addPlayer(new HeartsDucker());

    // Reset deals the cards
    g->Reset();
    g->setPassDir(kHold);

    iiGameState *iiState = g->getiiGameState(true, 0, nullptr);

    std::vector<GameState *> worlds;
    std::vector<double> probs;

    // Generate 10 worlds
    iiState->getGameStates(10, worlds, probs);

    ASSERT_EQ(worlds.size(), 10);
    ASSERT_EQ(probs.size(), 10);

    // Clean up
    for (size_t i = 0; i < worlds.size(); i++)
        delete worlds[i];

    delete iiState;
    // Game owns players and game state
}

// ============================================================================
// 10. STATISTICS TESTS
// ============================================================================

TEST(statistics_collection)
{
    statistics s;

    // Run a quick game and collect stats
    srand(12345);
    HeartsGameState *g = new HeartsGameState(12345);
    HeartsCardGame game(g);

    HeartsDucker *p0 = new HeartsDucker();
    HeartsDucker *p1 = new HeartsDucker();
    HeartsDucker *p2 = new HeartsDucker();
    HeartsDucker *p3 = new HeartsDucker();

    game.addPlayer(p0);
    game.addPlayer(p1);
    game.addPlayer(p2);
    game.addPlayer(p3);

    g->setPassDir(kHold);
    game.Play();

    s.collect(&game, g);

    ASSERT_TRUE(game.Done());
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char **argv)
{
    std::cout << "========================================" << std::endl;
    std::cout << "Hearts AI Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. Card representation tests
    std::cout << "--- Card Representation Tests ---" << std::endl;
    RUN_TEST(card_creation);
    RUN_TEST(card_comparison);
    RUN_TEST(deck_operations);
    RUN_TEST(deck_suit_operations);
    RUN_TEST(full_deck);
    std::cout << std::endl;

    // 2. Game state tests
    std::cout << "--- Game State Tests ---" << std::endl;
    RUN_TEST(game_state_creation);
    RUN_TEST(game_state_deal);
    RUN_TEST(pass_directions);
    std::cout << std::endl;

    // 3. Move generation tests
    std::cout << "--- Move Generation Tests ---" << std::endl;
    RUN_TEST(move_generation_basic);
    std::cout << std::endl;

    // 4. AI algorithm tests
    std::cout << "--- AI Algorithm Tests ---" << std::endl;
    RUN_TEST(uct_creation);
    RUN_TEST(uct_with_playout_module);
    RUN_TEST(uct_clone);
    RUN_TEST(iiMonteCarlo_creation);
    RUN_TEST(iiMonteCarlo_decision_rules);
    std::cout << std::endl;

    // 5. Multi-threading tests
    std::cout << "--- Multi-Threading Tests ---" << std::endl;
    RUN_TEST(threading_enabled);
    RUN_TEST(single_threaded_iiMonteCarlo);
    RUN_TEST(threaded_iiMonteCarlo);
    std::cout << std::endl;

    // 6. Player tests
    std::cout << "--- Player Tests ---" << std::endl;
    RUN_TEST(simple_hearts_player);
    RUN_TEST(hearts_ducker_player);
    RUN_TEST(hearts_shooter_player);
    std::cout << std::endl;

    // 7. Game simulation tests
    std::cout << "--- Game Simulation Tests ---" << std::endl;
    RUN_TEST(full_game_simulation);
    RUN_TEST(multiple_hands_simulation);
    std::cout << std::endl;

    // 8. Timer tests
    std::cout << "--- Timer Tests ---" << std::endl;
    RUN_TEST(timer_basic);
    std::cout << std::endl;

    // 9. Imperfect information tests
    std::cout << "--- Imperfect Information Tests ---" << std::endl;
    RUN_TEST(ii_state_creation);
    RUN_TEST(ii_state_world_generation);
    RUN_TEST(ii_state_multiple_worlds);
    std::cout << std::endl;

    // 10. Statistics tests
    std::cout << "--- Statistics Tests ---" << std::endl;
    RUN_TEST(statistics_collection);
    std::cout << std::endl;

    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    std::cout << "Total:  " << (tests_passed + tests_failed) << std::endl;
    std::cout << std::endl;

    if (tests_failed == 0)
    {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "SOME TESTS FAILED!" << std::endl;
        return 1;
    }
}
