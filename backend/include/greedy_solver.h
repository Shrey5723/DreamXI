/*
 * greedy_solver.h — Quick Pick using Greedy Algorithm
 *
 * Greedy selection by fantasy_points/credits ratio (descending).
 * Respects the same role/credit/team constraints as the DP solver.
 *
 * Algorithm:
 *   1. Sort players by points-per-credit ratio descending
 *   2. First pass: fill minimum role requirements (1 WK, 3 BAT, 1 AR, 3 BOWL)
 *   3. Second pass: fill remaining 3 slots with best available
 *
 * Time complexity: O(N log N) for sorting + O(N) for selection
 * Space complexity: O(N)
 */

#ifndef GREEDY_SOLVER_H
#define GREEDY_SOLVER_H

#include "knapsack_solver.h"
#include <vector>

// Build team using greedy approach
TeamResult build_team_greedy(const std::vector<Player>& players);

// Build team using greedy on a subset (for benchmarks)
TeamResult build_team_greedy_subset(const std::vector<Player>& players, int subset_size);

#endif // GREEDY_SOLVER_H
