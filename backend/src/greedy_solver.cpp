/*
 * greedy_solver.cpp — Quick Pick via Greedy (points/credit ratio)
 *
 * Two-phase greedy:
 *   Phase 1: Fill minimum role slots with best-ratio players per role
 *            (1 WK, 3 BAT, 1 AR, 3 BOWL = 8 players)
 *   Phase 2: Fill remaining 3 slots with best-ratio available
 *            (respecting role maxima and team constraint)
 *
 * Complexity: O(N log N) sort + O(N) scan = O(N log N)
 */

#include "greedy_solver.h"
#include <algorithm>
#include <chrono>

// Same captain/VC selection logic
static void select_captain_vc_greedy(TeamResult& result) {
    if (result.players.size() < 2) return;

    int best_idx = 0, second_idx = 1;
    if (result.players[second_idx].fantasy_points > result.players[best_idx].fantasy_points)
        std::swap(best_idx, second_idx);

    for (int i = 2; i < (int)result.players.size(); i++) {
        if (result.players[i].fantasy_points > result.players[best_idx].fantasy_points) {
            second_idx = best_idx;
            best_idx = i;
        } else if (result.players[i].fantasy_points > result.players[second_idx].fantasy_points) {
            second_idx = i;
        }
    }

    result.captain_id = result.players[best_idx].id;
    result.vice_captain_id = result.players[second_idx].id;

    double score = 0;
    for (auto& p : result.players) {
        if (p.id == result.captain_id)
            score += p.fantasy_points * 2.0;
        else if (p.id == result.vice_captain_id)
            score += p.fantasy_points * 1.5;
        else
            score += p.fantasy_points;
    }
    result.final_score = score;
}

TeamResult build_team_greedy(const std::vector<Player>& players) {
    return build_team_greedy_subset(players, (int)players.size());
}

TeamResult build_team_greedy_subset(const std::vector<Player>& players, int subset_size) {
    auto start = std::chrono::high_resolution_clock::now();

    int n = std::min(subset_size, (int)players.size());

    // Create indices sorted by points-per-credit ratio (descending)
    std::vector<int> indices(n);
    for (int i = 0; i < n; i++) indices[i] = i;

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        double ra = (double)players[a].fantasy_points / players[a].credits;
        double rb = (double)players[b].fantasy_points / players[b].credits;
        if (std::abs(ra - rb) > 1e-9) return ra > rb;
        return players[a].fantasy_points > players[b].fantasy_points;
    });

    // Track constraints
    int wk_count = 0, bat_count = 0, ar_count = 0, bowl_count = 0;
    int teamA_count = 0, teamB_count = 0;
    int credits_used = 0;
    int total_selected = 0;
    std::vector<bool> selected(n, false);

    // Identify teams
    std::string tA, tB;
    for (int i = 0; i < n; i++) {
        if (tA.empty()) tA = players[i].real_team;
        else if (players[i].real_team != tA && tB.empty()) { tB = players[i].real_team; break; }
    }

    auto can_add = [&](const Player& p) -> bool {
        if (credits_used + p.credits > 100) return false;
        if (total_selected >= 11) return false;

        int new_ta = teamA_count + ((p.real_team == tA) ? 1 : 0);
        int new_tb = teamB_count + ((p.real_team != tA) ? 1 : 0);
        if (new_ta > 7 || new_tb > 7) return false;

        if (p.role_idx == 0 && wk_count >= 4) return false;
        if (p.role_idx == 1 && bat_count >= 6) return false;
        if (p.role_idx == 2 && ar_count >= 4) return false;
        if (p.role_idx == 3 && bowl_count >= 6) return false;

        return true;
    };

    auto add_player = [&](int idx) {
        const Player& p = players[idx];
        selected[idx] = true;
        credits_used += p.credits;
        total_selected++;
        if (p.real_team == tA) teamA_count++;
        else teamB_count++;
        if (p.role_idx == 0) wk_count++;
        else if (p.role_idx == 1) bat_count++;
        else if (p.role_idx == 2) ar_count++;
        else bowl_count++;
    };

    // Minimum requirements: 1 WK, 3 BAT, 1 AR, 3 BOWL
    struct RoleReq { int role_idx; int min_needed; int* count; };
    RoleReq reqs[] = {
        {0, 1, &wk_count},
        {1, 3, &bat_count},
        {2, 1, &ar_count},
        {3, 3, &bowl_count}
    };

    // Phase 1: Fill minimum role requirements with best-ratio players per role
    for (auto& req : reqs) {
        for (int idx : indices) {
            if (selected[idx]) continue;
            if (players[idx].role_idx != req.role_idx) continue;
            if (*req.count >= req.min_needed) break;
            if (can_add(players[idx])) {
                add_player(idx);
            }
        }
    }

    // Phase 2: Fill remaining slots with best available (any role)
    for (int idx : indices) {
        if (total_selected >= 11) break;
        if (selected[idx]) continue;
        if (can_add(players[idx])) {
            add_player(idx);
        }
    }

    TeamResult result;
    result.method = "greedy";
    result.credits_used = credits_used;
    result.total_points = 0;

    for (int i = 0; i < n; i++) {
        if (selected[i]) {
            result.players.push_back(players[i]);
            result.total_points += players[i].fantasy_points;
        }
    }

    select_captain_vc_greedy(result);

    auto end = std::chrono::high_resolution_clock::now();
    result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}
