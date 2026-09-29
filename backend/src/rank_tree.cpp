/*
 * rank_tree.cpp — Custom Augmented Treap (Order-Statistic Tree)
 *
 * Hand-built Treap (tree + heap) with subtree size augmentation.
 * NO std::set or std::map used — entirely custom BST.
 *
 * BST property: nodes ordered by (score DESC, userId ASC)
 *   → rank 1 = highest score
 * Heap property: parent priority >= child priority (max-heap on random priority)
 * Size augmentation: each node stores subtree size for O(log n) rank queries
 *
 * Operations:
 *   split(node, key) → (left, right)   O(log n) expected
 *   merge(left, right) → node          O(log n) expected
 *   insert/remove via split+merge      O(log n) expected
 *   getRank via walking the tree        O(log n) expected
 *   getTopN via in-order traversal      O(log n + k)
 */

#include "rank_tree.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>

RankTree::RankTree() : root(nullptr) {
    std::srand((unsigned)std::time(nullptr));
}

RankTree::~RankTree() {
    destroyTree(root);
}

void RankTree::destroyTree(TreapNode* node) {
    if (!node) return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

void RankTree::updateSize(TreapNode* node) {
    if (!node) return;
    node->size = 1;
    if (node->left) node->size += node->left->size;
    if (node->right) node->size += node->right->size;
}

// Split: everything that "comesBefore" (score, userId) goes to left;
// everything else goes to right.
// In our ordering: comesBefore means higher score or same score + lower userId.
void RankTree::split(TreapNode* node, int score, int userId,
                     TreapNode*& left, TreapNode*& right) {
    if (!node) {
        left = right = nullptr;
        return;
    }

    // If this node comes before (score, userId), it goes to the LEFT subtree
    if (comesBefore(node->score, node->userId, score, userId)) {
        // node and its left subtree go to left; split right subtree
        split(node->right, score, userId, node->right, right);
        left = node;
    } else {
        // node and its right subtree go to right; split left subtree
        split(node->left, score, userId, left, node->left);
        right = node;
    }
    updateSize(node);
}

TreapNode* RankTree::merge(TreapNode* left, TreapNode* right) {
    if (!left) return right;
    if (!right) return left;

    if (left->priority >= right->priority) {
        left->right = merge(left->right, right);
        updateSize(left);
        return left;
    } else {
        right->left = merge(left, right->left);
        updateSize(right);
        return right;
    }
}

void RankTree::insert(TreapNode*& root, TreapNode* node) {
    TreapNode *left, *right;
    split(root, node->score, node->userId, left, right);
    root = merge(merge(left, node), right);
}

void RankTree::remove(TreapNode*& root, int score, int userId) {
    if (!root) return;

    if (root->score == score && root->userId == userId) {
        TreapNode* temp = merge(root->left, root->right);
        delete root;
        root = temp;
    } else if (comesBefore(score, userId, root->score, root->userId)) {
        // Target comes before root → it's in the left subtree
        remove(root->left, score, userId);
    } else {
        remove(root->right, score, userId);
    }
    updateSize(root);
}

void RankTree::upsert(int userId, const std::string& userName, int score) {
    // Remove old entry if exists
    auto it = userScores.find(userId);
    if (it != userScores.end()) {
        remove(root, it->second.first, userId);
    }

    // Insert new entry
    TreapNode* node = new TreapNode(score, userId, userName, std::rand());
    insert(root, node);
    userScores[userId] = {score, userName};
}

int RankTree::findRank(TreapNode* node, int score, int userId) const {
    if (!node) return 0;

    if (node->score == score && node->userId == userId) {
        // Rank = left subtree size + 1
        return (node->left ? node->left->size : 0) + 1;
    }

    if (comesBefore(score, userId, node->score, node->userId)) {
        // Target comes before this node → it's in the left subtree
        return findRank(node->left, score, userId);
    } else {
        // Target comes after → it's in the right subtree
        int left_size = (node->left ? node->left->size : 0);
        return left_size + 1 + findRank(node->right, score, userId);
    }
}

int RankTree::getRank(int userId) const {
    auto it = userScores.find(userId);
    if (it == userScores.end()) return -1;
    return findRank(root, it->second.first, userId);
}

void RankTree::collectTopN(TreapNode* node, int& remaining,
                           std::vector<TreapNode*>& result) const {
    if (!node || remaining <= 0) return;

    // In-order traversal (left first = highest scores first in our ordering)
    collectTopN(node->left, remaining, result);
    if (remaining <= 0) return;

    result.push_back(node);
    remaining--;

    collectTopN(node->right, remaining, result);
}

json RankTree::getTopN(int n) const {
    json arr = json::array();
    std::vector<TreapNode*> top;
    int rem = n;
    collectTopN(root, rem, top);

    for (int i = 0; i < (int)top.size(); i++) {
        json entry;
        entry["rank"] = i + 1;
        entry["userId"] = top[i]->userId;
        entry["userName"] = top[i]->userName;
        entry["score"] = top[i]->score;
        arr.push_back(entry);
    }
    return arr;
}

int RankTree::getSize() const {
    return root ? root->size : 0;
}

json RankTree::getUserInfo(int userId) const {
    auto it = userScores.find(userId);
    if (it == userScores.end()) {
        return json{{"error", "User not found"}};
    }

    json info;
    info["userId"] = userId;
    info["userName"] = it->second.second;
    info["score"] = it->second.first;
    info["rank"] = findRank(root, it->second.first, userId);
    info["total_users"] = getSize();
    return info;
}
