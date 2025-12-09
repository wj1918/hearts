// Comprehensive unit tests for game core classes
// Tests Hearts game state, card game state, and related functionality

#include "Hearts.h"
#include "Player.h"
#include "Game.h"
#include "States.h"
#include "statistics.h"

#include <iostream>
#include <vector>
#include <set>
#include <cmath>
#include <cassert>
#include <cstring>

namespace hearts {
namespace {

// ============================================================================
// Test Utilities
// ============================================================================

#define TEST_CHECK(condition) \
  do { \
    if (!(condition)) { \
      std::cerr << "TEST FAILED at " << __FILE__ << ":" << __LINE__ \
                << " - " << #condition << std::endl; \
      std::abort(); \
    } \
  } while(0)

#define TEST_CHECK_EQ(a, b) \
  do { \
    if ((a) != (b)) { \
      std::cerr << "TEST FAILED at " << __FILE__ << ":" << __LINE__ \
                << " - Expected " << (a) << " == " << (b) << std::endl; \
      std::abort(); \
    } \
  } while(0)

// ============================================================================
// Move Tests
// ============================================================================

void TestMove_BasicOperations() {
    std::cout << "  Testing Move basic operations..." << std::endl;

    Move m1;
    TEST_CHECK(m1.next == nullptr);
    TEST_CHECK_EQ(m1.dist, 0);

    Move m2(nullptr, 5);
    TEST_CHECK_EQ(m2.dist, 5);

    std::cout << "    PASSED" << std::endl;
}

void TestMove_LinkedList() {
    std::cout << "  Testing Move linked list operations..." << std::endl;

    Move* head = new Move(nullptr, 1);
    Move* m2 = new Move(nullptr, 2);
    Move* m3 = new Move(nullptr, 3);

    head->next = m2;
    m2->next = m3;

    // Test length
    TEST_CHECK_EQ(head->length(), 3);
    TEST_CHECK_EQ(m2->length(), 2);
    TEST_CHECK_EQ(m3->length(), 1);

    // Cleanup
    delete head;  // Should delete chain via destructor

    std::cout << "    PASSED" << std::endl;
}

void TestMove_Insert() {
    std::cout << "  Testing Move insert (sorted by dist)..." << std::endl;

    Move head;
    head.dist = 0;

    Move* m1 = new Move(nullptr, 5);
    Move* m2 = new Move(nullptr, 3);
    Move* m3 = new Move(nullptr, 7);
    Move* m4 = new Move(nullptr, 1);

    head.insert(m1);
    head.insert(m2);
    head.insert(m3);
    head.insert(m4);

    // Should be sorted by dist in descending order
    Move* curr = head.next;
    int64_t lastDist = INT64_MAX;
    while (curr) {
        TEST_CHECK(curr->dist <= lastDist);
        lastDist = curr->dist;
        curr = curr->next;
    }

    // Cleanup
    while (head.next) {
        Move* temp = head.next;
        head.next = temp->next;
        temp->next = nullptr;
        delete temp;
    }

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// returnValue Tests
// ============================================================================

void TestReturnValue_Basic() {
    std::cout << "  Testing returnValue basic..." << std::endl;

    returnValue rv;
    TEST_CHECK(rv.m == nullptr);
    TEST_CHECK(rv.next == nullptr);

    std::cout << "    PASSED" << std::endl;
}

void TestReturnValue_LinkedList() {
    std::cout << "  Testing returnValue linked list..." << std::endl;

    returnValue* rv1 = new returnValue();
    returnValue* rv2 = new returnValue(nullptr, rv1);
    returnValue* rv3 = new returnValue(nullptr, rv2);

    // Verify chain
    TEST_CHECK(rv3->next == rv2);
    TEST_CHECK(rv2->next == rv1);
    TEST_CHECK(rv1->next == nullptr);

    // Delete head should cleanup chain
    delete rv3;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// HeartsGameState Tests
// ============================================================================

void TestHeartsGameState_Creation() {
    std::cout << "  Testing HeartsGameState creation..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    TEST_CHECK(gs != nullptr);
    TEST_CHECK_EQ(gs->getNumPlayers(), 0u);  // No players added yet

    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsGameState_AddPlayers() {
    std::cout << "  Testing HeartsGameState add players..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    // Add 4 players (standard Hearts game)
    for (int i = 0; i < 4; i++) {
        HeartsDucker* player = new HeartsDucker();
        gs->addPlayer(player);
    }

    TEST_CHECK_EQ(gs->getNumPlayers(), 4u);

    // Verify players are accessible
    for (int i = 0; i < 4; i++) {
        TEST_CHECK(gs->getPlayer(i) != nullptr);
    }

    gs->deletePlayers();
    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsGameState_Rules() {
    std::cout << "  Testing HeartsGameState rules..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    // Set various rules
    int rules = kQueenPenalty | kMustBreakHearts | kDoPassCards;
    gs->setRules(rules);

    TEST_CHECK_EQ(gs->getRules(), rules);

    // Test individual rule flags
    TEST_CHECK(gs->getRules() & kQueenPenalty);
    TEST_CHECK(gs->getRules() & kMustBreakHearts);
    TEST_CHECK(gs->getRules() & kDoPassCards);
    TEST_CHECK(!(gs->getRules() & kJackBonus));

    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsGameState_PassDirection() {
    std::cout << "  Testing HeartsGameState pass direction..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    // Must enable passing in rules for setPassDir to work
    gs->setRules(kDoPassCards);

    gs->setPassDir(kLeftDir);
    TEST_CHECK_EQ(gs->getPassDir(), kLeftDir);

    gs->setPassDir(kRightDir);
    TEST_CHECK_EQ(gs->getPassDir(), kRightDir);

    gs->setPassDir(kAcrossDir);
    TEST_CHECK_EQ(gs->getPassDir(), kAcrossDir);

    gs->setPassDir(kHold);
    TEST_CHECK_EQ(gs->getPassDir(), kHold);

    // Test without kDoPassCards - should always be kHold
    gs->setRules(kQueenPenalty);  // No pass cards
    gs->setPassDir(kLeftDir);
    TEST_CHECK_EQ(gs->getPassDir(), kHold);  // Forced to kHold

    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsGameState_DealCards() {
    std::cout << "  Testing HeartsGameState deal cards..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    // Add 4 players
    for (int i = 0; i < 4; i++) {
        HeartsDucker* player = new HeartsDucker();
        gs->addPlayer(player);
    }

    gs->setRules(kQueenPenalty);
    gs->DealCards();

    // Each player should have 13 cards in a standard game
    // Total cards dealt should be 52
    TEST_CHECK(!gs->Done());

    gs->deletePlayers();
    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsGameState_Reset() {
    std::cout << "  Testing HeartsGameState reset..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    for (int i = 0; i < 4; i++) {
        HeartsDucker* player = new HeartsDucker();
        gs->addPlayer(player);
    }

    gs->setRules(kQueenPenalty);
    gs->DealCards();

    // Reset should reinitialize
    gs->Reset(54321);

    // Should be able to deal again
    gs->DealCards();

    gs->deletePlayers();
    delete gs;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Card and Suit Tests
// ============================================================================

void TestCardSuits() {
    std::cout << "  Testing card suits..." << std::endl;

    TEST_CHECK_EQ(SPADES, 0);
    TEST_CHECK_EQ(DIAMONDS, 1);
    TEST_CHECK_EQ(CLUBS, 2);
    TEST_CHECK_EQ(HEARTS, 3);

    std::cout << "    PASSED" << std::endl;
}

void TestCardRanks() {
    std::cout << "  Testing card ranks..." << std::endl;

    TEST_CHECK_EQ(ACE, 0);
    TEST_CHECK_EQ(KING, 1);
    TEST_CHECK_EQ(QUEEN, 2);
    TEST_CHECK_EQ(JACK, 3);
    TEST_CHECK_EQ(TEN, 4);
    TEST_CHECK_EQ(TWO, 12);

    // Verify ordering (Ace high)
    TEST_CHECK(ACE < KING);
    TEST_CHECK(KING < QUEEN);
    TEST_CHECK(QUEEN < JACK);
    TEST_CHECK(JACK < TEN);

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// HeartsCardGame Tests
// ============================================================================

void TestHeartsCardGame_Creation() {
    std::cout << "  Testing HeartsCardGame creation..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);
    HeartsCardGame game(gs);

    TEST_CHECK_EQ(game.getMaxPoints(), 100);

    game.setMaxPoints(50);
    TEST_CHECK_EQ(game.getMaxPoints(), 50);

    delete gs;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Player Tests
// ============================================================================

void TestHeartsDucker_Creation() {
    std::cout << "  Testing HeartsDucker creation..." << std::endl;

    HeartsDucker* player = new HeartsDucker();

    TEST_CHECK(player != nullptr);
    TEST_CHECK(strcmp(player->getName(), "HeartsDucker") == 0);

    // Test clone
    Player* clone = player->clone();
    TEST_CHECK(clone != nullptr);
    delete clone;

    delete player;

    std::cout << "    PASSED" << std::endl;
}

void TestHeartsShooter_Creation() {
    std::cout << "  Testing HeartsShooter creation..." << std::endl;

    HeartsShooter* player = new HeartsShooter();

    TEST_CHECK(player != nullptr);
    TEST_CHECK(strcmp(player->getName(), "HeartsShooter") == 0);

    delete player;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Statistics Tests
// ============================================================================

void TestStatistics_Creation() {
    std::cout << "  Testing statistics creation..." << std::endl;

    // Note: statistics destructor calls save(), which may create a file
    // We test in a limited scope to avoid file I/O issues
    {
        statistics stats;
        stats.reset();
        // Just verify no crash
    }

    std::cout << "    PASSED" << std::endl;
}

void TestPlayData_Basic() {
    std::cout << "  Testing playData basic..." << std::endl;

    playData pd;
    pd.algorithms = "TestAlgorithm";
    pd.type = kPlayerStat;
    pd.player = 0;
    pd.wins = 5;
    pd.plays = 10;
    pd.score = 50;
    pd.rank = 2;

    TEST_CHECK_EQ(pd.algorithms, "TestAlgorithm");
    TEST_CHECK_EQ(pd.type, kPlayerStat);
    TEST_CHECK_EQ(pd.wins, 5);
    TEST_CHECK_EQ(pd.plays, 10);

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// HashState Tests
// ============================================================================

void TestHashState_Basic() {
    std::cout << "  Testing HashState basic..." << std::endl;

    HashState* hs = new HashState();

    TEST_CHECK(hs != nullptr);
    TEST_CHECK(hs->ret == nullptr);
    TEST_CHECK(hs->as == nullptr);
    TEST_CHECK(hs->ghs == nullptr);

    delete hs;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestFullGameSetup() {
    std::cout << "  Testing full game setup..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(42);

    // Add players
    for (int i = 0; i < 4; i++) {
        HeartsDucker* player = new HeartsDucker();
        gs->addPlayer(player);
    }

    // Set standard rules
    int rules = kQueenPenalty | kMustBreakHearts | kLeadClubs |
                kNoHeartsFirstTrick | kNoQueenFirstTrick;
    gs->setRules(rules);

    // Deal and verify setup
    gs->DealCards();

    TEST_CHECK(!gs->Done());
    TEST_CHECK_EQ(gs->getNumPlayers(), 4u);

    gs->deletePlayers();
    delete gs;

    std::cout << "    PASSED" << std::endl;
}

void TestMultipleGames() {
    std::cout << "  Testing multiple game instances..." << std::endl;

    const int NUM_GAMES = 5;
    std::vector<HeartsGameState*> games;

    // Create multiple game states
    for (int i = 0; i < NUM_GAMES; i++) {
        games.push_back(new HeartsGameState(i * 1000));
    }

    // Add players to each
    for (auto* gs : games) {
        for (int j = 0; j < 4; j++) {
            gs->addPlayer(new HeartsDucker());
        }
        gs->setRules(kQueenPenalty);
        gs->DealCards();
    }

    // Verify all are independent
    for (auto* gs : games) {
        TEST_CHECK(!gs->Done());
        TEST_CHECK_EQ(gs->getNumPlayers(), 4u);
    }

    // Cleanup
    for (auto* gs : games) {
        gs->deletePlayers();
        delete gs;
    }

    std::cout << "    PASSED" << std::endl;
}

void TestGameStateReset() {
    std::cout << "  Testing game state reset consistency..." << std::endl;

    HeartsGameState* gs = new HeartsGameState(12345);

    for (int i = 0; i < 4; i++) {
        gs->addPlayer(new HeartsDucker());
    }
    gs->setRules(kQueenPenalty);

    // Play multiple rounds with reset
    for (int round = 0; round < 3; round++) {
        gs->Reset(round * 100);
        gs->DealCards();
        TEST_CHECK(!gs->Done());
    }

    gs->deletePlayers();
    delete gs;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// HeartsPlayout Tests
// ============================================================================

void TestHeartsPlayout_Creation() {
    std::cout << "  Testing HeartsPlayout creation..." << std::endl;

    HeartsPlayout* playout = new HeartsPlayout();

    TEST_CHECK(playout != nullptr);
    TEST_CHECK(strcmp(playout->GetModuleName(), "HPlayout") == 0);

    delete playout;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Constants and Enums Tests
// ============================================================================

void TestGameConstants() {
    std::cout << "  Testing game constants..." << std::endl;

    TEST_CHECK(INF > 0);
    TEST_CHECK(NINF < 0);
    TEST_CHECK_EQ(MAXPLAYERS, 6u);

    // Pass directions
    TEST_CHECK_EQ(kLeftDir, 1);
    TEST_CHECK_EQ(kRightDir, -1);
    TEST_CHECK_EQ(kAcrossDir, 2);
    TEST_CHECK_EQ(kHold, 0);

    std::cout << "    PASSED" << std::endl;
}

void TestRuleFlags() {
    std::cout << "  Testing rule flags..." << std::endl;

    // Verify flags are unique (powers of 2)
    TEST_CHECK_EQ(kQueenPenalty, 0x0001);
    TEST_CHECK_EQ(kJackBonus, 0x0002);
    TEST_CHECK_EQ(kNoTrickBonus, 0x0004);
    TEST_CHECK_EQ(kMustBreakHearts, 0x0800);
    TEST_CHECK_EQ(kDoPassCards, 0x0400);

    // Test combining flags
    int combined = kQueenPenalty | kJackBonus | kMustBreakHearts;
    TEST_CHECK(combined & kQueenPenalty);
    TEST_CHECK(combined & kJackBonus);
    TEST_CHECK(combined & kMustBreakHearts);
    TEST_CHECK(!(combined & kNoTrickBonus));

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Main Test Runner
// ============================================================================

void RunAllTests() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Game Core Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << std::endl << "Move Tests:" << std::endl;
    TestMove_BasicOperations();
    TestMove_LinkedList();
    TestMove_Insert();

    std::cout << std::endl << "returnValue Tests:" << std::endl;
    TestReturnValue_Basic();
    TestReturnValue_LinkedList();

    std::cout << std::endl << "HeartsGameState Tests:" << std::endl;
    TestHeartsGameState_Creation();
    TestHeartsGameState_AddPlayers();
    TestHeartsGameState_Rules();
    TestHeartsGameState_PassDirection();
    TestHeartsGameState_DealCards();
    TestHeartsGameState_Reset();

    std::cout << std::endl << "Card Tests:" << std::endl;
    TestCardSuits();
    TestCardRanks();

    std::cout << std::endl << "HeartsCardGame Tests:" << std::endl;
    TestHeartsCardGame_Creation();

    std::cout << std::endl << "Player Tests:" << std::endl;
    TestHeartsDucker_Creation();
    TestHeartsShooter_Creation();

    std::cout << std::endl << "Statistics Tests:" << std::endl;
    TestStatistics_Creation();
    TestPlayData_Basic();

    std::cout << std::endl << "HashState Tests:" << std::endl;
    TestHashState_Basic();

    std::cout << std::endl << "HeartsPlayout Tests:" << std::endl;
    TestHeartsPlayout_Creation();

    std::cout << std::endl << "Constants Tests:" << std::endl;
    TestGameConstants();
    TestRuleFlags();

    std::cout << std::endl << "Integration Tests:" << std::endl;
    TestFullGameSetup();
    TestMultipleGames();
    TestGameStateReset();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "All Game Core Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
}

}  // namespace
}  // namespace hearts

int main(int argc, char** argv) {
    hearts::RunAllTests();
    return 0;
}
