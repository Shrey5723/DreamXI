/*
 * dsu.cpp — Disjoint Set Union (Union-Find) for Friend Leagues
 *
 * Path compression in find(): flattens the tree on each query
 * Union by rank in unite(): always attaches shorter tree under taller
 *
 * Amortized complexity per operation: O(α(n)) ≈ O(1)
 * Space: O(N)
 */

#include "dsu.h"

DSU::DSU() {}

void DSU::addUser(int userId, const std::string& userName) {
    if (parent.find(userId) == parent.end()) {
        parent[userId] = userId;
        rank_[userId] = 0;
        userNames[userId] = userName;
    }
}

int DSU::find(int userId) {
    if (parent.find(userId) == parent.end()) return -1;

    // Path compression
    if (parent[userId] != userId) {
        parent[userId] = find(parent[userId]);
    }
    return parent[userId];
}

bool DSU::unite(int userA, int userB) {
    int rootA = find(userA);
    int rootB = find(userB);

    if (rootA < 0 || rootB < 0) return false;
    if (rootA == rootB) return false; // Already in same set

    // Union by rank
    if (rank_[rootA] < rank_[rootB]) {
        parent[rootA] = rootB;
    } else if (rank_[rootA] > rank_[rootB]) {
        parent[rootB] = rootA;
    } else {
        parent[rootB] = rootA;
        rank_[rootA]++;
    }

    return true;
}

bool DSU::same(int userA, int userB) {
    int rootA = find(userA);
    int rootB = find(userB);
    if (rootA < 0 || rootB < 0) return false;
    return rootA == rootB;
}

bool DSU::hasUser(int userId) const {
    return parent.find(userId) != parent.end();
}

json DSU::getAllLeagues() const {
    // Group users by their root
    std::unordered_map<int, std::vector<int>> groups;

    // Need non-const find, so we'll manually walk parent chains
    // (const-safe version without path compression)
    auto findConst = [&](int u) -> int {
        int x = u;
        auto it = parent.find(x);
        while (it != parent.end() && it->second != x) {
            x = it->second;
            it = parent.find(x);
        }
        return x;
    };

    for (auto& [uid, _] : parent) {
        int root = findConst(uid);
        groups[root].push_back(uid);
    }

    json result = json::array();
    for (auto& [root, members] : groups) {
        json league;
        league["league_id"] = root;
        json memberArr = json::array();
        for (int uid : members) {
            auto nameIt = userNames.find(uid);
            json m;
            m["userId"] = uid;
            m["userName"] = (nameIt != userNames.end()) ? nameIt->second : "Unknown";
            memberArr.push_back(m);
        }
        league["members"] = memberArr;
        league["size"] = (int)members.size();
        result.push_back(league);
    }

    return result;
}

json DSU::getLeagueMembers(int userId) {
    int root = find(userId);
    if (root < 0) return json{{"error", "User not found"}};

    json members = json::array();
    for (auto& [uid, _] : parent) {
        if (find(uid) == root) {
            auto nameIt = userNames.find(uid);
            json m;
            m["userId"] = uid;
            m["userName"] = (nameIt != userNames.end()) ? nameIt->second : "Unknown";
            members.push_back(m);
        }
    }

    return json{{"league_id", root}, {"members", members}, {"size", members.size()}};
}
