import random
import statistics
from collections import deque

COLORS = "abcdefg"
SIZES = "123"

# The 40 board positions, numbered 0 through 39.
#
# Positions 0..29 are the 30 outer/side spaces.
# Positions 30..39 are the 10 inner spaces.
NEIGHBORS = [
  [29, 1],
  [0, 2, 30],
  [1, 3],
  [2, 4],
  [3, 5, 31],
  [4, 6],
  [5, 7],
  [6, 8, 32],
  [7, 9],
  [8, 10],
  [9, 11, 33],
  [10, 12],
  [11, 13],
  [12, 14, 34],
  [13, 15],
  [14, 16],
  [15, 17, 35],
  [16, 18],
  [17, 19],
  [18, 20, 36],
  [19, 21],
  [20, 22],
  [21, 23, 37],
  [22, 24],
  [23, 25],
  [24, 26, 38],
  [25, 27],
  [26, 28],
  [27, 29, 39],
  [28, 0],
  [39, 31, 1],
  [30, 32, 4],
  [31, 33, 7],
  [32, 34, 10],
  [33, 35, 13],
  [34, 36, 16],
  [35, 37, 19],
  [36, 38, 22],
  [37, 39, 25],
  [38, 30, 28],
]
EDGES = [(a,b) for a in range(40) for b in NEIGHBORS[a] if a < b]


# ============================================================
# Pieces
# ============================================================

def color(piece):
  return piece[0]

def size(piece):
  return piece[1]

def parse_state(text):
  state = tuple(text.split())
  assert len(state) == 40
  for piece in state:
    assert len(piece) == 2 and piece[0] in COLORS and piece[1] in SIZES
  return state

def print_board(state):
  return " ".join(state)


# ============================================================
# Components
# ============================================================

def components(state):
  """
  Return the connected components, grouped by color.
  result[c] is a list of sets of positions.
  """

  result = {c: [] for c in COLORS}
  seen = set()

  for start in range(40):
    if start in seen:
      continue

    c = color(state[start])
    component = set()
    queue = [start]
    seen.add(start)

    while queue:
      p = queue.pop()
      component.add(p)
      for n in NEIGHBORS[p]:
        if n not in seen and color(state[n]) == c:
          seen.add(n)
          queue.append(n)
    result[c].append(component)
  return result


def component_count(state):
  return sum(len(x) for x in components(state).values())


def is_solved(state):
  return all(len(x) <= 1 for x in components(state).values())


def color_positions(state, c):
  return {i for i, piece in enumerate(state) if color(piece) == c}


# ============================================================
# Moves
# ============================================================

def legal_move(state, a, b):
  return color(state[a]) == color(state[b]) or size(state[a]) == size(state[b])

def legal_moves(state):
  for a in range(40):
    for b in range(a + 1, 40):
      if legal_move(state, a, b):
        yield a, b

def swap(state, a, b):
  state = list(state)
  state[a], state[b] = state[b], state[a]
  return tuple(state)


# ============================================================
# Phase 1: greedy component reduction
# ============================================================

def greedy_merge(state, route):
  """
  Keep making moves that reduce the number of color components.
  """
  while True:
    before = component_count(state)
    candidates = []
    for a, b in legal_moves(state):
      new_state = swap(state, a, b)
      after = component_count(new_state)
      if after >= before:
        continue
      same_edges = sum(
        color(new_state[x]) == color(new_state[y])
        for x, y in EDGES
      )
      candidates.append((after, -same_edges, a, b, new_state))
    if not candidates:
      return state
    candidates.sort(key=lambda x: x[:2])
    _, _, a, b, state = candidates[0]
    route.append((a, b))


# ============================================================
# Target-region construction
# ============================================================

def grow_region(start, wanted, preferred, forbidden):
  """
  Randomly grow a connected region of exactly `wanted` cells.
  Cells in `preferred` are favored because those are cells
  currently occupied by the color we're trying to place.
  """
  region = {start}
  while len(region) < wanted:
    candidates = set()
    for p in region:
      for n in NEIGHBORS[p]:
        if n not in region and n not in forbidden:
          candidates.add(n)
    if not candidates:
      return None
    candidates = list(candidates)
    # Prefer cells that currently contain this color.
    weights = [20 if p in preferred else 1 for p in candidates]
    p = random.choices(candidates, weights=weights, k=1)[0]
    region.add(p)
  return region


def make_target_partition(state):
  """
  Try to partition the 40 cells into connected regions,
  one region for each represented color.
  The size of each region equals the number of pieces of
  that color.
  Among many random partitions, keep the one that preserves
  the greatest number of already-correct pieces.
  """
  represented = [
    c for c in COLORS
    if color_positions(state, c)
  ]
  counts = {
    c: len(color_positions(state, c))
    for c in represented
  }
  best_targets = None
  best_score = -1
  attempt = 0
  while best_targets is None or attempt < 500:
    attempt += 1
    # Put difficult colors first.  Large regions are also
    # handled first because they are harder to fit.
    order = represented[:]
    random.shuffle(order)
    order.sort(
      key=lambda c: -counts[c]
    )

    targets = {}
    used = set()
    success = True
    for c in order:
      wanted = counts[c]
      preferred = color_positions(state, c)
      # Try several possible starting cells.  Prefer a
      # cell that already has the desired color.
      starts = list(set(range(40)) - used)
      random.shuffle(starts)
      preferred_starts = [p for p in starts if p in preferred]
      starts = preferred_starts + starts
      region = None
      for start in starts[:30]:
        region = grow_region(start, wanted, preferred, used)
        if region is not None:
          break
      if region is None:
        success = False
        break
      targets[c] = region
      used.update(region)
    if (not success) or len(used) != 40:
      continue
    score = sum(
      sum(color(state[p]) == c for p in region)
      for c, region in targets.items()
    )
    if score > best_score:
      best_score = score
      best_targets = targets
      if best_score <= 50:
        break
  return best_targets


# ============================================================
# Phase 3: selection sort into target regions
# ============================================================

def unconstrained_swap(state, a, b, found_moves):
  """
  Return a sequence of legal moves that exchanges the pieces
  currently at positions a and b, in 1, 3, or at most 5 moves.
  """
  if found_moves <= 1:
    return [1,1,1,1,1,1]

  if legal_move(state, a, b):
    return [(a, b)]

  if found_moves <= 3:
    return [1,1,1,1,1,1]

  def find_corner(state, a, b):
    for p, piece in enumerate(state):
      if legal_move(state, a, p) and legal_move(state, p, b):
        return p
    return None

  corner = find_corner(state, a, b)
  if corner is not None:
    return [
      (a, corner),
      (a, b),
      (b, corner),
    ]

  if found_moves <= 5:
    return [1,1,1,1,1,1]

  def find_third_and_fourth_pieces(state, a, b):
    for d, pd in enumerate(state):
      if d in [a, b] or not legal_move(state, a, d):
        continue
      for e, pe in enumerate(state):
        if e in [a, b, d] or not legal_move(state, d, e) or not legal_move(state, e, b):
          continue
        return (d, e)
    assert False

  d, e = find_third_and_fourth_pieces(state, a, b)
  return [
    (a, d),
    (b, e),
    (a, b),
    (a, e),
    (b, d),
  ]


def move_colors_to_targets(state, targets, route):
  for c, target in targets.items():
    target = set(target)
    for target_cell in target:
      if color(state[target_cell]) == c:
        continue
      # Find a c-piece that is outside this target.
      source_cells = [p for p in color_positions(state, c) if p not in target]
      moves = [1,1,1,1,1,1]  # fake, just to get len(moves) == 6
      for p in source_cells:
        pmoves = unconstrained_swap(state, p, target_cell, len(moves))
        if len(pmoves) < len(moves):
          moves = pmoves
      for a, b in moves:
        assert legal_move(state, a, b)
        state = swap(state, a, b)
      route += moves
  return state

# ============================================================
# Complete heuristic solver
# ============================================================


def solve(initial, attempts=20):
  initial = tuple(initial)
  initial_route = []
  initial = greedy_merge(initial, initial_route)
  print("After greedy phase: %d components, %d moves taken" % (component_count(initial), len(initial_route)))

  best_route = None
  for attempt in range(attempts):
    state = initial
    route = initial_route[:]
    if not is_solved(state):
      targets = make_target_partition(state)
      assert targets is not None
      state = move_colors_to_targets(state, targets, route)

    assert is_solved(state)
    if best_route is None or len(route) < len(best_route):
      best_route = route[:]

  return best_route


# ============================================================
# Main
# ============================================================

if __name__ == "__main__":
  successes = []
  failures = 0
  for i in range(1000):
    bag = [c+s for c in 'abcdefg' for s in '123' for i in '123']
    while True:
      random.shuffle(bag)
      initial = parse_state(' '.join(bag[:40]))
      if component_count(initial) == 40:
        break

    # initial = parse_state('c2 b1 c2 b3 f2 b3 c1 d3 a3 b2 g1 f1 c1 g2 f1 d1 f1 b1 e1 d3 f2 g2 c3 a1 e1 c3 g3 d1 b1 a2 e2 b3 g3 e1 d2 c2 g1 f3 g3 f3')

    print("Initial board:", print_board(initial))
    print("Initial components:", component_count(initial))

    route = solve(
      initial,
      attempts=20
    )

    if route is None:
      print("No solution found by the heuristic.")
      failures += 1
    else:
      successes += [len(route)]
      print("=" * 60)
      print(f"BEST SOLUTION: {len(route)} moves")
      print("=" * 60)
      state = initial
      for number, (a, b) in enumerate(route, 1):
        print("%2d. swap %02d (%s) <-> %02d (%s)" % (number, a, state[a], b, state[b]))
        state = swap(state, a, b)

      print("Solved:", is_solved(state))
      print("Final components:", component_count(state))

    print("So far: %d/%d solved, in %.02f moves (std=%.02f), worst case %d moves\n%r" % (len(successes), len(successes)+failures, statistics.mean(successes), statistics.stdev(successes) if len(successes) >= 3 else 0.0, max(successes), successes))
