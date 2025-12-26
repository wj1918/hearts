Hearts - PIMC MCTS Card Game AI
================================

A C++ implementation of Hearts card game with AI using Perfect Information
Monte Carlo (PIMC) and Monte Carlo Tree Search (MCTS).


CREDITS
-------

Original Author: Nathan Sturtevant
                 University of Alberta / University of Denver
                 https://www.cs.du.edu/~sturtevant/

This project implements research on imperfect information game playing
using Monte Carlo Tree Search techniques.

License: GNU General Public License v3.0


BUILD
-----

Using Make:
    make all          # Build main executable
    make tests        # Build test binaries
    make run-tests    # Run tests

Using CMake:
    mkdir build && cd build
    cmake ..
    make


RUN
---

    ./hearts          # Run 100 games with AI players


AI CONFIGURATION
----------------

Default settings (per move):
- 30 world samples
- 333 UCT simulations per world
- ~10,000 total simulations

Key parameters in main.cpp:
- worlds = 30        # Number of sampled worlds
- sims = 10000       # Total simulations
- C = 0.4            # UCT exploration constant
- epsilon = 0.1      # Playout exploration rate


GAME RULES
----------

Standard Hearts rules with:
- Queen of Spades = 13 points
- Each Heart = 1 point
- Lowest score wins
- Pass 3 cards (left/right/across/hold rotation)
- Game ends when someone reaches 100 points


FILES
-----

Core:
- Hearts.cpp/h       - Game rules and state
- UCT.cpp/h          - Monte Carlo Tree Search
- iiMonteCarlo.cpp/h - Imperfect info handling
- main.cpp           - Entry point

Utilities:
- mt_random.cpp/h    - Mersenne Twister RNG
- statistics.cpp/h   - Performance tracking
- Timer.cpp/h        - Timing utilities


REQUIREMENTS
------------

- C++11 compiler (clang++ or g++)
- pthreads library
- CMake 3.10+ (optional)
