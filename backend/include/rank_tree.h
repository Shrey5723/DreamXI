/*
 * rank_tree.h — Live Leaderboard using Custom Augmented Treap
 *
 * Hand-built order-statistic tree (Treap variant) with subtree sizes.
 * Does NOT use std::set or std::map — entirely custom BST.
 *
 * Each node stores: (score, userId), random priority, subtree size.
 * BST ordering: descending by score (highest = rank 1), tiebreak by userId.
 *
 * Supported operations:
 *   - insert(userId, score):    O(log n) expected
 *   - update(userId, newScore): O(log n) expected (remove + reinsert)
 *   - getRank(userId):          O(log n) expected
 *   - getTopN(k):               O(log n + k) expected
 *
 * Implementation: Treap with split/merge operations.
 */

#ifndef RANK_TREE_H
#define RANK_TREE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include "json.hpp"

using json = nlohmann::json;

struct TreapNode {
    int score;
    int userId;
    std::string userName;
    int priority;       // random heap priority
    int size;           // subtree size
    TreapNode* left;
    TreapNode* right;

    TreapNode(int s, int uid, const std::string& name, int prio)
        : score(s), userId(uid), userName(name), priority(prio),
          size(1), left(nullptr), right(nullptr) {}
};

class RankTree {
public:
    RankTree();
    ~RankTree();

    // Insert or update a user's score
    void upsert(int userId, const std::string& userName, int score);

    // Get the rank (1-based, rank 1 = highest score) of a user
    int getRank(int userId) const;

    // Get top N users as JSON array [{rank, userId, userName, score}, ...]
    json getTopN(int n) const;

    // Get total number of users
    int getSize() const;

    // Get user info
    json getUserInfo(int userId) const;

private:
    TreapNode* root;
    std::unordered_map<int, std::pair<int, std::string>> userScores; // userId -> (score, name)

    // Treap operations
    void updateSize(TreapNode* node);
    void split(TreapNode* node, int score, int userId, TreapNode*& left, TreapNode*& right);
    TreapNode* merge(TreapNode* left, TreapNode* right);
    void insert(TreapNode*& root, TreapNode* node);
    void remove(TreapNode*& root, int score, int userId);
    int findRank(TreapNode* node, int score, int userId) const;
    void collectTopN(TreapNode* node, int& remaining, std::vector<TreapNode*>& result) const;
    void destroyTree(TreapNode* node);

    // Comparison: returns true if (s1, uid1) should come BEFORE (s2, uid2) in rank order
    // Higher score = lower rank number = comes first
    static bool comesBefore(int s1, int uid1, int s2, int uid2) {
        if (s1 != s2) return s1 > s2;
        return uid1 < uid2;
    }
};

#endif // RANK_TREE_H
