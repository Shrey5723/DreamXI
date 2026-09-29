#!/usr/bin/env python3
"""
Generate 200 fake fantasy cricket players for DreamXI.
Ensures at least one scenario where greedy (pick by points/credit ratio)
produces a worse team than the DP-optimal one.

Strategy for greedy failure:
- Create several high-ratio players that are expensive (11-12 credits)
- Create moderate-ratio players that are cheap (5-7 credits)
- Budget constraint (100 credits for 11 players) means greedy picks too many
  expensive players and runs out of budget, forcing cheap fillers.
- DP finds a better mix of expensive and moderate players.
"""
import json
import random

random.seed(42)

teams = ["India", "Australia"]
roles = ["wicketkeeper", "batter", "allrounder", "bowler"]

players = []
pid = 1

def add_player(name, role, team, credits, points, form):
    global pid
    players.append({
        "id": pid,
        "name": name,
        "role": role,
        "real_team": team,
        "credits": credits,
        "fantasy_points": points,
        "recent_form": round(form, 1)
    })
    pid += 1

# ---- HANDCRAFTED "GREEDY TRAP" PLAYERS ----
# These are specifically designed so greedy picks expensive high-ratio players
# but DP finds a better combination with cheaper alternatives.

# Expensive batters with decent ratio - greedy loves these
add_player("Virat Kohli", "batter", "India", 12, 95, 9.2)
add_player("Rohit Sharma", "batter", "India", 12, 94, 9.0)
add_player("KL Rahul", "batter", "India", 11, 88, 8.5)
add_player("Shubman Gill", "batter", "India", 11, 86, 8.8)
add_player("Shreyas Iyer", "batter", "India", 10, 82, 7.5)

# Cheaper batters with slightly lower ratio but much better credit efficiency
add_player("Suryakumar Yadav", "batter", "India", 7, 62, 8.0)
add_player("Ishan Kishan", "batter", "India", 6, 52, 7.2)

# Australian batters - mix of expensive and cheap
add_player("Steve Smith", "batter", "Australia", 11, 90, 8.8)
add_player("David Warner", "batter", "Australia", 11, 87, 8.2)
add_player("Marnus Labuschagne", "batter", "Australia", 10, 80, 8.0)
add_player("Travis Head", "batter", "Australia", 9, 75, 7.8)
add_player("Cameron Green", "batter", "Australia", 7, 58, 7.0)
add_player("Marcus Harris", "batter", "Australia", 6, 48, 6.5)

# Expensive bowlers - greedy trap
add_player("Jasprit Bumrah", "bowler", "India", 12, 92, 9.5)
add_player("Mohammed Shami", "bowler", "India", 11, 85, 8.5)
add_player("Mohammed Siraj", "bowler", "India", 10, 78, 8.0)

# Cheaper bowlers - DP prefers these to save credits
add_player("Shardul Thakur", "bowler", "India", 7, 58, 7.0)
add_player("Umesh Yadav", "bowler", "India", 6, 50, 6.2)
add_player("Navdeep Saini", "bowler", "India", 5, 38, 5.5)

# Australian bowlers
add_player("Pat Cummins", "bowler", "Australia", 12, 93, 9.3)
add_player("Mitchell Starc", "bowler", "Australia", 11, 88, 9.0)
add_player("Josh Hazlewood", "bowler", "Australia", 10, 80, 8.2)
add_player("Nathan Lyon", "bowler", "Australia", 8, 65, 7.5)
add_player("Scott Boland", "bowler", "Australia", 7, 55, 6.8)
add_player("Mitchell Swepson", "bowler", "Australia", 5, 35, 5.0)

# Wicketkeepers
add_player("Rishabh Pant", "wicketkeeper", "India", 10, 78, 8.5)
add_player("KS Bharat", "wicketkeeper", "India", 7, 52, 6.5)
add_player("Sanju Samson", "wicketkeeper", "India", 8, 60, 7.0)
add_player("Alex Carey", "wicketkeeper", "Australia", 9, 70, 7.5)
add_player("Josh Inglis", "wicketkeeper", "Australia", 7, 55, 6.8)
add_player("Matthew Wade", "wicketkeeper", "Australia", 6, 42, 6.0)

# All-rounders
add_player("Ravindra Jadeja", "allrounder", "India", 10, 82, 8.5)
add_player("Ravichandran Ashwin", "allrounder", "India", 9, 75, 8.0)
add_player("Axar Patel", "allrounder", "India", 7, 55, 7.0)
add_player("Washington Sundar", "allrounder", "India", 6, 45, 6.2)
add_player("Mitchell Marsh", "allrounder", "Australia", 10, 80, 8.0)
add_player("Glenn Maxwell", "allrounder", "Australia", 9, 72, 7.8)
add_player("Marcus Stoinis", "allrounder", "Australia", 7, 55, 6.5)
add_player("Ashton Agar", "allrounder", "Australia", 6, 42, 5.8)

# ---- GENERATED PLAYERS (fill up to 200) ----
first_names_ind = ["Rahul", "Amit", "Sanjay", "Vijay", "Ajay", "Raj", "Deepak", "Ankit",
                    "Nitin", "Pradeep", "Manish", "Ravi", "Sunil", "Anil", "Mohit",
                    "Arjun", "Karan", "Yash", "Dev", "Hari", "Pranav", "Gaurav",
                    "Rohan", "Varun", "Sachin", "Dinesh", "Piyush", "Tarun", "Vishal", "Kunal"]
last_names_ind = ["Kumar", "Singh", "Sharma", "Verma", "Gupta", "Patel", "Rao", "Mishra",
                   "Reddy", "Nair", "Joshi", "Pandey", "Yadav", "Chauhan", "Tiwari",
                   "Saxena", "Agarwal", "Bhatt", "Sinha", "Kapoor"]

first_names_aus = ["James", "Jack", "Liam", "Noah", "Oliver", "William", "Ben", "Tom",
                    "Daniel", "Luke", "Sam", "Ryan", "Chris", "Matt", "Josh",
                    "Adam", "Nathan", "Jake", "Ethan", "Max", "Harry", "Alex",
                    "Dylan", "Connor", "Finn", "Charlie", "Oscar", "Leo", "Hugo", "Zach"]
last_names_aus = ["Smith", "Jones", "Brown", "Wilson", "Taylor", "Anderson", "Thomas",
                   "Jackson", "White", "Harris", "Martin", "Thompson", "Clark", "Lewis",
                   "Walker", "Hall", "Young", "King", "Wright", "Scott"]

used_names = set()

def gen_name(team):
    while True:
        if team == "India":
            n = random.choice(first_names_ind) + " " + random.choice(last_names_ind)
        else:
            n = random.choice(first_names_aus) + " " + random.choice(last_names_aus)
        if n not in used_names:
            used_names.add(n)
            return n

# Role distribution for remaining players
role_weights = {
    "wicketkeeper": 0.12,
    "batter": 0.35,
    "allrounder": 0.18,
    "bowler": 0.35
}

remaining = 200 - len(players)
for _ in range(remaining):
    team = random.choice(teams)
    role = random.choices(list(role_weights.keys()), list(role_weights.values()))[0]
    name = gen_name(team)

    # Generate credits and points with some correlation
    credits = random.randint(5, 12)

    # Base points scales with credits but has noise
    base = credits * 6.5 + random.gauss(0, 8)
    points = max(20, min(100, int(base)))

    # Recent form
    form = max(1.0, min(10.0, random.gauss(6.0, 2.0)))

    add_player(name, role, team, credits, points, round(form, 1))

# Shuffle to avoid ordering bias
random.shuffle(players)

# Re-assign sequential IDs after shuffle
for i, p in enumerate(players):
    p["id"] = i + 1

# Print stats
print(f"Total players: {len(players)}")
for r in roles:
    count = sum(1 for p in players if p["role"] == r)
    print(f"  {r}: {count}")
for t in teams:
    count = sum(1 for p in players if p["real_team"] == t)
    print(f"  {t}: {count}")

# Verify greedy-trap scenario
# Sort all by ratio
sorted_by_ratio = sorted(players, key=lambda p: p["fantasy_points"] / p["credits"], reverse=True)
print("\nTop 20 by points/credit ratio:")
for p in sorted_by_ratio[:20]:
    ratio = p["fantasy_points"] / p["credits"]
    print(f"  {p['name']:25s} {p['role']:15s} {p['real_team']:12s} pts={p['fantasy_points']:3d} cr={p['credits']:2d} ratio={ratio:.2f}")

with open("backend/data/players.json", "w") as f:
    json.dump(players, f, indent=2)

print(f"\nData written to backend/data/players.json")
