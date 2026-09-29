/**
 * DreamXI — Frontend Engine Controller
 * Communicates with C++ REST API endpoints on port 8080 (or same origin).
 */

const API_BASE = window.location.origin.includes('localhost') || window.location.origin.includes('127.0.0.1')
  ? window.location.origin
  : 'http://localhost:8080';

// Global state cache
let state = {
  players: [],
  filteredPlayers: [],
  dpResult: null,
  greedyResult: null,
  leaderboardTopN: 15,
  trendingK: 10,
  leagues: []
};

// ==================== INITIALIZATION ====================
document.addEventListener('DOMContentLoaded', () => {
  setupNavigation();
  setupOptimizerEvents();
  setupPlayerPoolEvents();
  setupLeaderboardEvents();
  setupTrendingEvents();
  setupLeaguesEvents();
  setupBenchmarkEvents();

  // Load initial datasets
  loadPlayers();
  loadLeaderboard();
  loadTrending();
  loadLeagues();
});

// ==================== TAB NAVIGATION ====================
function setupNavigation() {
  const tabs = document.querySelectorAll('.nav-tab');
  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));

      tab.classList.add('active');
      const targetPane = document.getElementById(`pane-${tab.dataset.tab}`);
      if (targetPane) {
        targetPane.classList.add('active');
      }

      // If switching to benchmark, trigger render if data exists
      if (tab.dataset.tab === 'benchmark' && window.lastBenchmarkData) {
        drawBenchmarkChart(window.lastBenchmarkData);
      }
    });
  });
}

// ==================== TAB 1: BUILD TEAM (OPTIMIZER) ====================
function setupOptimizerEvents() {
  const btnRunBoth = document.getElementById('btn-run-both');
  const btnRunDP = document.getElementById('btn-run-dp');
  const btnRunGreedy = document.getElementById('btn-run-greedy');

  btnRunBoth.addEventListener('click', async () => {
    btnRunBoth.disabled = true;
    btnRunBoth.innerHTML = `<span class="btn-icon">⏳</span> Computing DP & Greedy...`;
    try {
      await Promise.all([runDP(), runGreedy()]);
      updateComparisonBanner();
    } finally {
      btnRunBoth.disabled = false;
      btnRunBoth.innerHTML = `<span class="btn-icon">⚡</span> Run Side-by-Side Comparison`;
    }
  });

  btnRunDP.addEventListener('click', async () => {
    btnRunDP.disabled = true;
    btnRunDP.innerHTML = `<span class="btn-icon">⏳</span> Computing...`;
    try {
      await runDP();
      if (state.greedyResult) updateComparisonBanner();
    } finally {
      btnRunDP.disabled = false;
      btnRunDP.innerHTML = `<span class="btn-icon">🎯</span> Run DP Only`;
    }
  });

  btnRunGreedy.addEventListener('click', async () => {
    btnRunGreedy.disabled = true;
    btnRunGreedy.innerHTML = `<span class="btn-icon">⏳</span> Computing...`;
    try {
      await runGreedy();
      if (state.dpResult) updateComparisonBanner();
    } finally {
      btnRunGreedy.disabled = false;
      btnRunGreedy.innerHTML = `<span class="btn-icon">🏃</span> Run Greedy Only`;
    }
  });
}

async function runDP() {
  try {
    const res = await fetch(`${API_BASE}/build-team`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({})
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    state.dpResult = data;
    renderTeamCard('dp', data);
  } catch (err) {
    console.error('DP failed:', err);
    alert('Failed to execute DP solver. Please verify the backend is running.');
  }
}

async function runGreedy() {
  try {
    const res = await fetch(`${API_BASE}/quick-pick`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({})
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    state.greedyResult = data;
    renderTeamCard('greedy', data);
  } catch (err) {
    console.error('Greedy failed:', err);
    alert('Failed to execute Greedy solver. Please verify the backend is running.');
  }
}

function renderTeamCard(prefix, data) {
  document.getElementById(`${prefix}-total-points`).textContent = data.total_points;
  document.getElementById(`${prefix}-final-score`).textContent = data.final_score.toFixed(1);
  document.getElementById(`${prefix}-credits`).textContent = `${data.credits_used}/100`;
  document.getElementById(`${prefix}-time`).textContent = `${data.time_ms.toFixed(2)} ms`;

  // Render role composition
  const roles = data.role_counts || {};
  document.getElementById(`${prefix}-composition`).innerHTML = `
    <span class="comp-item">🧤 WK: <strong>${roles.wicketkeeper || 0}</strong></span>
    <span class="comp-item">🏏 BAT: <strong>${roles.batter || 0}</strong></span>
    <span class="comp-item">⚡ AR: <strong>${roles.allrounder || 0}</strong></span>
    <span class="comp-item">🎯 BOWL: <strong>${roles.bowler || 0}</strong></span>
  `;

  // Render roster
  const rosterContainer = document.getElementById(`${prefix}-roster`);
  rosterContainer.innerHTML = '';

  data.players.forEach(p => {
    const isCap = p.is_captain;
    const isVc = p.is_vice_captain;

    let roleClass = 'role-bat';
    let roleShort = 'BAT';
    if (p.role === 'wicketkeeper') { roleClass = 'role-wk'; roleShort = 'WK'; }
    else if (p.role === 'allrounder') { roleClass = 'role-ar'; roleShort = 'AR'; }
    else if (p.role === 'bowler') { roleClass = 'role-bowl'; roleShort = 'BOWL'; }

    const item = document.createElement('div');
    item.className = `roster-card ${isCap ? 'is-captain' : ''} ${isVc ? 'is-vc' : ''}`;
    item.innerHTML = `
      <div class="player-left">
        <span class="role-badge ${roleClass}">${roleShort}</span>
        <div class="player-name-block">
          <div class="player-name">
            ${escapeHtml(p.name)}
            ${isCap ? '<span class="tag-captain">C (2x)</span>' : ''}
            ${isVc ? '<span class="tag-vc">VC (1.5x)</span>' : ''}
          </div>
          <div class="player-team-sub">${escapeHtml(p.real_team)} • Form: ${p.recent_form.toFixed(1)}</div>
        </div>
      </div>
      <div class="player-right">
        <span class="p-credits">${p.credits} cr</span>
        <span class="p-pts">${p.effective_points.toFixed(0)} pts</span>
      </div>
    `;
    rosterContainer.appendChild(item);
  });
}

function updateComparisonBanner() {
  if (!state.dpResult || !state.greedyResult) return;

  const dpScore = state.dpResult.total_points;
  const greedyScore = state.greedyResult.total_points;
  const diff = dpScore - greedyScore;
  const gapPct = ((diff / dpScore) * 100).toFixed(1);

  const banner = document.getElementById('comparison-banner');
  const heading = document.getElementById('banner-heading');
  const subtext = document.getElementById('banner-subtext');

  banner.classList.remove('hidden');

  document.getElementById('banner-dp-score').textContent = dpScore;
  document.getElementById('banner-greedy-score').textContent = greedyScore;
  document.getElementById('banner-gap').textContent = `+${diff} pts (+${gapPct}%)`;

  if (diff > 0) {
    heading.textContent = `🎯 Dynamic Programming Beats Greedy by +${diff} Fantasy Points!`;
    subtext.innerHTML = `The Greedy heuristic prioritized high points-per-credit ratios and ran out of budget/slots prematurely (used <strong>${state.greedyResult.credits_used} credits</strong>, score: ${greedyScore}), whereas 0/1 Knapsack DP found the global optimal combination (used <strong>${state.dpResult.credits_used} credits</strong>, score: ${dpScore}).`;
  } else {
    heading.textContent = `Both Solvers Reached Equivalent Points (${dpScore} pts)`;
    subtext.textContent = `In this particular configuration, the greedy heuristic happened to match the DP optimal score.`;
  }
}

// ==================== TAB 2: PLAYER POOL ====================
async function loadPlayers() {
  try {
    const res = await fetch(`${API_BASE}/players`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    state.players = await res.json();
    filterAndRenderPlayers();
  } catch (err) {
    console.error('Failed to load players:', err);
    document.getElementById('pool-table-body').innerHTML = `
      <tr><td colspan="8" class="text-center" style="color: #f87171;">Failed to connect to backend server at ${API_BASE}.</td></tr>
    `;
  }
}

function setupPlayerPoolEvents() {
  document.getElementById('filter-role').addEventListener('change', filterAndRenderPlayers);
  document.getElementById('filter-team').addEventListener('change', filterAndRenderPlayers);
  document.getElementById('sort-by').addEventListener('change', filterAndRenderPlayers);
  document.getElementById('search-player').addEventListener('input', filterAndRenderPlayers);
}

function filterAndRenderPlayers() {
  const roleFilter = document.getElementById('filter-role').value;
  const teamFilter = document.getElementById('filter-team').value;
  const sortBy = document.getElementById('sort-by').value;
  const searchQuery = document.getElementById('search-player').value.toLowerCase().trim();

  let list = [...state.players];

  if (roleFilter !== 'all') {
    list = list.filter(p => p.role === roleFilter);
  }
  if (teamFilter !== 'all') {
    list = list.filter(p => p.real_team === teamFilter);
  }
  if (searchQuery) {
    list = list.filter(p => p.name.toLowerCase().includes(searchQuery));
  }

  // Sorting
  list.sort((a, b) => {
    if (sortBy === 'ratio-desc') return (b.fantasy_points / b.credits) - (a.fantasy_points / a.credits);
    if (sortBy === 'points-desc') return b.fantasy_points - a.fantasy_points;
    if (sortBy === 'credits-desc') return b.credits - a.credits;
    if (sortBy === 'credits-asc') return a.credits - b.credits;
    if (sortBy === 'form-desc') return b.recent_form - a.recent_form;
    return 0;
  });

  state.filteredPlayers = list;
  document.getElementById('pool-count').textContent = list.length;

  const tbody = document.getElementById('pool-table-body');
  if (list.length === 0) {
    tbody.innerHTML = `<tr><td colspan="8" class="text-center">No players match the selected filters.</td></tr>`;
    return;
  }

  tbody.innerHTML = list.map(p => {
    let roleClass = 'role-bat';
    let roleShort = 'BAT';
    if (p.role === 'wicketkeeper') { roleClass = 'role-wk'; roleShort = 'WK'; }
    else if (p.role === 'allrounder') { roleClass = 'role-ar'; roleShort = 'AR'; }
    else if (p.role === 'bowler') { roleClass = 'role-bowl'; roleShort = 'BOWL'; }

    const ratio = (p.fantasy_points / p.credits).toFixed(2);

    return `
      <tr>
        <td style="font-family: var(--font-mono); color: var(--text-muted);">${p.id}</td>
        <td><strong>${escapeHtml(p.name)}</strong></td>
        <td><span class="role-badge ${roleClass}">${roleShort}</span></td>
        <td>${escapeHtml(p.real_team)}</td>
        <td style="font-family: var(--font-mono); font-weight: 600;">${p.credits}</td>
        <td style="font-family: var(--font-mono); font-weight: 700; color: var(--accent-cyan-light);">${p.fantasy_points}</td>
        <td style="font-family: var(--font-mono); color: var(--dp-green-light); font-weight: 700;">${ratio}</td>
        <td>
          <span style="font-family: var(--font-mono); font-weight: 600;">${p.recent_form.toFixed(1)}</span>
        </td>
      </tr>
    `;
  }).join('');
}

// ==================== TAB 3: LEADERBOARD (AUGMENTED TREAP) ====================
function setupLeaderboardEvents() {
  document.getElementById('select-top-n').addEventListener('change', (e) => {
    state.leaderboardTopN = parseInt(e.target.value);
    loadLeaderboard();
  });

  document.getElementById('btn-lookup-rank').addEventListener('click', async () => {
    const uid = document.getElementById('input-lookup-id').value;
    const output = document.getElementById('lookup-result');
    if (!uid) return;

    output.className = 'lookup-output';
    output.textContent = 'Querying Treap...';

    try {
      const res = await fetch(`${API_BASE}/rank/${uid}`);
      if (res.status === 404) {
        output.className = 'lookup-output error';
        output.textContent = `User ID ${uid} not found in Treap`;
        return;
      }
      const data = await res.json();
      output.className = 'lookup-output success';
      output.textContent = `🎯 ${data.userName} (ID: ${data.userId}) is Rank #${data.rank} of ${data.total_users} users with Score ${data.score}!`;
    } catch (err) {
      output.className = 'lookup-output error';
      output.textContent = `Lookup error: ${err.message}`;
    }
  });

  document.getElementById('btn-upsert-score').addEventListener('click', async () => {
    const uid = parseInt(document.getElementById('input-upsert-id').value);
    const name = document.getElementById('input-upsert-name').value;
    const score = parseInt(document.getElementById('input-upsert-score').value);
    const output = document.getElementById('upsert-result');

    if (!uid || !score) return;

    output.className = 'lookup-output';
    output.textContent = 'Updating Treap in O(log N)...';

    try {
      const res = await fetch(`${API_BASE}/leaderboard/update`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ userId: uid, userName: name, score: score })
      });
      const data = await res.json();
      output.className = 'lookup-output success';
      output.textContent = `✅ Upserted ${data.userName}: New Rank #${data.rank} with Score ${data.score}!`;
      await loadLeaderboard();
      highlightUserRow(uid);
    } catch (err) {
      output.className = 'lookup-output error';
      output.textContent = `Upsert error: ${err.message}`;
    }
  });

  // Live simulation button
  document.getElementById('btn-simulate-live').addEventListener('click', async () => {
    const btn = document.getElementById('btn-simulate-live');
    btn.disabled = true;
    btn.innerHTML = `<span class="btn-icon">⚡</span> Simulating Live Matches...`;

    // Pick 3 random users and boost their scores
    const randomBoosts = [
      { id: 104, name: "Karan Singh", delta: Math.floor(Math.random() * 50) + 30 },
      { id: 107, name: "Sneha Reddy", delta: Math.floor(Math.random() * 60) + 40 },
      { id: 111, name: "Ishaan Nair", delta: Math.floor(Math.random() * 70) + 50 }
    ];

    try {
      for (const item of randomBoosts) {
        // Query current score first
        let curScore = 800;
        try {
          const r = await fetch(`${API_BASE}/rank/${item.id}`);
          if (r.ok) {
            const info = await r.json();
            curScore = info.score;
          }
        } catch (_) {}

        await fetch(`${API_BASE}/leaderboard/update`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            userId: item.id,
            userName: item.name,
            score: curScore + item.delta
          })
        });
      }

      await loadLeaderboard();
      // Highlight updated rows
      randomBoosts.forEach(b => highlightUserRow(b.id));
    } catch (err) {
      console.error('Simulation error:', err);
    } finally {
      btn.disabled = false;
      btn.innerHTML = `<span class="btn-icon">⚡</span> Simulate Live Score Updates`;
    }
  });
}

async function loadLeaderboard() {
  try {
    const res = await fetch(`${API_BASE}/leaderboard/top/${state.leaderboardTopN}`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    renderLeaderboard(data.top);
  } catch (err) {
    console.error('Failed to load leaderboard:', err);
  }
}

function renderLeaderboard(list) {
  const tbody = document.getElementById('leaderboard-table-body');
  if (!list || list.length === 0) {
    tbody.innerHTML = `<tr><td colspan="5" class="text-center">No leaderboard entries.</td></tr>`;
    return;
  }

  tbody.innerHTML = list.map(item => {
    let rankBadgeClass = 'rank-other';
    if (item.rank === 1) rankBadgeClass = 'rank-gold';
    else if (item.rank === 2) rankBadgeClass = 'rank-silver';
    else if (item.rank === 3) rankBadgeClass = 'rank-bronze';

    return `
      <tr id="lb-row-${item.userId}">
        <td>
          <span class="rank-badge ${rankBadgeClass}">${item.rank}</span>
        </td>
        <td style="font-family: var(--font-mono); color: var(--text-muted);">${item.userId}</td>
        <td><strong>${escapeHtml(item.userName)}</strong></td>
        <td style="font-family: var(--font-mono); font-weight: 700; color: var(--accent-cyan-light); font-size: 1rem;">
          ${item.score}
        </td>
        <td>
          <span style="font-size: 0.75rem; color: var(--dp-green-light); background: rgba(16, 185, 129, 0.1); padding: 0.2rem 0.5rem; border-radius: var(--radius-sm);">
            Treap Node Balanced
          </span>
        </td>
      </tr>
    `;
  }).join('');
}

function highlightUserRow(userId) {
  const row = document.getElementById(`lb-row-${userId}`);
  if (row) {
    row.classList.remove('row-updated');
    void row.offsetWidth; // trigger reflow
    row.classList.add('row-updated');
  }
}

// ==================== TAB 4: TRENDING (BINARY HEAP) ====================
function setupTrendingEvents() {
  document.getElementById('select-k').addEventListener('change', (e) => {
    state.trendingK = parseInt(e.target.value);
    loadTrending();
  });
}

async function loadTrending() {
  try {
    const res = await fetch(`${API_BASE}/trending/${state.trendingK}`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    document.getElementById('heap-time').textContent = `Heap Time: ${data.time_ms.toFixed(3)} ms (O(N log ${data.k}))`;
    renderTrending(data.players);
  } catch (err) {
    console.error('Failed to load trending:', err);
  }
}

function renderTrending(players) {
  const container = document.getElementById('trending-grid');
  if (!players || players.length === 0) {
    container.innerHTML = `<div class="text-center" style="grid-column: 1/-1;">No trending players available.</div>`;
    return;
  }

  container.innerHTML = players.map(p => {
    const pct = Math.min(100, Math.max(0, (p.recent_form / 10.0) * 100));

    let roleClass = 'role-bat';
    let roleShort = 'BAT';
    if (p.role === 'wicketkeeper') { roleClass = 'role-wk'; roleShort = 'WK'; }
    else if (p.role === 'allrounder') { roleClass = 'role-ar'; roleShort = 'AR'; }
    else if (p.role === 'bowler') { roleClass = 'role-bowl'; roleShort = 'BOWL'; }

    return `
      <div class="trending-card">
        <div class="trending-top-row">
          <span class="trending-rank">#${p.rank} TRENDING</span>
          <span class="role-badge ${roleClass}">${roleShort}</span>
        </div>
        <div>
          <div style="font-weight: 700; font-size: 1.05rem;">${escapeHtml(p.name)}</div>
          <div style="font-size: 0.78rem; color: var(--text-muted);">${escapeHtml(p.real_team)} • ${p.credits} Credits • ${p.fantasy_points} Pts</div>
        </div>
        <div class="form-bar-container">
          <div class="form-track">
            <div class="form-fill" style="width: ${pct}%;"></div>
          </div>
          <span class="form-score">⭐ ${p.recent_form.toFixed(1)}</span>
        </div>
      </div>
    `;
  }).join('');
}

// ==================== TAB 5: LEAGUES (DSU) ====================
function setupLeaguesEvents() {
  document.getElementById('btn-check-same').addEventListener('click', async () => {
    const u1 = document.getElementById('input-same-u1').value;
    const u2 = document.getElementById('input-same-u2').value;
    const output = document.getElementById('same-result');

    if (!u1 || !u2) return;

    output.className = 'lookup-output';
    output.textContent = 'Querying DSU find(u1) == find(u2)...';

    try {
      const res = await fetch(`${API_BASE}/league/same/${u1}/${u2}`);
      const data = await res.json();

      if (data.same_league) {
        output.className = 'lookup-output success';
        output.textContent = `✅ Connected! User ${u1} and User ${u2} belong to the SAME league cluster!`;
      } else {
        output.className = 'lookup-output error';
        output.textContent = `❌ Separate Leagues: User ${u1} and User ${u2} are in distinct disjoint sets.`;
      }
    } catch (err) {
      output.className = 'lookup-output error';
      output.textContent = `Error: ${err.message}`;
    }
  });

  document.getElementById('btn-merge-leagues').addEventListener('click', async () => {
    const u1 = parseInt(document.getElementById('input-merge-u1').value);
    const u2 = parseInt(document.getElementById('input-merge-u2').value);
    const output = document.getElementById('merge-result');

    if (!u1 || !u2) return;

    output.className = 'lookup-output';
    output.textContent = 'Executing DSU unite(u1, u2)...';

    try {
      const res = await fetch(`${API_BASE}/league/merge`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ userA: u1, userB: u2 })
      });
      const data = await res.json();

      output.className = 'lookup-output success';
      if (data.merged) {
        output.textContent = `🤝 Successfully united User ${u1} and User ${u2} into the same league!`;
      } else {
        output.textContent = `ℹ️ Users ${u1} and ${u2} were already in the same league.`;
      }

      await loadLeagues();
    } catch (err) {
      output.className = 'lookup-output error';
      output.textContent = `Merge error: ${err.message}`;
    }
  });
}

async function loadLeagues() {
  try {
    const res = await fetch(`${API_BASE}/league/all`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    state.leagues = data.leagues || [];
    renderLeagues(state.leagues);
  } catch (err) {
    console.error('Failed to load leagues:', err);
  }
}

function renderLeagues(leagues) {
  document.getElementById('league-count-badge').textContent = `${leagues.length} Disjoint Sets`;

  const container = document.getElementById('league-clusters');
  if (!leagues || leagues.length === 0) {
    container.innerHTML = `<div class="text-center" style="grid-column: 1/-1;">No leagues registered.</div>`;
    return;
  }

  container.innerHTML = leagues.map((lg, idx) => {
    const membersHtml = lg.members.map(m => `
      <span class="member-chip">
        <span>${escapeHtml(m.userName)}</span>
        <span class="member-uid">#${m.userId}</span>
      </span>
    `).join('');

    return `
      <div class="league-card">
        <div class="league-card-header">
          <div class="league-name">
            <span>🛡️</span> League Set #${lg.league_id}
          </div>
          <span class="league-size">${lg.size} Member${lg.size > 1 ? 's' : ''}</span>
        </div>
        <div class="member-chips-wrap">
          ${membersHtml}
        </div>
      </div>
    `;
  }).join('');
}

// ==================== TAB 6: BENCHMARK ====================
function setupBenchmarkEvents() {
  const btn = document.getElementById('btn-run-benchmark');
  btn.addEventListener('click', async () => {
    btn.disabled = true;
    btn.innerHTML = `<span class="btn-icon">⏳</span> Executing Benchmarks...`;

    try {
      const res = await fetch(`${API_BASE}/benchmark`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      window.lastBenchmarkData = data.benchmark;
      renderBenchmarkTable(data.benchmark);
      drawBenchmarkChart(data.benchmark);
    } catch (err) {
      console.error('Benchmark failed:', err);
      alert('Benchmark execution failed.');
    } finally {
      btn.disabled = false;
      btn.innerHTML = `<span class="btn-icon">▶️</span> Run Live Benchmark`;
    }
  });
}

function renderBenchmarkTable(benchmark) {
  const tbody = document.getElementById('benchmark-table-body');
  if (!benchmark || benchmark.length === 0) return;

  tbody.innerHTML = benchmark.map(b => {
    return `
      <tr>
        <td style="font-family: var(--font-mono); font-weight: 700;">N = ${b.candidate_count}</td>
        <td style="font-family: var(--font-mono); color: var(--dp-green-light);">${b.dp_time_ms.toFixed(2)} ms</td>
        <td style="font-family: var(--font-mono); font-weight: 700; color: var(--accent-cyan-light);">${b.dp_points}</td>
        <td style="font-family: var(--font-mono); color: var(--greedy-amber-light);">${b.greedy_time_ms.toFixed(3)} ms</td>
        <td style="font-family: var(--font-mono);">${b.greedy_points}</td>
        <td style="font-family: var(--font-mono); font-weight: 700; color: #f87171;">+${b.optimality_gap_percent.toFixed(1)}%</td>
      </tr>
    `;
  }).join('');
}

function drawBenchmarkChart(data) {
  const canvas = document.getElementById('benchmark-chart');
  if (!canvas || !data || data.length === 0) return;

  const ctx = canvas.getContext('2d');
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();

  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  const width = rect.width;
  const height = rect.height;

  // Clear background
  ctx.clearRect(0, 0, width, height);

  const padding = { top: 20, right: 30, bottom: 40, left: 60 };
  const chartW = width - padding.left - padding.right;
  const chartH = height - padding.top - padding.bottom;

  // Max value calculation
  const maxTime = Math.max(...data.map(d => d.dp_time_ms), ...data.map(d => d.greedy_time_ms), 10);
  const yMax = Math.ceil(maxTime * 1.15);

  // Draw grid lines
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
  ctx.lineWidth = 1;
  const yTicks = 5;
  for (let i = 0; i <= yTicks; i++) {
    const yVal = (yMax / yTicks) * i;
    const yPos = padding.top + chartH - (yVal / yMax) * chartH;
    ctx.beginPath();
    ctx.moveTo(padding.left, yPos);
    ctx.lineTo(padding.left + chartW, yPos);
    ctx.stroke();

    ctx.fillStyle = '#6b7280';
    ctx.font = '10px JetBrains Mono, monospace';
    ctx.textAlign = 'right';
    ctx.fillText(`${yVal.toFixed(0)} ms`, padding.left - 8, yPos + 3);
  }

  // Draw X Axis labels
  const nPoints = data.length;
  const xStep = chartW / (nPoints - 1 || 1);

  data.forEach((d, idx) => {
    const xPos = padding.left + idx * xStep;
    ctx.fillStyle = '#9ca3af';
    ctx.font = '11px Outfit, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(`N=${d.candidate_count}`, xPos, height - padding.bottom + 20);
  });

  // Function to draw line series
  function drawSeries(points, color, fillColor) {
    if (points.length === 0) return;

    ctx.beginPath();
    points.forEach((pt, idx) => {
      if (idx === 0) ctx.moveTo(pt.x, pt.y);
      else ctx.lineTo(pt.x, pt.y);
    });
    ctx.strokeStyle = color;
    ctx.lineWidth = 3;
    ctx.stroke();

    // Area fill
    if (fillColor) {
      ctx.lineTo(points[points.length - 1].x, padding.top + chartH);
      ctx.lineTo(points[0].x, padding.top + chartH);
      ctx.closePath();
      ctx.fillStyle = fillColor;
      ctx.fill();
    }

    // Draw dots
    points.forEach(pt => {
      ctx.beginPath();
      ctx.arc(pt.x, pt.y, 5, 0, Math.PI * 2);
      ctx.fillStyle = color;
      ctx.fill();
      ctx.lineWidth = 2;
      ctx.strokeStyle = '#111827';
      ctx.stroke();
    });
  }

  // Calculate DP Points
  const dpPoints = data.map((d, idx) => ({
    x: padding.left + idx * xStep,
    y: padding.top + chartH - (d.dp_time_ms / yMax) * chartH
  }));

  // Calculate Greedy Points
  const greedyPoints = data.map((d, idx) => ({
    x: padding.left + idx * xStep,
    y: padding.top + chartH - (d.greedy_time_ms / yMax) * chartH
  }));

  // Draw DP
  drawSeries(dpPoints, '#10b981', 'rgba(16, 185, 129, 0.08)');
  // Draw Greedy
  drawSeries(greedyPoints, '#f59e0b', 'rgba(245, 158, 11, 0.05)');
}

// Utility: escape HTML strings
function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}
