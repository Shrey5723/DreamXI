/*
 * server.cpp — DreamXI REST API Server
 *
 * REST Endpoints:
 *   POST /build-team           - Optimal team builder (DP knapsack)
 *   POST /quick-pick           - Fast heuristic team builder (Greedy)
 *   GET  /leaderboard/top/:n   - Top-N users from augmented Treap (O(log n + k))
 *   GET  /rank/:userId         - Get user rank from augmented Treap (O(log n))
 *   POST /leaderboard/update   - Upsert user score in Treap (O(log n))
 *   GET  /trending/:k          - Top-K trending players from Min-Heap (O(N log K))
 *   POST /league/merge         - Merge two users' leagues via DSU (O(α(N)))
 *   GET  /league/same/:u1/:u2  - Check if two users share a league via DSU (O(α(N)))
 *   GET  /league/all           - Get all league clusters for visualization
 *   GET  /players              - Get all available players for grid display
 *   GET  /benchmark            - Compare DP vs Greedy performance on subsets
 */

#include "httplib.h"
#include "json.hpp"
#include "knapsack_solver.h"
#include "greedy_solver.h"
#include "rank_tree.h"
#include "min_heap.h"
#include "dsu.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>

using json = nlohmann::json;

// Global state protected by mutexes
static std::vector<Player> g_players;
static RankTree g_rank_tree;
static std::mutex g_tree_mutex;
static DSU g_dsu;
static std::mutex g_dsu_mutex;
static MinHeap g_min_heap;

void seed_initial_data() {
    // Seed initial leaderboard users
    struct SeedUser { int id; std::string name; int score; };
    std::vector<SeedUser> seed_users = {
        {101, "Aarav Sharma", 945},
        {102, "Rohan Verma", 912},
        {103, "Priya Patel", 889},
        {104, "Karan Singh", 874},
        {105, "Ananya Gupta", 860},
        {106, "Vikram Malhotra", 842},
        {107, "Sneha Reddy", 825},
        {108, "Rahul Dravid Fan", 810},
        {109, "Aditi Joshi", 795},
        {110, "Devansh Kumar", 780},
        {111, "Ishaan Nair", 765},
        {112, "Tanvi Shah", 750},
        {113, "Siddharth Rao", 735},
        {114, "Meera Iyer", 720},
        {115, "Arjun Kapoor", 705}
    };

    for (const auto& u : seed_users) {
        g_rank_tree.upsert(u.id, u.name, u.score);
        g_dsu.addUser(u.id, u.name);
    }

    // Seed some initial friend league connections
    g_dsu.unite(101, 102); // Aarav & Rohan
    g_dsu.unite(102, 105); // + Ananya (Royal Strikers)
    g_dsu.unite(103, 107); // Priya & Sneha
    g_dsu.unite(107, 110); // + Devansh (Super Kings)
    g_dsu.unite(104, 106); // Karan & Vikram (Power Hitters)
    g_dsu.unite(108, 111); // Rahul & Ishaan
    g_dsu.unite(111, 114); // + Meera (Spin Masters)
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::string data_path = "backend/data/players.json";
    try {
        g_players = load_players(data_path);
        std::cout << "[DreamXI] Loaded " << g_players.size() << " players from " << data_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DreamXI] Failed to load players: " << e.what() << std::endl;
        return 1;
    }

    seed_initial_data();
    std::cout << "[DreamXI] Seeded initial leaderboard (" << g_rank_tree.getSize() 
              << " users) and friend leagues." << std::endl;

    httplib::Server svr;

    // CORS & Common Headers Middleware
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        {"Access-Control-Max-Age", "86400"}
    });

    // Handle OPTIONS for CORS preflight
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    // GET /players - Raw player pool for grid display & filtering
    svr.Get("/players", [](const httplib::Request&, httplib::Response& res) {
        json j = json::array();
        for (const auto& p : g_players) {
            json item;
            item["id"] = p.id;
            item["name"] = p.name;
            item["role"] = p.role;
            item["real_team"] = p.real_team;
            item["credits"] = p.credits;
            item["fantasy_points"] = p.fantasy_points;
            item["recent_form"] = p.recent_form;
            item["ratio"] = (double)p.fantasy_points / p.credits;
            j.push_back(item);
        }
        res.set_content(j.dump(), "application/json");
    });

    // POST /build-team - DP Optimal Team Builder
    svr.Post("/build-team", [](const httplib::Request& req, httplib::Response& res) {
        int subset_size = (int)g_players.size();
        if (!req.body.empty()) {
            try {
                auto body = json::parse(req.body);
                if (body.contains("subset_size") && body["subset_size"].is_number_integer()) {
                    subset_size = body["subset_size"].get<int>();
                }
            } catch (...) {}
        }

        TeamResult result = build_team_dp_subset(g_players, subset_size);
        json out = team_result_to_json(result);
        res.set_content(out.dump(), "application/json");
    });

    // POST /quick-pick - Greedy Team Builder
    svr.Post("/quick-pick", [](const httplib::Request& req, httplib::Response& res) {
        int subset_size = (int)g_players.size();
        if (!req.body.empty()) {
            try {
                auto body = json::parse(req.body);
                if (body.contains("subset_size") && body["subset_size"].is_number_integer()) {
                    subset_size = body["subset_size"].get<int>();
                }
            } catch (...) {}
        }

        TeamResult result = build_team_greedy_subset(g_players, subset_size);
        json out = team_result_to_json(result);
        res.set_content(out.dump(), "application/json");
    });

    // GET /leaderboard/top/:n - Top N from Treap
    svr.Get(R"(/leaderboard/top/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int n = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(g_tree_mutex);
        json top = g_rank_tree.getTopN(n);
        json response;
        response["top"] = top;
        response["total_users"] = g_rank_tree.getSize();
        res.set_content(response.dump(), "application/json");
    });

    // GET /rank/:userId - Rank from Treap
    svr.Get(R"(/rank/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int uid = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(g_tree_mutex);
        json info = g_rank_tree.getUserInfo(uid);
        if (info.contains("error")) {
            res.status = 404;
        }
        res.set_content(info.dump(), "application/json");
    });

    // POST /leaderboard/update - Update or add user score in Treap
    svr.Post("/leaderboard/update", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            int uid = body["userId"].get<int>();
            std::string name = body.value("userName", "User " + std::to_string(uid));
            int score = body["score"].get<int>();

            std::lock_guard<std::mutex> lock(g_tree_mutex);
            g_rank_tree.upsert(uid, name, score);
            
            // Also ensure user is in DSU
            {
                std::lock_guard<std::mutex> dsu_lock(g_dsu_mutex);
                g_dsu.addUser(uid, name);
            }

            json response;
            response["success"] = true;
            response["userId"] = uid;
            response["userName"] = name;
            response["score"] = score;
            response["rank"] = g_rank_tree.getRank(uid);
            response["total_users"] = g_rank_tree.getSize();
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // GET /trending/:k - Binary Heap Top-K Trending Players
    svr.Get(R"(/trending/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int k = std::stoi(req.matches[1]);
        auto start = std::chrono::high_resolution_clock::now();
        json trending = g_min_heap.getTopK(g_players, k);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        json response;
        response["k"] = k;
        response["time_ms"] = ms;
        response["players"] = trending;
        res.set_content(response.dump(), "application/json");
    });

    // POST /league/merge - DSU unite
    svr.Post("/league/merge", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            int u1 = body["userA"].get<int>();
            int u2 = body["userB"].get<int>();

            std::lock_guard<std::mutex> lock(g_dsu_mutex);
            if (!g_dsu.hasUser(u1)) {
                g_dsu.addUser(u1, "User " + std::to_string(u1));
            }
            if (!g_dsu.hasUser(u2)) {
                g_dsu.addUser(u2, "User " + std::to_string(u2));
            }

            bool merged = g_dsu.unite(u1, u2);
            json response;
            response["success"] = true;
            response["merged"] = merged;
            response["userA"] = u1;
            response["userB"] = u2;
            response["same_league"] = g_dsu.same(u1, u2);
            response["leagues"] = g_dsu.getAllLeagues();
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // GET /league/same/:u1/:u2 - DSU same check
    svr.Get(R"(/league/same/(\d+)/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int u1 = std::stoi(req.matches[1]);
        int u2 = std::stoi(req.matches[2]);

        std::lock_guard<std::mutex> lock(g_dsu_mutex);
        bool same_league = g_dsu.same(u1, u2);

        json response;
        response["userA"] = u1;
        response["userB"] = u2;
        response["same_league"] = same_league;
        res.set_content(response.dump(), "application/json");
    });

    // GET /league/all - Get all current leagues
    svr.Get("/league/all", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_dsu_mutex);
        json leagues = g_dsu.getAllLeagues();
        res.set_content(json{{"leagues", leagues}}.dump(), "application/json");
    });

    // GET /benchmark - Live performance benchmark across player subset sizes
    svr.Get("/benchmark", [](const httplib::Request&, httplib::Response& res) {
        std::vector<int> sample_sizes = {40, 80, 120, 160, 200};
        json results = json::array();

        for (int sz : sample_sizes) {
            if (sz > (int)g_players.size()) continue;

            auto g_res = build_team_greedy_subset(g_players, sz);
            auto dp_res = build_team_dp_subset(g_players, sz);

            json entry;
            entry["candidate_count"] = sz;
            entry["dp_time_ms"] = dp_res.time_ms;
            entry["dp_points"] = dp_res.total_points;
            entry["dp_final_score"] = dp_res.final_score;
            entry["greedy_time_ms"] = g_res.time_ms;
            entry["greedy_points"] = g_res.total_points;
            entry["greedy_final_score"] = g_res.final_score;
            entry["optimality_gap_percent"] = (dp_res.total_points > 0)
                ? ((dp_res.total_points - g_res.total_points) * 100.0 / dp_res.total_points)
                : 0.0;
            results.push_back(entry);
        }

        res.set_content(json{{"benchmark", results}}.dump(), "application/json");
    });

    // Mount frontend static files
    svr.set_mount_point("/", "./frontend");

    std::cout << "\n========================================================" << std::endl;
    std::cout << "  DreamXI Server running at http://localhost:" << port << std::endl;
    std::cout << "  Static UI: http://localhost:" << port << "/index.html" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    svr.listen("0.0.0.0", port);
    return 0;
}
