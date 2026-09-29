/*
 * knapsack_solver.cpp — Multi-constraint 0/1 Knapsack DP Implementation
 *
 * DP with state (credits, wk, bat, ar, bowl, teamA):
 *   - credits: 0..100 (101 values)
 *   - wk: 0..4      (5 values)
 *   - bat: 0..6      (7 values)
 *   - ar: 0..4       (5 values)
 *   - bowl: 0..6     (7 values)
 *   - teamA: 0..7    (8 values)
 *   Total states: 101 * 5 * 7 * 5 * 7 * 8 = 989,800
 *
 * For each player, iterate backward through states to ensure 0/1 property.
 * Keep a bitset for backtracking to recover the chosen 11 players.
 *
 * Complexity: O(N * 989800) time, O(989800 + N*989800/8) space
 */

#include "knapsack_solver.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <climits>
#include <iostream>

// State dimensions
static constexpr int MAX_CR = 101;  // 0..100
static constexpr int MAX_WK = 5;   // 0..4
static constexpr int MAX_BT = 7;   // 0..6
static constexpr int MAX_AR = 5;   // 0..4
static constexpr int MAX_BW = 7;   // 0..6
static constexpr int MAX_TA = 8;   // 0..7

static constexpr int TOTAL_STATES = MAX_CR * MAX_WK * MAX_BT * MAX_AR * MAX_BW * MAX_TA;
// = 101 * 5 * 7 * 5 * 7 * 8 = 989,800

static inline int encode(int cr, int wk, int bt, int ar, int bw, int ta) {
    return ((((cr * MAX_WK + wk) * MAX_BT + bt) * MAX_AR + ar) * MAX_BW + bw) * MAX_TA + ta;
}

static inline void decode(int idx, int& cr, int& wk, int& bt, int& ar, int& bw, int& ta) {
    ta = idx % MAX_TA; idx /= MAX_TA;
    bw = idx % MAX_BW; idx /= MAX_BW;
    ar = idx % MAX_AR; idx /= MAX_AR;
    bt = idx % MAX_BT; idx /= MAX_BT;
    wk = idx % MAX_WK; idx /= MAX_WK;
    cr = idx;
}

// Map team names to indices
static std::string teamA_name, teamB_name;

static void identify_teams(const std::vector<Player>& players) {
    teamA_name.clear();
    teamB_name.clear();
    for (auto& p : players) {
        if (teamA_name.empty()) {
            teamA_name = p.real_team;
        } else if (p.real_team != teamA_name && teamB_name.empty()) {
            teamB_name = p.real_team;
            break;
        }
    }
}

std::vector<Player> load_players(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open player data file: " + filepath);
    }

    json j;
    file >> j;

    std::vector<Player> players;
    players.reserve(j.size());

    // Identify teams
    std::string tA, tB;
    for (auto& item : j) {
        std::string team = item["real_team"];
        if (tA.empty()) tA = team;
        else if (team != tA && tB.empty()) { tB = team; break; }
    }

    for (auto& item : j) {
        Player p;
        p.id = item["id"];
        p.name = item["name"];
        p.role = item["role"];
        p.real_team = item["real_team"];
        p.credits = item["credits"];
        p.fantasy_points = item["fantasy_points"];
        p.recent_form = item["recent_form"];

        // Map role to index
        if (p.role == "wicketkeeper") p.role_idx = 0;
        else if (p.role == "batter") p.role_idx = 1;
        else if (p.role == "allrounder") p.role_idx = 2;
        else p.role_idx = 3; // bowler

        // Map team to index
        p.team_idx = (p.real_team == tA) ? 0 : 1;

        players.push_back(p);
    }

    teamA_name = tA;
    teamB_name = tB;

    return players;
}

// Select captain and vice-captain (highest and second-highest points)
static void select_captain_vc(TeamResult& result) {
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

    // Calculate final score with multipliers
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

TeamResult build_team_dp(const std::vector<Player>& players) {
    return build_team_dp_subset(players, (int)players.size());
}

TeamResult build_team_dp_subset(const std::vector<Player>& players, int subset_size) {
    auto start = std::chrono::high_resolution_clock::now();

    int n = std::min(subset_size, (int)players.size());

    // Identify teams for this subset
    identify_teams(players);

    // Allocate DP table: -1 means unreachable
    std::vector<int> dp(TOTAL_STATES, -1);

    // Keep bitset for backtracking: keep[i * TOTAL_STATES + s] = 1 bit
    // Using vector<uint8_t> with bit packing
    int keep_size = ((long long)n * TOTAL_STATES + 7) / 8;
    std::vector<uint8_t> keep(keep_size, 0);

    auto set_keep = [&](int player_idx, int state) {
        long long bit = (long long)player_idx * TOTAL_STATES + state;
        keep[bit / 8] |= (1 << (bit % 8));
    };

    auto get_keep = [&](int player_idx, int state) -> bool {
        long long bit = (long long)player_idx * TOTAL_STATES + state;
        return (keep[bit / 8] >> (bit % 8)) & 1;
    };

    // Initialize: state (0,0,0,0,0,0) = 0 points
    dp[encode(0, 0, 0, 0, 0, 0)] = 0;

    // Process each player
    for (int i = 0; i < n; i++) {
        const Player& p = players[i];
        int pcr = p.credits;
        int pwk = (p.role_idx == 0) ? 1 : 0;
        int pbt = (p.role_idx == 1) ? 1 : 0;
        int par = (p.role_idx == 2) ? 1 : 0;
        int pbw = (p.role_idx == 3) ? 1 : 0;
        int pta = (p.team_idx == 0) ? 1 : 0;
        int pts = p.fantasy_points;

        // Iterate backward through all states to maintain 0/1 property
        for (int cr = 100; cr >= 0; cr--) {
            if (cr + pcr > 100) continue; // new credits would exceed budget
            for (int wk = MAX_WK - 1 - pwk; wk >= 0; wk--) {
                // After adding player: wk + pwk <= MAX_WK - 1 (i.e., <= 4)
                int nwk = wk + pwk;
                if (nwk >= MAX_WK) continue;
                for (int bt = MAX_BT - 1 - pbt; bt >= 0; bt--) {
                    int nbt = bt + pbt;
                    if (nbt >= MAX_BT) continue;
                    for (int ar = MAX_AR - 1 - par; ar >= 0; ar--) {
                        int nar = ar + par;
                        if (nar >= MAX_AR) continue;
                        for (int bw = MAX_BW - 1 - pbw; bw >= 0; bw--) {
                            int nbw = bw + pbw;
                            if (nbw >= MAX_BW) continue;

                            // Check total players doesn't exceed 11
                            int count = wk + bt + ar + bw;
                            if (count >= 11) continue; // already have 11 or more

                            for (int ta = MAX_TA - 1 - pta; ta >= 0; ta--) {
                                int nta = ta + pta;
                                if (nta >= MAX_TA) continue;

                                // Also check teamB count
                                int count_new = count + 1;
                                int tb_new = count_new - nta;
                                if (tb_new > 7) continue;

                                int from_idx = encode(cr, wk, bt, ar, bw, ta);
                                if (dp[from_idx] < 0) continue; // unreachable

                                int to_idx = encode(cr + pcr, nwk, nbt, nar, nbw, nta);
                                int new_val = dp[from_idx] + pts;

                                if (new_val > dp[to_idx]) {
                                    dp[to_idx] = new_val;
                                    set_keep(i, to_idx);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Find the best valid final state
    int best_val = -1;
    int best_state = -1;

    for (int wk = 1; wk <= 4; wk++) {
        for (int bat = 3; bat <= 6; bat++) {
            for (int ar = 1; ar <= 4; ar++) {
                int bowl = 11 - wk - bat - ar;
                if (bowl < 3 || bowl > 6) continue;
                for (int ta = 4; ta <= 7; ta++) {
                    // teamB = 11 - ta, must be >= 4 (i.e., ta <= 7, already checked)
                    int tb = 11 - ta;
                    if (tb < 4 || tb > 7) continue;
                    for (int cr = 0; cr <= 100; cr++) {
                        int idx = encode(cr, wk, bat, ar, bowl, ta);
                        if (dp[idx] > best_val) {
                            best_val = dp[idx];
                            best_state = idx;
                        }
                    }
                }
            }
        }
    }

    TeamResult result;
    result.method = "dp";
    result.total_points = 0;
    result.credits_used = 0;
    result.final_score = 0;
    result.captain_id = -1;
    result.vice_captain_id = -1;

    if (best_state < 0) {
        // No valid team found
        auto end = std::chrono::high_resolution_clock::now();
        result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return result;
    }

    // Backtrack to find chosen players
    int cr, wk, bt, ar, bw, ta;
    decode(best_state, cr, wk, bt, ar, bw, ta);

    result.total_points = best_val;
    result.credits_used = cr;

    for (int i = n - 1; i >= 0; i--) {
        int cur_idx = encode(cr, wk, bt, ar, bw, ta);
        if (get_keep(i, cur_idx)) {
            result.players.push_back(players[i]);

            // Remove this player's contribution
            cr -= players[i].credits;
            if (players[i].role_idx == 0) wk--;
            else if (players[i].role_idx == 1) bt--;
            else if (players[i].role_idx == 2) ar--;
            else bw--;
            if (players[i].team_idx == 0) ta--;
        }
    }

    // Select captain and vice-captain
    select_captain_vc(result);

    auto end = std::chrono::high_resolution_clock::now();
    result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

json team_result_to_json(const TeamResult& result) {
    json j;
    j["method"] = result.method;
    j["total_points"] = result.total_points;
    j["final_score"] = result.final_score;
    j["credits_used"] = result.credits_used;
    j["captain_id"] = result.captain_id;
    j["vice_captain_id"] = result.vice_captain_id;
    j["time_ms"] = result.time_ms;

    json players_arr = json::array();
    for (auto& p : result.players) {
        json pj;
        pj["id"] = p.id;
        pj["name"] = p.name;
        pj["role"] = p.role;
        pj["real_team"] = p.real_team;
        pj["credits"] = p.credits;
        pj["fantasy_points"] = p.fantasy_points;
        pj["recent_form"] = p.recent_form;
        pj["is_captain"] = (p.id == result.captain_id);
        pj["is_vice_captain"] = (p.id == result.vice_captain_id);

        double multiplier = 1.0;
        if (p.id == result.captain_id) multiplier = 2.0;
        else if (p.id == result.vice_captain_id) multiplier = 1.5;
        pj["effective_points"] = p.fantasy_points * multiplier;

        players_arr.push_back(pj);
    }
    j["players"] = players_arr;

    // Role counts
    int wk = 0, bat = 0, ar = 0, bowl = 0;
    for (auto& p : result.players) {
        if (p.role == "wicketkeeper") wk++;
        else if (p.role == "batter") bat++;
        else if (p.role == "allrounder") ar++;
        else bowl++;
    }
    j["role_counts"] = {{"wicketkeeper", wk}, {"batter", bat}, {"allrounder", ar}, {"bowler", bowl}};

    return j;
}
