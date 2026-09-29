/*
 * min_heap.h — Top-K Trending Players using Binary Heap
 *
 * Uses a min-heap of size K to efficiently find the top-K players
 * by recent form without performing a full sort.
 *
 * Algorithm:
 *   1. Build a min-heap of first K players by recent_form
 *   2. For each remaining player, if form > heap top, replace and heapify
 *   3. Extract all K from heap (they are the top-K)
 *
 * Time complexity:  O(N log K) where N = total players, K = desired top count
 * Space complexity: O(K)
 */

#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include "knapsack_solver.h"
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class MinHeap {
public:
    MinHeap();

    // Get top K players by recent form
    json getTopK(const std::vector<Player>& players, int k);

private:
    struct HeapEntry {
        double form;
        int playerIdx;

        bool operator<(const HeapEntry& o) const { return form < o.form; }
        bool operator>(const HeapEntry& o) const { return form > o.form; }
    };

    std::vector<HeapEntry> heap;

    void heapifyUp(int idx);
    void heapifyDown(int idx);
    void buildHeap();
    HeapEntry extractMin();
    void insert(const HeapEntry& entry);
    int size() const { return (int)heap.size(); }
    const HeapEntry& top() const { return heap[0]; }
    void replaceTop(const HeapEntry& entry);
};

#endif // MIN_HEAP_H
