cmake_minimum_required(VERSION 3.10)
project(Hearts LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # /wd4267 /wd4244: suppress size_t to int narrowing warnings (safe for bounded game values)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /wd4267 /wd4244")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
endif()

# Find pthreads
find_package(Threads REQUIRED)

# Library source files (all .cpp except main.cpp)
set(LIB_SOURCES
    Algorithm.cpp
    CardGameState.cpp
    CardProbabilityData.cpp
    fpUtil.cpp
    Game.cpp
    GameState.cpp
    hash.cpp
    Hearts.cpp
    HeartsGameData.cpp
    HeartsGameHistories.cpp
    iiGameState.cpp
    iiMonteCarlo.cpp
    algorithmStates.cpp
    mt_random.cpp
    Player.cpp
    ProblemState.cpp
    States.cpp
    statistics.cpp
    Timer.cpp
    UCT.cpp
)

# Create static library for shared code
add_library(hearts_lib STATIC ${LIB_SOURCES})
target_link_libraries(hearts_lib PUBLIC Threads::Threads)

# Main executable
add_executable(hearts main.cpp)
target_link_libraries(hearts PRIVATE hearts_lib)

# Enable testing
enable_testing()

# Comprehensive test suite
add_executable(hearts_tests tests.cpp)
target_link_libraries(hearts_tests PRIVATE hearts_lib)
add_test(NAME hearts_tests COMMAND hearts_tests)

# Queen of Spades test suite
add_executable(queen_spade_tests queen_spade_tests.cpp)
target_link_libraries(queen_spade_tests PRIVATE hearts_lib)
add_test(NAME queen_spade_tests COMMAND queen_spade_tests)

# Performance benchmark
add_executable(hearts_benchmark benchmark.cpp)
target_link_libraries(hearts_benchmark PRIVATE hearts_lib)

# HTTP REST API Server
set(SERVER_SOURCES
    server/ServerMain.cpp
    server/HeartsAIServer.cpp
    server/AIRequestHandler.cpp
    server/JsonProtocol.cpp
)

add_executable(hearts_server ${SERVER_SOURCES})
target_include_directories(hearts_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_link_libraries(hearts_server PRIVATE hearts_lib Threads::Threads)

# Windows: link socket libraries for cpp-httplib
if(WIN32)
    target_link_libraries(hearts_server PRIVATE ws2_32)
endif()
