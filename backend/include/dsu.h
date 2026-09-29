/*
 * dsu.h — Friend Leagues using Disjoint Set Union (Union-Find)
 *
 * Implements DSU with:
 *   - Path compression in find()
 *   - Union by rank in unite()
 *
 * Operations:
 *   - find(x):       O(α(n)) amortized — nearly O(1)
 *   - unite(x, y):   O(α(n)) amortized
 *   - same(x, y):    O(α(n)) amortized
 *   - getMembers(x): O(size of component)
 *
 * Space complexity: O(N) where N = number of users
 */

#ifndef DSU_H
#define DSU_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

class DSU {
public:
    DSU();

    // Add a user if not already present
    void addUser(int userId, const std::string& userName);

    // Find the representative of the set containing userId
    int find(int userId);

    // Unite the sets containing userA and userB
    bool unite(int userA, int userB);

    // Check if two users are in the same league
    bool same(int userA, int userB);

    // Get all leagues as JSON (array of arrays)
    json getAllLeagues() const;

    // Check if a user exists
    bool hasUser(int userId) const;

    // Get league members for a given user
    json getLeagueMembers(int userId);

private:
    std::unordered_map<int, int> parent;
    std::unordered_map<int, int> rank_;
    std::unordered_map<int, std::string> userNames;
};

#endif // DSU_H
