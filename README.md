# Baba Is You solver

This repo contains the code for a program that can solve levels from the video game *Baba Is You*.

## How to build and run the code

### macOS

1. Install [CMake](https://cmake.org/) and [Homebrew](https://brew.sh/).
2. In the terminal, install dependencies: `brew install gcc` and `brew install tbb`
3. Clone this repo and navigate to the `src/` directory.
4. Generate CMake files: `cmake -B build --toolchain=toolchain.cmake`
5. Build the code: `cmake --build build`
6. Run the binary: `build/BabaSolver`

### Windows

1. Install [CMake](https://cmake.org/) and [Visual Studio](https://visualstudio.microsoft.com/).
2. Clone this repo and navigate to the `src/` directory.
3. Generate CMake files: `cmake -B build`
4. Build and run the code in Visual Studio through the generated .sln file.

## How the algorithm works

The algorithm works like a [MapReduce](https://en.wikipedia.org/wiki/MapReduce), where all possible
final game states (up to `max_turn_depth` turns deep in the game state tree) are simulated and scored
based on how close to beating the level the game state is (the "map" part of MapReduce). The game state
with the highest score is returned (the "reduce" part). The game state with the highest score is then
iterated on by another run of the solver starting at that game state, and this iteration continues until
`iteration_count` is reached.

The algorithm also features:

* **Caching:** Game states are cached (not recomputed) until `max_cache_depth` is reached.
* **Pruning:** Game states that are unlikely or impossible to reach a working solution are pruned.
* **Parallelization:** Game states are simulated in parallel for maximum throughput.

## Limitations

The program has the following limitations. Eventually I'd like to change the code so that it no
longer has these limitations.

1. Currently, the program can only solve one level, The Floatiest Platforms (Mountain-Extra 1).

## FAQ

### Why are only certain levels available for solving?

The mechanics of the *Baba Is You* levels are reverse engineered and baked into the code, which has
benefits and drawbacks. The main benefit is that the code can run extremely quickly, since it's all
pure C++ code and there is no need to wait for normal video game functions like graphics and audio.
The main drawback is that every new level added to the solver requires reverse engineering the mechanics
of the level and adding code for those mechanics.

### Is it always possible for the solver to beat a level?

No, it depends on the config options passed to the solver (e.g. `iteration_count` and `max_turn_depth`)
as well as the scoring function of the level. The algorithm relies on heuristics which make it possible
for the solver to not reach a working solution, especially if `iteration_count` or `max_turn_depth` are
set too low.
