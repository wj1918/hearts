// Performance benchmark: Single-threaded vs Multi-threaded PIMC
// Compares execution time for Hearts AI decision making

#include "Player.h"
#include "Game.h"
#include "Hearts.h"
#include "iiMonteCarlo.h"
#include "Timer.h"
#include "ThreadPool.h"

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

using namespace hearts;

struct BenchmarkResult {
    double avgTime;
    double minTime;
    double maxTime;
    double stdDev;
    int numDecisions;
};

// Create a player with specified threading mode
Player* CreateBenchmarkPlayer(bool useThreads, int sims, int worlds) {
    double C = 0.4;
    SimpleHeartsPlayer *p;
    iiMonteCarlo *a;
    UCT *b;

    p = new SafeSimpleHeartsPlayer(a = new iiMonteCarlo(b = new UCT(sims/worlds, C), worlds));
    p->setModelLevel(2);
    b->setPlayoutModule(new HeartsPlayout());
    a->setUseThreads(useThreads);
    b->setEpsilonPlayout(0.1);

    return p;
}

// Calculate statistics from timing data
BenchmarkResult CalculateStats(const std::vector<double>& times) {
    BenchmarkResult result;
    result.numDecisions = times.size();

    if (times.empty()) {
        result.avgTime = result.minTime = result.maxTime = result.stdDev = 0;
        return result;
    }

    result.minTime = *std::min_element(times.begin(), times.end());
    result.maxTime = *std::max_element(times.begin(), times.end());
    result.avgTime = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    double variance = 0;
    for (double t : times) {
        variance += (t - result.avgTime) * (t - result.avgTime);
    }
    result.stdDev = std::sqrt(variance / times.size());

    return result;
}

// Run benchmark with specified configuration
BenchmarkResult RunBenchmark(bool useThreads, int sims, int worlds, int numDecisions, int seed) {
    std::vector<double> times;
    times.reserve(numDecisions);

    int rules = kQueenPenalty | kMustBreakHearts | kQueenBreaksHearts |
                kDoPassCards | kNoQueenFirstTrick | kNoHeartsFirstTrick | kLeadClubs;

    std::cout << "  Running " << numDecisions << " decisions..." << std::flush;

    for (int i = 0; i < numDecisions; i++) {
        // Create fresh game state for each decision
        HeartsGameState* gs = new HeartsGameState(seed + i);
        gs->setRules(rules);

        HeartsCardGame game(gs);

        // Create AI player (player 0) and dummy players
        Player* aiPlayer = CreateBenchmarkPlayer(useThreads, sims, worlds);
        game.addPlayer(aiPlayer);
        game.addPlayer(new HeartsDucker());
        game.addPlayer(new HeartsDucker());
        game.addPlayer(new HeartsDucker());

        gs->setPassDir(kHold);  // No passing for benchmark simplicity
        gs->DealCards();

        // Time a single AI decision
        Timer timer;
        timer.StartTimer();
        Move* move = aiPlayer->Play();
        timer.EndTimer();

        times.push_back(timer.GetElapsedTime());

        // Cleanup
        delete move;
        delete aiPlayer->getAlgorithm();
        gs->deletePlayers();
        delete gs;

        if ((i + 1) % 5 == 0) {
            std::cout << "." << std::flush;
        }
    }
    std::cout << " done" << std::endl;

    return CalculateStats(times);
}

void PrintResult(const char* name, const BenchmarkResult& result) {
    std::cout << "  " << name << ":" << std::endl;
    std::cout << "    Decisions: " << result.numDecisions << std::endl;
    std::cout << "    Avg time:  " << (result.avgTime * 1000) << " ms" << std::endl;
    std::cout << "    Min time:  " << (result.minTime * 1000) << " ms" << std::endl;
    std::cout << "    Max time:  " << (result.maxTime * 1000) << " ms" << std::endl;
    std::cout << "    Std dev:   " << (result.stdDev * 1000) << " ms" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "============================================" << std::endl;
    std::cout << "Hearts AI Performance Benchmark" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // Configuration
    int sims = 3000;      // Total simulations per decision
    int worlds = 20;      // Number of sampled worlds (threads in MT mode)
    int numDecisions = 10; // Number of decisions to benchmark
    int seed = 12345;

    // Parse command line args
    if (argc > 1) numDecisions = std::atoi(argv[1]);
    if (argc > 2) sims = std::atoi(argv[2]);
    if (argc > 3) worlds = std::atoi(argv[3]);

    std::cout << "Configuration:" << std::endl;
    std::cout << "  Simulations per decision: " << sims << std::endl;
    std::cout << "  Sampled worlds: " << worlds << std::endl;
    std::cout << "  Sims per world: " << (sims / worlds) << std::endl;
    std::cout << "  Decisions to benchmark: " << numDecisions << std::endl;
    std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << std::endl;

    // Run single-threaded benchmark
    std::cout << "Running SINGLE-THREADED benchmark..." << std::endl;
    BenchmarkResult singleResult = RunBenchmark(false, sims, worlds, numDecisions, seed);
    PrintResult("Single-threaded", singleResult);
    std::cout << std::endl;

    // Run multi-threaded benchmark
    std::cout << "Running MULTI-THREADED benchmark..." << std::endl;
    BenchmarkResult multiResult = RunBenchmark(true, sims, worlds, numDecisions, seed);
    PrintResult("Multi-threaded", multiResult);
    std::cout << std::endl;

    // Compare results
    std::cout << "============================================" << std::endl;
    std::cout << "COMPARISON" << std::endl;
    std::cout << "============================================" << std::endl;
    double speedup = singleResult.avgTime / multiResult.avgTime;
    std::cout << "  Speedup: " << speedup << "x" << std::endl;
    std::cout << "  Single-threaded avg: " << (singleResult.avgTime * 1000) << " ms" << std::endl;
    std::cout << "  Multi-threaded avg:  " << (multiResult.avgTime * 1000) << " ms" << std::endl;
    std::cout << "  Time saved per decision: " << ((singleResult.avgTime - multiResult.avgTime) * 1000) << " ms" << std::endl;
    std::cout << std::endl;

    if (speedup > 1.0) {
        std::cout << "  Result: Multi-threaded is FASTER" << std::endl;
    } else if (speedup < 1.0) {
        std::cout << "  Result: Single-threaded is FASTER (threading overhead)" << std::endl;
    } else {
        std::cout << "  Result: No significant difference" << std::endl;
    }

    unsigned int hwThreads = std::thread::hardware_concurrency();
    if (hwThreads > 0) {
        std::cout << std::endl;
        std::cout << "Efficiency: " << (speedup / hwThreads * 100) << "% of ideal linear scaling" << std::endl;
    }

    return 0;
}
