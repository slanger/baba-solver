#include <iostream>

#include "Solver.h"

#include "Summarizer.h"

namespace BabaSolver
{
	void Summarize(const SolverResult& result)
    {
        // TODO: Sum up all iteration times and simulated move counts and print them.
        std::cout << "\n~~~ Summary ~~~\n";
        std::cout << "\nLevel: " << result.level_name << "\n";
        std::cout << "Level solved? " << (result.solved ? "YES" : "NO") << "\n";
        std::cout << "\nSolver options:\n";
        result.options.Print();
        std::cout << "\nInitial state:\n";
        result.iterations[0].initial_state->PrintGrid();
        std::cout << "\nPer iteration stats:\n";
        int iter_count = 0;
        for (const IterationResult& iter : result.iterations)
        {
            std::cout << "\nIteration " << ++iter_count << "\n";
            iter.Print();
            std::cout << "End state:\n";
            iter.end_state->PrintGrid();
        }
    }

}  // namespace BabaSolver
