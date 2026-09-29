# DreamXI: Multi-Constraint Team Optimizer with Custom Order-Statistic Tree Leaderboard

A Dream11-style fantasy sports team builder demonstrating advanced **Data Structures & Algorithms** concepts. Built with a **C++17 REST API backend** and **vanilla HTML/CSS/JS frontend**.

---

## 📚 Implemented Algorithms & Data Structures

| Feature | Algorithm / Data Structure | Complexity |
|---------|---------------------------|------------|
| **Optimal Team Selection** | Multi-constraint 0/1 Knapsack DP | O(N × 990K states) |
| **Quick Pick** | Greedy by points/credit ratio | O(N log N) |
| **Live Leaderboard** | Custom Augmented Treap (Order-Statistic Tree) | getRank: O(log N), getTopK: O(log N + K) |
| **Trending Players** | Binary Min-Heap (hand-built) | O(N log K) |
| **Friend Leagues** | DSU with Path Compression + Union-by-Rank | O(α(N)) ≈ O(1) |

---

## 🏗️ Project Structure

```
project/
├── backend/
│   ├── include/
│   │   ├── httplib.h              # cpp-httplib (header-only HTTP server)
│   │   ├── json.hpp               # nlohmann/json (header-only JSON)
│   │   ├── knapsack_solver.h      # DP knapsack header
│   │   ├── greedy_solver.h        # Greedy solver header
│   │   ├── rank_tree.h            # Custom Treap BST header
│   │   ├── min_heap.h             # Binary heap header
│   │   └── dsu.h                  # Disjoint Set Union header
│   ├── src/
│   │   ├── knapsack_solver.cpp    # Multi-constraint 0/1 Knapsack DP
│   │   ├── greedy_solver.cpp      # Greedy ratio-based selection
│   │   ├── rank_tree.cpp          # Augmented Treap (NO std::set/map)
│   │   ├── min_heap.cpp           # Custom binary min-heap
│   │   ├── dsu.cpp                # DSU with path compression
│   │   └── server.cpp             # REST API wiring (cpp-httplib)
│   └── data/
│       └── players.json           # 200 generated fantasy players
├── frontend/
│   ├── index.html                 # Single-page app with 6 tab views
│   ├── styles.css                 # Modern dark-theme design system
│   └── app.js                     # Frontend controller & API client
├── Makefile                       # Build system
├── generate_data.py               # Player data generator script
└── README.md                      # This file
```

---

## 🚀 How to Build & Run

### Prerequisites
- **macOS** with Xcode Command Line Tools (or any system with `clang++` supporting C++17)
- Python 3 (only needed to regenerate player data)

### Build & Launch
```bash
# 1. Clone or navigate to the project directory
cd project/

# 2. Build the C++ backend (compiles all modules + server)
make

# 3. Run the server on port 8080
./dreamxi_server 8080

# 4. Open the UI in your browser
open http://localhost:8080/index.html
```

### Regenerate Player Data (optional)
```bash
python3 generate_data.py
```

---

## 🖥️ Frontend Tabs

1. **Team Optimizer** — Side-by-side DP vs Greedy comparison with points, credits, time, captain/VC selection
2. **Player Pool** — Filterable/sortable grid of all 200 players (by role, team, points, credits, form)
3. **BST Leaderboard** — Live-updating augmented Treap leaderboard with rank lookup and score updates
4. **Heap Trending** — Top-K trending players by recent form via custom binary min-heap
5. **DSU Leagues** — Friend league clusters via Disjoint Set Union with merge and connectivity checks
6. **Complexity Benchmark** — Canvas chart plotting DP vs Greedy execution time across pool sizes

---

## 🔬 REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/players` | All 200 players with metadata |
| POST | `/build-team` | Run DP knapsack (returns optimal XI) |
| POST | `/quick-pick` | Run greedy selection (returns heuristic XI) |
| GET | `/leaderboard/top/{n}` | Top N users from Treap |
| GET | `/rank/{userId}` | User rank in O(log N) |
| POST | `/leaderboard/update` | Upsert user score in Treap |
| GET | `/trending/{k}` | Top K trending players from heap |
| POST | `/league/merge` | DSU unite two users |
| GET | `/league/same/{u1}/{u2}` | DSU connectivity check |
| GET | `/league/all` | All league clusters |
| GET | `/benchmark` | DP vs Greedy timing comparison |

---

## 🧪 Algorithm Details

### 1. Multi-Constraint 0/1 Knapsack (DP)
- **State**: `dp[credits(0-100)][wk(0-4)][bat(0-6)][ar(0-4)][bowl(0-6)][teamA(0-7)]`
- **Total states**: 989,800 — fits in ~2 MB
- **Backtracking**: Bitset-based keep array (~25 MB) to recover chosen players
- **Captain/VC**: Top-2 scorers from the XI get 2× and 1.5× multipliers
- Verified: DP scores **831 pts** vs Greedy's **607 pts** (27% optimality gap)

### 2. Greedy Quick Pick
- Two-phase: fill minimum role requirements first, then best-ratio remaining
- Much faster (~0.02 ms vs ~80 ms for DP) but suboptimal

### 3. Augmented Treap (Order-Statistic Tree)
- Custom BST with randomized heap priorities — **no `std::set` or `std::map`**
- Split/merge operations maintain balance in O(log N) expected time
- Subtree size augmentation enables order-statistic rank queries

### 4. Binary Min-Heap
- Maintains bounded-capacity K heap during single-pass scan
- All heap operations (insert, extractMin, heapifyDown) implemented from scratch

### 5. DSU (Union-Find)
- Path compression flattens trees during `find()`
- Union-by-rank keeps tree depth balanced
- Combined: amortized O(α(N)) per operation (inverse Ackermann)
# DreamXI
