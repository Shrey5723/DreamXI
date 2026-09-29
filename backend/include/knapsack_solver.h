/*
 * knapsack_solver.h — Optimal Team Builder using Dynamic Programming
 *
 * Multi-constraint 0/1 Knapsack:
 *   Select exactly 11 players maximizing total fantasy_points, subject to:
 *     - total credits <= 100
 *     - 1-4 wicketkeepers, 3-6 batters, 1-4 all-rounders, 3-6 bowlers
 *     - max 7 players from any single real_team
 *
 * State: dp[credits][wk][bat][ar][bowl][teamA]
 *   credits: 0..100, wk: 0..4, bat: 0..6, ar: 0..4, bowl: 0..6, teamA: 0..7
 *   Total states ≈ 990K — fits comfortably in memory.
 *
 * Time complexity: O(N * TOTAL_STATES) where N = number of players
 * Space complexity: O(TOTAL_STATES + N * TOTAL_STATES / 8) for dp + keep bitset
 *
 * Captain/Vice-Captain: After selecting 11, pick the two highest-scoring players
 * as captain (2x) and vice-captain (1.5x) to maximize final score.
 */

#ifndef KNAPSACK_SOLVER_H
#define KNAPSACK_SOLVER_H

#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

struct Player {
    int id;
    std::string name;
    std::string role;       // "wicketkeeper", "batter", "allrounder", "bowler"
    std::string real_team;
    int credits;
    int fantasy_points;
    double recent_form;
    int role_idx;           // 0=wk, 1=bat, 2=ar, 3=bowl
    int team_idx;           // 0=teamA, 1=teamB
};

struct TeamResult {
    std::vector<Player> players;
    int total_points;
    double final_score;      // after captain/VC multipliers
    int credits_used;
    int captain_id;
    int vice_captain_id;
    double time_ms;
    std::string method;
};

// Load players from JSON file
std::vector<Player> load_players(const std::string& filepath);

// Build optimal team using DP knapsack
TeamResult build_team_dp(const std::vector<Player>& players);

// Build optimal team using DP on a subset (for benchmarks)
TeamResult build_team_dp_subset(const std::vector<Player>& players, int subset_size);

// Convert TeamResult to JSON
json team_result_to_json(const TeamResult& result);

#endif // KNAPSACK_SOLVER_H
