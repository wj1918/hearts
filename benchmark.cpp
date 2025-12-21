/**
 * Performance Benchmark: Single-threaded vs Multi-threaded iiMonteCarlo
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

#include "Hearts.h"
#include "UCT.h"
#include "iiMonteCarlo.h"

using namespace hearts;

struct BenchmarkResult {
    int numWorlds;
    int numSimulations;
    double singleThreadMs;
    double multiThreadMs;
    double speedup;
};

BenchmarkResult runBenchmark(int numWorlds, int simsPerWorld, int numRuns = 5)
{
    BenchmarkResult result;
    result.numWorlds = numWorlds;
    result.numSimulations = simsPerWorld;

    double totalSingleThread = 0;
    double totalMultiThread = 0;

    for (int run = 0; run < numRuns; run++)
    {
        int seed = 12345 + run;

        // Single-threaded test
        {
            srand(seed);
            HeartsGameState *g = new HeartsGameState(seed);
            HeartsCardGame game(g);

            UCT *uct = new UCT(simsPerWorld, 0.4);
            uct->setPlayoutModule(new HeartsPlayout());

            iiMonteCarlo *iimc = new iiMonteCarlo(uct, numWorlds);
            iimc->setUseThreads(false);

            SimpleHeartsPlayer *player = new SimpleHeartsPlayer(iimc);
            player->setModelLevel(1);

            game.addPlayer(player);
            game.addPlayer(new HeartsDucker());
            game.addPlayer(new HeartsDucker());
            game.addPlayer(new HeartsDucker());

            g->Reset();
            g->setPassDir(kHold);

            auto start = std::chrono::high_resolution_clock::now();
            player->Play();
            auto end = std::chrono::high_resolution_clock::now();

            totalSingleThread += std::chrono::duration<double, std::milli>(end - start).count();
        }

        // Multi-threaded test
        {
            srand(seed);
            HeartsGameState *g = new HeartsGameState(seed);
            HeartsCardGame game(g);

            UCT *uct = new UCT(simsPerWorld, 0.4);
            uct->setPlayoutModule(new HeartsPlayout());

            iiMonteCarlo *iimc = new iiMonteCarlo(uct, numWorlds);
            iimc->setUseThreads(true);

            SimpleHeartsPlayer *player = new SimpleHeartsPlayer(iimc);
            player->setModelLevel(1);

            game.addPlayer(player);
            game.addPlayer(new HeartsDucker());
            game.addPlayer(new HeartsDucker());
            game.addPlayer(new HeartsDucker());

            g->Reset();
            g->setPassDir(kHold);

            auto start = std::chrono::high_resolution_clock::now();
            player->Play();
            auto end = std::chrono::high_resolution_clock::now();

            totalMultiThread += std::chrono::duration<double, std::milli>(end - start).count();
        }
    }

    result.singleThreadMs = totalSingleThread / numRuns;
    result.multiThreadMs = totalMultiThread / numRuns;
    result.speedup = result.singleThreadMs / result.multiThreadMs;

    return result;
}

int main(int argc, char **argv)
{
    unsigned int numCPU = std::thread::hardware_concurrency();

    std::cout << "========================================" << std::endl;
    std::cout << "iiMonteCarlo Threading Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Detected CPUs: " << numCPU << std::endl;
    std::cout << std::endl;

    std::vector<BenchmarkResult> results;

    // Test different configurations
    std::cout << "Running benchmarks (5 runs each, averaged)..." << std::endl;
    std::cout << std::endl;

    // Configuration: worlds x simulations per world
    std::vector<std::pair<int, int>> configs = {
        {4, 50},      // Light: 4 worlds, 50 sims each = 200 total
        {10, 100},    // Medium: 10 worlds, 100 sims each = 1000 total
        {20, 200},    // Heavy: 20 worlds, 200 sims each = 4000 total
        {30, 333},    // Production: 30 worlds, 333 sims each = ~10000 total
    };

    for (const auto& config : configs)
    {
        std::cout << "Testing " << config.first << " worlds x "
                  << config.second << " sims... " << std::flush;
        BenchmarkResult r = runBenchmark(config.first, config.second);
        results.push_back(r);
        std::cout << "done" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << std::left << std::setw(20) << "Configuration"
              << std::right << std::setw(15) << "Single (ms)"
              << std::setw(15) << "Multi (ms)"
              << std::setw(12) << "Speedup" << std::endl;
    std::cout << std::string(62, '-') << std::endl;

    for (const auto& r : results)
    {
        char config[32];
        snprintf(config, sizeof(config), "%d worlds x %d", r.numWorlds, r.numSimulations);

        std::cout << std::left << std::setw(20) << config
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(15) << r.singleThreadMs
                  << std::setw(15) << r.multiThreadMs
                  << std::setw(11) << r.speedup << "x" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Analysis" << std::endl;
    std::cout << "========================================" << std::endl;

    if (results.size() > 0)
    {
        double avgSpeedup = 0;
        for (const auto& r : results)
            avgSpeedup += r.speedup;
        avgSpeedup /= results.size();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Average speedup: " << avgSpeedup << "x" << std::endl;
        std::cout << "Theoretical max (with " << numCPU << " CPUs): " << numCPU << ".0x" << std::endl;
        std::cout << "Efficiency: " << (avgSpeedup / numCPU * 100) << "%" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
