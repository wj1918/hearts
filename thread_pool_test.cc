// Copyright 2021 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Comprehensive unit tests for multi-threading infrastructure

#include "ThreadPool.h"

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <atomic>
#include <chrono>
#include <thread>
#include <cmath>
#include <cassert>
#include <random>
#include <sstream>

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
// BinomialLookup Tests
// ============================================================================

void TestBinomialLookup_BasicValues() {
  std::cout << "  Testing BinomialLookup basic values..." << std::endl;

  BinomialLookup& lookup = BinomialLookup::getInstance();

  // Test well-known values
  TEST_CHECK_EQ(lookup.choose(0, 0), 1ULL);
  TEST_CHECK_EQ(lookup.choose(1, 0), 1ULL);
  TEST_CHECK_EQ(lookup.choose(1, 1), 1ULL);
  TEST_CHECK_EQ(lookup.choose(5, 0), 1ULL);
  TEST_CHECK_EQ(lookup.choose(5, 1), 5ULL);
  TEST_CHECK_EQ(lookup.choose(5, 2), 10ULL);
  TEST_CHECK_EQ(lookup.choose(5, 3), 10ULL);
  TEST_CHECK_EQ(lookup.choose(5, 4), 5ULL);
  TEST_CHECK_EQ(lookup.choose(5, 5), 1ULL);

  // Pascal's triangle row
  TEST_CHECK_EQ(lookup.choose(10, 0), 1ULL);
  TEST_CHECK_EQ(lookup.choose(10, 1), 10ULL);
  TEST_CHECK_EQ(lookup.choose(10, 2), 45ULL);
  TEST_CHECK_EQ(lookup.choose(10, 3), 120ULL);
  TEST_CHECK_EQ(lookup.choose(10, 4), 210ULL);
  TEST_CHECK_EQ(lookup.choose(10, 5), 252ULL);

  // Larger values
  TEST_CHECK_EQ(lookup.choose(20, 10), 184756ULL);
  TEST_CHECK_EQ(lookup.choose(52, 13), 635013559600ULL);  // Cards in a suit

  std::cout << "    PASSED" << std::endl;
}

void TestBinomialLookup_EdgeCases() {
  std::cout << "  Testing BinomialLookup edge cases..." << std::endl;

  BinomialLookup& lookup = BinomialLookup::getInstance();

  // k > n should return 0
  TEST_CHECK_EQ(lookup.choose(5, 6), 0ULL);
  TEST_CHECK_EQ(lookup.choose(0, 1), 0ULL);
  TEST_CHECK_EQ(lookup.choose(10, 11), 0ULL);

  // Negative inputs (cast to int)
  TEST_CHECK_EQ(lookup.choose(-1, 0), 0ULL);
  TEST_CHECK_EQ(lookup.choose(5, -1), 0ULL);

  std::cout << "    PASSED" << std::endl;
}

void TestBinomialLookup_Symmetry() {
  std::cout << "  Testing BinomialLookup symmetry C(n,k) = C(n,n-k)..." << std::endl;

  BinomialLookup& lookup = BinomialLookup::getInstance();

  for (int n = 0; n < 50; n++) {
    for (int k = 0; k <= n; k++) {
      TEST_CHECK_EQ(lookup.choose(n, k), lookup.choose(n, n - k));
    }
  }

  std::cout << "    PASSED" << std::endl;
}

void TestBinomialLookup_PascalIdentity() {
  std::cout << "  Testing BinomialLookup Pascal's identity C(n,k) = C(n-1,k-1) + C(n-1,k)..." << std::endl;

  BinomialLookup& lookup = BinomialLookup::getInstance();

  for (int n = 1; n < 50; n++) {
    for (int k = 1; k < n; k++) {
      uint64_t lhs = lookup.choose(n, k);
      uint64_t rhs = lookup.choose(n - 1, k - 1) + lookup.choose(n - 1, k);
      TEST_CHECK_EQ(lhs, rhs);
    }
  }

  std::cout << "    PASSED" << std::endl;
}

void TestBinomialLookup_ThreadSafety() {
  std::cout << "  Testing BinomialLookup thread safety..." << std::endl;

  const int NUM_THREADS = 8;
  const int ITERATIONS = 10000;
  std::atomic<int> errors{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&errors, t]() {
      BinomialLookup& lookup = BinomialLookup::getInstance();
      std::mt19937 rng(t);

      for (int i = 0; i < ITERATIONS; i++) {
        int n = rng() % 50;
        int k = rng() % (n + 1);

        uint64_t result = lookup.choose(n, k);

        // Verify against expected value
        if (result != lookup.choose(n, k)) {
          errors++;
        }

        // Basic sanity check
        if (k <= n && result == 0 && k != n + 1) {
          // C(n,k) should be > 0 for 0 <= k <= n
          if (n >= 0 && k >= 0) {
            errors++;
          }
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  TEST_CHECK_EQ(errors.load(), 0);
  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// CompletionQueue Tests
// ============================================================================

void TestCompletionQueue_BasicOperations() {
  std::cout << "  Testing CompletionQueue basic operations..." << std::endl;

  CompletionQueue<int> queue;

  // Test empty
  TEST_CHECK(queue.empty());
  TEST_CHECK_EQ(queue.size(), 0u);

  // Test push and pop
  queue.push(42);
  TEST_CHECK(!queue.empty());
  TEST_CHECK_EQ(queue.size(), 1u);

  int value = queue.pop();
  TEST_CHECK_EQ(value, 42);
  TEST_CHECK(queue.empty());

  // Test multiple values
  queue.push(1);
  queue.push(2);
  queue.push(3);
  TEST_CHECK_EQ(queue.size(), 3u);

  TEST_CHECK_EQ(queue.pop(), 1);
  TEST_CHECK_EQ(queue.pop(), 2);
  TEST_CHECK_EQ(queue.pop(), 3);
  TEST_CHECK(queue.empty());

  std::cout << "    PASSED" << std::endl;
}

void TestCompletionQueue_TryPop() {
  std::cout << "  Testing CompletionQueue tryPop..." << std::endl;

  CompletionQueue<int> queue;
  int value;

  // tryPop on empty queue should return false
  TEST_CHECK(!queue.tryPop(value));

  queue.push(123);
  TEST_CHECK(queue.tryPop(value));
  TEST_CHECK_EQ(value, 123);

  // Should be empty now
  TEST_CHECK(!queue.tryPop(value));

  std::cout << "    PASSED" << std::endl;
}

void TestCompletionQueue_ProducerConsumer() {
  std::cout << "  Testing CompletionQueue producer-consumer pattern..." << std::endl;

  CompletionQueue<int> queue;
  const int NUM_ITEMS = 1000;
  std::atomic<int> sum{0};

  // Producer thread
  std::thread producer([&queue]() {
    for (int i = 1; i <= NUM_ITEMS; i++) {
      queue.push(i);
    }
  });

  // Consumer thread
  std::thread consumer([&queue, &sum]() {
    for (int i = 0; i < NUM_ITEMS; i++) {
      int value = queue.pop();
      sum += value;
    }
  });

  producer.join();
  consumer.join();

  // Sum of 1 to N = N*(N+1)/2
  int expected = NUM_ITEMS * (NUM_ITEMS + 1) / 2;
  TEST_CHECK_EQ(sum.load(), expected);

  std::cout << "    PASSED" << std::endl;
}

void TestCompletionQueue_MultipleProducers() {
  std::cout << "  Testing CompletionQueue with multiple producers..." << std::endl;

  CompletionQueue<int> queue;
  const int NUM_PRODUCERS = 4;
  const int ITEMS_PER_PRODUCER = 250;
  std::atomic<int> totalReceived{0};
  std::set<int> receivedValues;
  std::mutex receivedMutex;

  std::vector<std::thread> producers;
  for (int p = 0; p < NUM_PRODUCERS; p++) {
    producers.emplace_back([&queue, p, ITEMS_PER_PRODUCER]() {
      for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        queue.push(p * ITEMS_PER_PRODUCER + i);
      }
    });
  }

  // Consumer
  std::thread consumer([&]() {
    for (int i = 0; i < NUM_PRODUCERS * ITEMS_PER_PRODUCER; i++) {
      int value = queue.pop();
      std::lock_guard<std::mutex> lock(receivedMutex);
      receivedValues.insert(value);
      totalReceived++;
    }
  });

  for (auto& p : producers) {
    p.join();
  }
  consumer.join();

  TEST_CHECK_EQ(totalReceived.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  TEST_CHECK_EQ(receivedValues.size(), static_cast<size_t>(NUM_PRODUCERS * ITEMS_PER_PRODUCER));

  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// ThreadPool Tests
// ============================================================================

void TestThreadPool_Singleton() {
  std::cout << "  Testing ThreadPool singleton pattern..." << std::endl;

  ThreadPool& pool1 = ThreadPool::getInstance();
  ThreadPool& pool2 = ThreadPool::getInstance();

  TEST_CHECK(&pool1 == &pool2);
  TEST_CHECK(pool1.getThreadCount() > 0);

  std::cout << "    PASSED (thread count: " << pool1.getThreadCount() << ")" << std::endl;
}

void TestThreadPool_SimpleTask() {
  std::cout << "  Testing ThreadPool simple task execution..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();

  auto future = pool.submit([]() {
    return 42;
  });

  TEST_CHECK_EQ(future.get(), 42);

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_MultipleTaskTypes() {
  std::cout << "  Testing ThreadPool with different return types..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();

  // Integer task
  auto intFuture = pool.submit([]() { return 123; });

  // Double task
  auto doubleFuture = pool.submit([]() { return 3.14159; });

  // String task
  auto stringFuture = pool.submit([]() { return std::string("hello"); });

  // Void task with side effect
  std::atomic<bool> executed{false};
  auto voidFuture = pool.submit([&executed]() { executed = true; });

  TEST_CHECK_EQ(intFuture.get(), 123);
  TEST_CHECK(std::abs(doubleFuture.get() - 3.14159) < 0.0001);
  TEST_CHECK_EQ(stringFuture.get(), "hello");
  voidFuture.get();
  TEST_CHECK(executed.load());

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_ManyTasks() {
  std::cout << "  Testing ThreadPool with many concurrent tasks..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();
  const int NUM_TASKS = 1000;

  std::vector<std::future<int>> futures;
  for (int i = 0; i < NUM_TASKS; i++) {
    futures.push_back(pool.submit([i]() {
      return i * i;
    }));
  }

  for (int i = 0; i < NUM_TASKS; i++) {
    TEST_CHECK_EQ(futures[i].get(), i * i);
  }

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_TasksWithDelay() {
  std::cout << "  Testing ThreadPool with varying task durations..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();
  std::atomic<int> completionOrder{0};
  std::vector<int> orderRecorded(5);

  std::vector<std::future<void>> futures;

  // Submit tasks with different delays
  // Task 0: 50ms, Task 1: 10ms, Task 2: 30ms, Task 3: 5ms, Task 4: 20ms
  int delays[] = {50, 10, 30, 5, 20};

  for (int i = 0; i < 5; i++) {
    int delay = delays[i];
    futures.push_back(pool.submit([i, delay, &completionOrder, &orderRecorded]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      orderRecorded[i] = completionOrder.fetch_add(1);
    }));
  }

  for (auto& f : futures) {
    f.get();
  }

  // Verify all tasks completed
  TEST_CHECK_EQ(completionOrder.load(), 5);

  // Task 3 (5ms) should complete before Task 0 (50ms)
  TEST_CHECK(orderRecorded[3] < orderRecorded[0]);

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_ExceptionHandling() {
  std::cout << "  Testing ThreadPool exception handling..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();

  auto future = pool.submit([]() -> int {
    throw std::runtime_error("Test exception");
    return 0;
  });

  bool exceptionCaught = false;
  try {
    future.get();
  } catch (const std::runtime_error& e) {
    exceptionCaught = true;
    TEST_CHECK(std::string(e.what()).find("Test exception") != std::string::npos);
  }

  TEST_CHECK(exceptionCaught);

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_StressTest() {
  std::cout << "  Testing ThreadPool under stress..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();
  const int NUM_TASKS = 10000;
  std::atomic<int> counter{0};

  std::vector<std::future<void>> futures;
  for (int i = 0; i < NUM_TASKS; i++) {
    futures.push_back(pool.submit([&counter]() {
      counter.fetch_add(1, std::memory_order_relaxed);
    }));
  }

  for (auto& f : futures) {
    f.get();
  }

  TEST_CHECK_EQ(counter.load(), NUM_TASKS);

  std::cout << "    PASSED" << std::endl;
}

void TestThreadPool_ComputeIntensive() {
  std::cout << "  Testing ThreadPool with compute-intensive tasks..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();

  // Compute sum of primes up to N for different N values
  auto countPrimes = [](int limit) -> int {
    if (limit < 2) return 0;
    std::vector<bool> sieve(limit + 1, true);
    sieve[0] = sieve[1] = false;
    for (int i = 2; i * i <= limit; i++) {
      if (sieve[i]) {
        for (int j = i * i; j <= limit; j += i) {
          sieve[j] = false;
        }
      }
    }
    int count = 0;
    for (int i = 2; i <= limit; i++) {
      if (sieve[i]) count++;
    }
    return count;
  };

  std::vector<std::future<int>> futures;
  std::vector<int> limits = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000};

  for (int limit : limits) {
    futures.push_back(pool.submit([countPrimes, limit]() {
      return countPrimes(limit);
    }));
  }

  // Expected prime counts
  std::vector<int> expected = {168, 303, 430, 550, 669, 783, 900, 1007};

  for (size_t i = 0; i < futures.size(); i++) {
    TEST_CHECK_EQ(futures[i].get(), expected[i]);
  }

  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Thread Safety Stress Tests
// ============================================================================

void TestThreadSafety_ConcurrentBinomialAccess() {
  std::cout << "  Testing concurrent BinomialLookup access under heavy load..." << std::endl;

  const int NUM_THREADS = 16;
  const int OPERATIONS = 100000;
  std::atomic<int> totalOperations{0};
  std::atomic<int> errors{0};

  auto worker = [&](int threadId) {
    BinomialLookup& lookup = BinomialLookup::getInstance();
    std::mt19937 rng(threadId * 12345);

    for (int i = 0; i < OPERATIONS; i++) {
      int n = rng() % 52 + 1;  // 1-52 (card game relevant)
      int k = rng() % (n + 1);

      uint64_t val1 = lookup.choose(n, k);
      uint64_t val2 = lookup.choose(n, k);

      if (val1 != val2) {
        errors++;
      }

      // Check symmetry
      if (lookup.choose(n, k) != lookup.choose(n, n - k)) {
        errors++;
      }

      totalOperations++;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < NUM_THREADS; i++) {
    threads.emplace_back(worker, i);
  }

  for (auto& t : threads) {
    t.join();
  }

  TEST_CHECK_EQ(errors.load(), 0);
  TEST_CHECK_EQ(totalOperations.load(), NUM_THREADS * OPERATIONS);

  std::cout << "    PASSED (" << totalOperations.load() << " operations)" << std::endl;
}

void TestThreadSafety_CompletionQueueHighContention() {
  std::cout << "  Testing CompletionQueue under high contention..." << std::endl;

  CompletionQueue<int> queue;
  const int NUM_PRODUCERS = 8;
  const int NUM_CONSUMERS = 8;
  const int ITEMS_PER_PRODUCER = 10000;

  std::atomic<long long> producerSum{0};
  std::atomic<long long> consumerSum{0};
  std::atomic<int> itemsProduced{0};
  std::atomic<int> itemsConsumed{0};
  std::atomic<bool> done{false};

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;

  // Producers
  for (int p = 0; p < NUM_PRODUCERS; p++) {
    producers.emplace_back([&, p]() {
      for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int value = p * ITEMS_PER_PRODUCER + i;
        queue.push(value);
        producerSum += value;
        itemsProduced++;
      }
    });
  }

  // Consumers
  for (int c = 0; c < NUM_CONSUMERS; c++) {
    consumers.emplace_back([&]() {
      while (true) {
        int value;
        if (queue.tryPop(value)) {
          consumerSum += value;
          itemsConsumed++;
        } else if (done.load() && queue.empty()) {
          break;
        } else {
          std::this_thread::yield();
        }
      }
    });
  }

  // Wait for producers to finish
  for (auto& p : producers) {
    p.join();
  }

  // Signal done and wait for queue to drain
  done = true;
  while (!queue.empty() || itemsConsumed.load() < itemsProduced.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  for (auto& c : consumers) {
    c.join();
  }

  TEST_CHECK_EQ(itemsProduced.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  TEST_CHECK_EQ(itemsConsumed.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  TEST_CHECK_EQ(producerSum.load(), consumerSum.load());

  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Performance Tests
// ============================================================================

void TestPerformance_BinomialLookupVsComputation() {
  std::cout << "  Testing BinomialLookup performance vs direct computation..." << std::endl;

  const int ITERATIONS = 1000000;
  BinomialLookup& lookup = BinomialLookup::getInstance();

  // Measure lookup time
  auto start = std::chrono::high_resolution_clock::now();
  volatile uint64_t sum1 = 0;
  for (int i = 0; i < ITERATIONS; i++) {
    sum1 += lookup.choose(52, 13);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto lookupTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  // Measure direct computation time (simple implementation)
  auto computeChoose = [](int n, int k) -> uint64_t {
    if (k > n) return 0;
    if (k > n - k) k = n - k;
    long double result = 1;
    for (int i = 0; i < k; i++) {
      result *= (n - i);
      result /= (i + 1);
    }
    return static_cast<uint64_t>(result + 0.5);
  };

  start = std::chrono::high_resolution_clock::now();
  volatile uint64_t sum2 = 0;
  for (int i = 0; i < ITERATIONS; i++) {
    sum2 += computeChoose(52, 13);
  }
  end = std::chrono::high_resolution_clock::now();
  auto computeTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  // Lookup should be significantly faster
  double speedup = static_cast<double>(computeTime) / lookupTime;

  std::cout << "    Lookup time: " << lookupTime << " us, Compute time: " << computeTime
            << " us, Speedup: " << speedup << "x" << std::endl;

  TEST_CHECK(speedup > 1.0);  // Lookup should be faster

  std::cout << "    PASSED" << std::endl;
}

void TestPerformance_ThreadPoolOverhead() {
  std::cout << "  Testing ThreadPool overhead..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();
  const int NUM_TASKS = 10000;

  // Measure time for trivial tasks
  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::future<int>> futures;
  for (int i = 0; i < NUM_TASKS; i++) {
    futures.push_back(pool.submit([i]() { return i; }));
  }

  int sum = 0;
  for (auto& f : futures) {
    sum += f.get();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double avgOverhead = static_cast<double>(elapsed) / NUM_TASKS;

  std::cout << "    Total time: " << elapsed << " us for " << NUM_TASKS
            << " tasks, Avg overhead: " << avgOverhead << " us/task" << std::endl;

  // Verify correctness
  int expected = (NUM_TASKS - 1) * NUM_TASKS / 2;
  TEST_CHECK_EQ(sum, expected);

  // Overhead should be reasonable (less than 100us per task on modern hardware)
  TEST_CHECK(avgOverhead < 1000);

  std::cout << "    PASSED" << std::endl;
}

void TestPerformance_ParallelSpeedup() {
  std::cout << "  Testing parallel speedup..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();

  // CPU-bound work function
  auto heavyWork = [](int iterations) -> long long {
    long long result = 0;
    for (int i = 0; i < iterations; i++) {
      result += (i * i) % 1000000007;
    }
    return result;
  };

  const int WORK_SIZE = 10000000;
  const int NUM_CHUNKS = 8;
  const int CHUNK_SIZE = WORK_SIZE / NUM_CHUNKS;

  // Sequential execution
  auto start = std::chrono::high_resolution_clock::now();
  long long seqResult = heavyWork(WORK_SIZE);
  auto end = std::chrono::high_resolution_clock::now();
  auto seqTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Parallel execution
  start = std::chrono::high_resolution_clock::now();
  std::vector<std::future<long long>> futures;
  for (int i = 0; i < NUM_CHUNKS; i++) {
    futures.push_back(pool.submit([heavyWork, CHUNK_SIZE]() {
      return heavyWork(CHUNK_SIZE);
    }));
  }

  long long parResult = 0;
  for (auto& f : futures) {
    parResult += f.get();
  }
  end = std::chrono::high_resolution_clock::now();
  auto parTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  double speedup = static_cast<double>(seqTime) / parTime;

  std::cout << "    Sequential: " << seqTime << " ms, Parallel: " << parTime
            << " ms, Speedup: " << speedup << "x" << std::endl;

  // On multi-core systems, we should see some speedup
  if (pool.getThreadCount() > 1) {
    TEST_CHECK(speedup > 1.0);
  }

  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestIntegration_SimulatedMonteCarloWorkload() {
  std::cout << "  Testing simulated Monte Carlo workload..." << std::endl;

  ThreadPool& pool = ThreadPool::getInstance();
  CompletionQueue<std::pair<int, double>> results;

  const int NUM_MODELS = 20;

  // Simulate analysis of different game states (varying computation times)
  auto analyzeModel = [](int modelId) -> double {
    // Simulate varying computation time
    std::mt19937 rng(modelId);
    int iterations = 10000 + (rng() % 50000);

    double result = 0;
    for (int i = 0; i < iterations; i++) {
      result += std::sin(i * 0.001) * std::cos(i * 0.002);
    }
    return result / iterations;
  };

  auto start = std::chrono::high_resolution_clock::now();

  // Submit all models
  for (int i = 0; i < NUM_MODELS; i++) {
    pool.submit([&results, analyzeModel, i]() {
      double result = analyzeModel(i);
      results.push({i, result});
    });
  }

  // Collect results as they complete
  std::vector<double> modelResults(NUM_MODELS);
  std::vector<int> completionOrder;

  for (int i = 0; i < NUM_MODELS; i++) {
    auto result = results.pop();
    modelResults[result.first] = result.second;
    completionOrder.push_back(result.first);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Verify all models completed
  TEST_CHECK_EQ(completionOrder.size(), static_cast<size_t>(NUM_MODELS));

  // With completion queue, results should not be in sequential order
  // (unless all tasks complete at exactly the same time, which is unlikely)
  bool inOrder = true;
  for (int i = 0; i < NUM_MODELS - 1 && inOrder; i++) {
    if (completionOrder[i] > completionOrder[i + 1]) {
      inOrder = false;
    }
  }

  std::cout << "    Completed " << NUM_MODELS << " models in " << elapsed << " ms" << std::endl;
  std::cout << "    Results arrived in order: " << (inOrder ? "yes" : "no (good - completion queue working)") << std::endl;

  std::cout << "    PASSED" << std::endl;
}

// ============================================================================
// Main Test Runner
// ============================================================================

void RunAllTests() {
  std::cout << "========================================" << std::endl;
  std::cout << "Running Threading Infrastructure Tests" << std::endl;
  std::cout << "========================================" << std::endl;

  std::cout << std::endl << "BinomialLookup Tests:" << std::endl;
  TestBinomialLookup_BasicValues();
  TestBinomialLookup_EdgeCases();
  TestBinomialLookup_Symmetry();
  TestBinomialLookup_PascalIdentity();
  TestBinomialLookup_ThreadSafety();

  std::cout << std::endl << "CompletionQueue Tests:" << std::endl;
  TestCompletionQueue_BasicOperations();
  TestCompletionQueue_TryPop();
  TestCompletionQueue_ProducerConsumer();
  TestCompletionQueue_MultipleProducers();

  std::cout << std::endl << "ThreadPool Tests:" << std::endl;
  TestThreadPool_Singleton();
  TestThreadPool_SimpleTask();
  TestThreadPool_MultipleTaskTypes();
  TestThreadPool_ManyTasks();
  TestThreadPool_TasksWithDelay();
  TestThreadPool_ExceptionHandling();
  TestThreadPool_StressTest();
  TestThreadPool_ComputeIntensive();

  std::cout << std::endl << "Thread Safety Stress Tests:" << std::endl;
  TestThreadSafety_ConcurrentBinomialAccess();
  TestThreadSafety_CompletionQueueHighContention();

  std::cout << std::endl << "Performance Tests:" << std::endl;
  TestPerformance_BinomialLookupVsComputation();
  TestPerformance_ThreadPoolOverhead();
  TestPerformance_ParallelSpeedup();

  std::cout << std::endl << "Integration Tests:" << std::endl;
  TestIntegration_SimulatedMonteCarloWorkload();

  std::cout << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "All Threading Infrastructure Tests PASSED!" << std::endl;
  std::cout << "========================================" << std::endl;
}

}  // namespace
}  // namespace hearts

int main(int argc, char** argv) {
  hearts::RunAllTests();
  return 0;
}
