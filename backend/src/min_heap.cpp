/*
 * min_heap.cpp — Binary Min-Heap for Top-K Trending Players
 *
 * Uses a min-heap of size K:
 *   1. Insert first K players
 *   2. For each remaining player, if form > heap.top(), replace top and heapify
 *   3. Extract all K elements (they are the top-K by recent form)
 *
 * All heap operations implemented from scratch (no std::priority_queue).
 *
 * Time:  O(N log K)
 * Space: O(K)
 */

#include "min_heap.h"
#include <algorithm>

MinHeap::MinHeap() {}

void MinHeap::heapifyUp(int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap[idx].form < heap[parent].form) {
            std::swap(heap[idx], heap[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

void MinHeap::heapifyDown(int idx) {
    int n = (int)heap.size();
    while (true) {
        int smallest = idx;
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;

        if (left < n && heap[left].form < heap[smallest].form)
            smallest = left;
        if (right < n && heap[right].form < heap[smallest].form)
            smallest = right;

        if (smallest != idx) {
            std::swap(heap[idx], heap[smallest]);
            idx = smallest;
        } else {
            break;
        }
    }
}

void MinHeap::buildHeap() {
    for (int i = (int)heap.size() / 2 - 1; i >= 0; i--) {
        heapifyDown(i);
    }
}

MinHeap::HeapEntry MinHeap::extractMin() {
    HeapEntry min = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) heapifyDown(0);
    return min;
}

void MinHeap::insert(const HeapEntry& entry) {
    heap.push_back(entry);
    heapifyUp((int)heap.size() - 1);
}

void MinHeap::replaceTop(const HeapEntry& entry) {
    heap[0] = entry;
    heapifyDown(0);
}

json MinHeap::getTopK(const std::vector<Player>& players, int k) {
    heap.clear();

    if (k <= 0 || players.empty()) return json::array();

    int n = (int)players.size();
    k = std::min(k, n);

    // Build min-heap of first K elements
    for (int i = 0; i < k; i++) {
        heap.push_back({players[i].recent_form, i});
    }
    buildHeap();

    // Process remaining elements
    for (int i = k; i < n; i++) {
        if (players[i].recent_form > heap[0].form) {
            replaceTop({players[i].recent_form, i});
        }
    }

    // Extract all K elements and sort by form descending
    std::vector<HeapEntry> topK;
    while (!heap.empty()) {
        topK.push_back(extractMin());
    }

    // Reverse to get descending order (highest form first)
    std::reverse(topK.begin(), topK.end());

    json result = json::array();
    for (int i = 0; i < (int)topK.size(); i++) {
        const Player& p = players[topK[i].playerIdx];
        json entry;
        entry["rank"] = i + 1;
        entry["id"] = p.id;
        entry["name"] = p.name;
        entry["role"] = p.role;
        entry["real_team"] = p.real_team;
        entry["credits"] = p.credits;
        entry["fantasy_points"] = p.fantasy_points;
        entry["recent_form"] = p.recent_form;
        result.push_back(entry);
    }

    return result;
}
