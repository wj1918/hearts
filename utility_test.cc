// Comprehensive unit tests for utility classes
// Tests fpUtil, hash, mt_random, and Timer

#include "fpUtil.h"
#include "hash.h"
#include "mt_random.h"
#include "Timer.h"

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <cassert>
#include <thread>
#include <atomic>
#include <chrono>

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
// fpUtil Tests
// ============================================================================

void TestFpUtil_Fless() {
    std::cout << "  Testing fless()..." << std::endl;

    // Basic comparisons
    TEST_CHECK(fless(1.0, 2.0));
    TEST_CHECK(!fless(2.0, 1.0));
    TEST_CHECK(!fless(1.0, 1.0));

    // Tolerance testing - values within TOLERANCE_D should not be fless
    double tolerance = 0.000001;
    TEST_CHECK(!fless(1.0, 1.0 + tolerance * 0.5));  // Within tolerance
    TEST_CHECK(fless(1.0, 1.0 + tolerance * 2));     // Beyond tolerance

    // Negative numbers
    TEST_CHECK(fless(-2.0, -1.0));
    TEST_CHECK(!fless(-1.0, -2.0));

    // Float version
    TEST_CHECK(fless(1.0f, 2.0f));
    TEST_CHECK(!fless(2.0f, 1.0f));

    std::cout << "    PASSED" << std::endl;
}

void TestFpUtil_Fgreater() {
    std::cout << "  Testing fgreater()..." << std::endl;

    // Basic comparisons
    TEST_CHECK(fgreater(2.0, 1.0));
    TEST_CHECK(!fgreater(1.0, 2.0));
    TEST_CHECK(!fgreater(1.0, 1.0));

    // Tolerance testing
    double tolerance = 0.000001;
    TEST_CHECK(!fgreater(1.0 + tolerance * 0.5, 1.0));  // Within tolerance
    TEST_CHECK(fgreater(1.0 + tolerance * 2, 1.0));     // Beyond tolerance

    // Negative numbers
    TEST_CHECK(fgreater(-1.0, -2.0));
    TEST_CHECK(!fgreater(-2.0, -1.0));

    // Float version
    TEST_CHECK(fgreater(2.0f, 1.0f));
    TEST_CHECK(!fgreater(1.0f, 2.0f));

    std::cout << "    PASSED" << std::endl;
}

void TestFpUtil_Fequal() {
    std::cout << "  Testing fequal()..." << std::endl;

    // Exact equality
    TEST_CHECK(fequal(1.0, 1.0));
    TEST_CHECK(fequal(0.0, 0.0));
    TEST_CHECK(fequal(-5.5, -5.5));

    // Within tolerance
    double tolerance = 0.000001;
    TEST_CHECK(fequal(1.0, 1.0 + tolerance * 0.5));
    TEST_CHECK(fequal(1.0, 1.0 - tolerance * 0.5));

    // Beyond tolerance
    TEST_CHECK(!fequal(1.0, 1.0 + tolerance * 2));
    TEST_CHECK(!fequal(1.0, 2.0));

    // Float version
    TEST_CHECK(fequal(1.0f, 1.0f));
    float ftol = 0.00005f;
    TEST_CHECK(fequal(1.0f, 1.0f + ftol * 0.5f));
    TEST_CHECK(!fequal(1.0f, 1.0f + ftol * 2.0f));

    std::cout << "    PASSED" << std::endl;
}

void TestFpUtil_EdgeCases() {
    std::cout << "  Testing fpUtil edge cases..." << std::endl;

    // Very small numbers
    TEST_CHECK(fequal(0.0000001, 0.0000001));
    TEST_CHECK(fless(0.0, 0.000002));

    // Very large numbers
    TEST_CHECK(fequal(1000000.0, 1000000.0));
    TEST_CHECK(fless(999999.0, 1000000.0));

    // Zero comparisons
    TEST_CHECK(fequal(0.0, 0.0));
    TEST_CHECK(fless(0.0, 1.0));
    TEST_CHECK(fgreater(1.0, 0.0));

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// mt_random Tests
// ============================================================================

void TestMtRandom_Seeding() {
    std::cout << "  Testing mt_random seeding..." << std::endl;

    mt_random rng1;
    mt_random rng2;

    // Same seed should produce same sequence
    rng1.srand(12345);
    rng2.srand(12345);

    for (int i = 0; i < 100; i++) {
        TEST_CHECK_EQ(rng1.rand_long(), rng2.rand_long());
    }

    // Different seeds should produce different sequences
    rng1.srand(12345);
    rng2.srand(54321);

    bool different = false;
    for (int i = 0; i < 100; i++) {
        if (rng1.rand_long() != rng2.rand_long()) {
            different = true;
            break;
        }
    }
    TEST_CHECK(different);

    std::cout << "    PASSED" << std::endl;
}

void TestMtRandom_RandLong() {
    std::cout << "  Testing mt_random rand_long()..." << std::endl;

    mt_random rng;
    rng.srand(42);

    // Generate many values and check they're in valid range
    for (int i = 0; i < 10000; i++) {
        uint32_t val = rng.rand_long();
        // All values should be valid uint32_t (always true, but sanity check)
        TEST_CHECK(val <= UINT32_MAX);
    }

    // Check for reasonable distribution (no constant values)
    std::set<uint32_t> unique_values;
    rng.srand(42);
    for (int i = 0; i < 1000; i++) {
        unique_values.insert(rng.rand_long());
    }
    // Should have many unique values
    TEST_CHECK(unique_values.size() > 900);

    std::cout << "    PASSED" << std::endl;
}

void TestMtRandom_RandDouble() {
    std::cout << "  Testing mt_random rand_double()..." << std::endl;

    mt_random rng;
    rng.srand(42);

    // All values should be in [0, 1)
    for (int i = 0; i < 10000; i++) {
        double val = rng.rand_double();
        TEST_CHECK(val >= 0.0);
        TEST_CHECK(val < 1.0);
    }

    // Check distribution - should cover the range reasonably
    int bins[10] = {0};
    rng.srand(42);
    for (int i = 0; i < 10000; i++) {
        double val = rng.rand_double();
        int bin = static_cast<int>(val * 10);
        if (bin >= 10) bin = 9;
        bins[bin]++;
    }

    // Each bin should have roughly 1000 values (allowing for variance)
    for (int i = 0; i < 10; i++) {
        TEST_CHECK(bins[i] > 500);  // At least half expected
        TEST_CHECK(bins[i] < 1500); // At most 50% more than expected
    }

    std::cout << "    PASSED" << std::endl;
}

void TestMtRandom_RangedLong() {
    std::cout << "  Testing mt_random ranged_long()..." << std::endl;

    mt_random rng;
    rng.srand(42);

    // Test specific ranges
    for (int i = 0; i < 1000; i++) {
        uint32_t val = rng.ranged_long(10, 20);
        TEST_CHECK(val >= 10);
        TEST_CHECK(val <= 20);
    }

    // Test edge cases
    for (int i = 0; i < 100; i++) {
        uint32_t val = rng.ranged_long(5, 5);
        TEST_CHECK_EQ(val, 5u);  // min == max should return that value
    }

    // Test distribution
    std::map<uint32_t, int> counts;
    rng.srand(42);
    for (int i = 0; i < 11000; i++) {
        uint32_t val = rng.ranged_long(0, 10);
        counts[val]++;
    }
    // Each value 0-10 should appear roughly 1000 times
    for (uint32_t i = 0; i <= 10; i++) {
        TEST_CHECK(counts[i] > 500);
        TEST_CHECK(counts[i] < 1500);
    }

    std::cout << "    PASSED" << std::endl;
}

void TestMtRandom_ThreadSafety() {
    std::cout << "  Testing mt_random thread safety (separate instances)..." << std::endl;

    const int NUM_THREADS = 4;
    const int ITERATIONS = 10000;
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&errors, t, ITERATIONS]() {
            // Each thread has its own RNG instance
            mt_random rng;
            rng.srand(t * 1000);

            for (int i = 0; i < ITERATIONS; i++) {
                double val = rng.rand_double();
                if (val < 0.0 || val >= 1.0) {
                    errors++;
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    TEST_CHECK_EQ(errors.load(), 0);

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// HashTable Tests
// ============================================================================

// Simple test state for hash table
class TestState : public State {
public:
    uint32_t value;
    TestState(uint32_t v) : State(), value(v) {}
    unsigned long hash_key() override { return value; }
    bool equals(State* val) override {
        TestState* ts = dynamic_cast<TestState*>(val);
        return ts && ts->value == value;
    }
    int type() override { return 1; }
    void Print(int val = 0) const override {
        printf("TestState(%u)\n", value);
    }
};

void TestHashTable_BasicOperations() {
    std::cout << "  Testing HashTable basic operations..." << std::endl;

    HashTable ht(100);

    // Initially empty
    TEST_CHECK_EQ(ht.getNumElts(), 0);

    // Add elements
    ht.Add(new TestState(42));
    TEST_CHECK_EQ(ht.getNumElts(), 1);

    ht.Add(new TestState(123));
    ht.Add(new TestState(456));
    TEST_CHECK_EQ(ht.getNumElts(), 3);

    // Check if elements are in
    TestState query1(42);
    State* found = ht.IsIn(&query1);
    TEST_CHECK(found != nullptr);

    TestState query2(999);
    found = ht.IsIn(&query2);
    TEST_CHECK(found == nullptr);

    std::cout << "    PASSED" << std::endl;
}

void TestHashTable_Remove() {
    std::cout << "  Testing HashTable remove..." << std::endl;

    HashTable ht(100);

    ht.Add(new TestState(1));
    ht.Add(new TestState(2));
    ht.Add(new TestState(3));
    TEST_CHECK_EQ(ht.getNumElts(), 3);

    // Remove middle element
    TestState query(2);
    ht.Remove(&query);
    TEST_CHECK_EQ(ht.getNumElts(), 2);

    // Verify it's gone
    TEST_CHECK(ht.IsIn(&query) == nullptr);

    // Others still there
    TestState q1(1), q3(3);
    TEST_CHECK(ht.IsIn(&q1) != nullptr);
    TEST_CHECK(ht.IsIn(&q3) != nullptr);

    std::cout << "    PASSED" << std::endl;
}

void TestHashTable_Clear() {
    std::cout << "  Testing HashTable clear..." << std::endl;

    HashTable ht(100);

    for (int i = 0; i < 50; i++) {
        ht.Add(new TestState(i));
    }
    TEST_CHECK_EQ(ht.getNumElts(), 50);

    ht.Clear();
    TEST_CHECK_EQ(ht.getNumElts(), 0);

    // Verify elements are gone
    TestState query(25);
    TEST_CHECK(ht.IsIn(&query) == nullptr);

    std::cout << "    PASSED" << std::endl;
}

void TestHashTable_Iteration() {
    std::cout << "  Testing HashTable iteration..." << std::endl;

    HashTable ht(100);
    std::set<uint32_t> expected;

    for (uint32_t i = 0; i < 20; i++) {
        ht.Add(new TestState(i * 10));
        expected.insert(i * 10);
    }

    std::set<uint32_t> found;
    ht.iterReset();
    while (!ht.iterDone()) {
        State* s = ht.iterNext();
        TestState* ts = dynamic_cast<TestState*>(s);
        TEST_CHECK(ts != nullptr);
        found.insert(ts->value);
    }

    TEST_CHECK_EQ(found.size(), expected.size());
    TEST_CHECK(found == expected);

    std::cout << "    PASSED" << std::endl;
}

void TestHashTable_Collisions() {
    std::cout << "  Testing HashTable with collisions..." << std::endl;

    // Small table to force collisions
    HashTable ht(10);

    // Add many elements - will cause collisions
    for (int i = 0; i < 100; i++) {
        ht.Add(new TestState(i));
    }
    TEST_CHECK_EQ(ht.getNumElts(), 100);

    // All should be findable
    for (int i = 0; i < 100; i++) {
        TestState query(i);
        TEST_CHECK(ht.IsIn(&query) != nullptr);
    }

    std::cout << "    PASSED" << std::endl;
}

void TestHashTable_LargeScale() {
    std::cout << "  Testing HashTable large scale..." << std::endl;

    HashTable ht(10007);  // Prime size

    const int N = 5000;
    for (int i = 0; i < N; i++) {
        ht.Add(new TestState(i));
    }
    TEST_CHECK_EQ(ht.getNumElts(), N);

    // Random lookups
    mt_random rng;
    rng.srand(42);
    for (int i = 0; i < 1000; i++) {
        uint32_t val = rng.ranged_long(0, N - 1);
        TestState query(val);
        TEST_CHECK(ht.IsIn(&query) != nullptr);
    }

    // Lookups for non-existent values
    for (int i = N; i < N + 100; i++) {
        TestState query(i);
        TEST_CHECK(ht.IsIn(&query) == nullptr);
    }

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Timer Tests
// ============================================================================

void TestTimer_BasicTiming() {
    std::cout << "  Testing Timer basic timing..." << std::endl;

    Timer timer;

    timer.StartTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    double elapsed = timer.EndTimer();

    // Should be approximately 0.1 seconds (allow some variance)
    TEST_CHECK(elapsed >= 0.05);  // At least 50ms
    TEST_CHECK(elapsed < 0.5);    // Less than 500ms

    std::cout << "    PASSED (elapsed: " << elapsed << "s)" << std::endl;
}

void TestTimer_MultipleTimings() {
    std::cout << "  Testing Timer multiple timings..." << std::endl;

    Timer timer;

    // First timing
    timer.StartTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    double elapsed1 = timer.EndTimer();

    // Second timing (should reset)
    timer.StartTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    double elapsed2 = timer.EndTimer();

    // Second should be longer than first
    TEST_CHECK(elapsed2 > elapsed1);

    std::cout << "    PASSED" << std::endl;
}

void TestTimer_ShortDurations() {
    std::cout << "  Testing Timer short durations..." << std::endl;

    Timer timer;

    // Very short timing
    timer.StartTimer();
    // No sleep - measure overhead
    double elapsed = timer.EndTimer();

    // Should be very small but non-negative
    TEST_CHECK(elapsed >= 0.0);
    TEST_CHECK(elapsed < 0.1);  // Less than 100ms overhead

    std::cout << "    PASSED (overhead: " << elapsed * 1000 << "ms)" << std::endl;
}

void TestTimer_GetElapsedTime() {
    std::cout << "  Testing Timer getElapsedTime()..." << std::endl;

    Timer timer;

    timer.StartTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    double elapsed = timer.EndTimer();

    // GetElapsedTime should return last measured value
    double stored = timer.GetElapsedTime();
    TEST_CHECK(fequal(elapsed, stored) || std::abs(elapsed - stored) < 0.001);

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// State Base Class Tests
// ============================================================================

void TestState_CreationCounter() {
    std::cout << "  Testing State creation counter..." << std::endl;

    int startCount = creationCounter;

    TestState* s1 = new TestState(1);
    TestState* s2 = new TestState(2);
    TestState* s3 = new TestState(3);

    // Each state should have unique nodeNum
    TEST_CHECK(s1->nodeNum < s2->nodeNum);
    TEST_CHECK(s2->nodeNum < s3->nodeNum);

    // Counter should have increased
    TEST_CHECK(creationCounter >= startCount + 3);

    delete s1;
    delete s2;
    delete s3;

    std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Main Test Runner
// ============================================================================

void RunAllTests() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Utility Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << std::endl << "fpUtil Tests:" << std::endl;
    TestFpUtil_Fless();
    TestFpUtil_Fgreater();
    TestFpUtil_Fequal();
    TestFpUtil_EdgeCases();

    std::cout << std::endl << "mt_random Tests:" << std::endl;
    TestMtRandom_Seeding();
    TestMtRandom_RandLong();
    TestMtRandom_RandDouble();
    TestMtRandom_RangedLong();
    TestMtRandom_ThreadSafety();

    std::cout << std::endl << "HashTable Tests:" << std::endl;
    TestHashTable_BasicOperations();
    TestHashTable_Remove();
    TestHashTable_Clear();
    TestHashTable_Iteration();
    TestHashTable_Collisions();
    TestHashTable_LargeScale();

    std::cout << std::endl << "Timer Tests:" << std::endl;
    TestTimer_BasicTiming();
    TestTimer_MultipleTimings();
    TestTimer_ShortDurations();
    TestTimer_GetElapsedTime();

    std::cout << std::endl << "State Tests:" << std::endl;
    TestState_CreationCounter();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "All Utility Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
}

}  // namespace
}  // namespace hearts

int main(int argc, char** argv) {
    hearts::RunAllTests();
    return 0;
}
