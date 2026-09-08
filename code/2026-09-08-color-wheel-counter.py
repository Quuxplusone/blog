from math import factorial
import random

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

ROTATIONS = [
  [3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,31,32,33,34,35,36,37,38,39,30],
  [6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,32,33,34,35,36,37,38,39,30,31],
  [9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,33,34,35,36,37,38,39,30,31,32],
  [12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,34,35,36,37,38,39,30,31,32,33],
  [15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,35,36,37,38,39,30,31,32,33,34],
  [18,19,20,21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,36,37,38,39,30,31,32,33,34,35],
  [21,22,23,24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,37,38,39,30,31,32,33,34,35,36],
  [24,25,26,27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,38,39,30,31,32,33,34,35,36,37],
  [27,28,29,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,39,30,31,32,33,34,35,36,37,38],
  [29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,39,38,37,36,35,34,33,32,31,30],
  [26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,38,37,36,35,34,33,32,31,30,39],
  [23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,37,36,35,34,33,32,31,30,39,38],
  [20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,36,35,34,33,32,31,30,39,38,37],
  [17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,35,34,33,32,31,30,39,38,37,36],
  [14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,34,33,32,31,30,39,38,37,36,35],
  [11,10,9,8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,33,32,31,30,39,38,37,36,35,34],
  [8,7,6,5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,32,31,30,39,38,37,36,35,34,33],
  [5,4,3,2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,31,30,39,38,37,36,35,34,33,32],
  [2,1,0,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,30,39,38,37,36,35,34,33,32,31],
]

def color(piece):
  return piece[0]

def size(piece):
  return piece[1]

if __name__ == "__main__":
  bag = [c+s for c in 'abcdefg' for s in '123' for i in '123']
  permutationsInspected = 0
  validMidsFound = 0
  validSetupsFound = 0
  canonicalMidsFound = 0
  canonicalSetupsFound = 0
  t = 0
  while True:
    t += 1
    random.shuffle(bag)
    board = bag[:40]
    isSetup = not any(color(board[a]) == color(board[b]) for a,b in EDGES)
    isCanonical = not any(board < [board[r[i]] for i in range(40)] for r in ROTATIONS)
    # We shouldn't just increment permutationsInspected and validSetupsFound by 1;
    # inspecting all 63! permutations would then overcount the number of valid
    # setups by a factor of at least 23!. We should consider this discovery
    # to contribute only 1/k to validSetupsFound, where k is the number of permutations
    # that will contribute to this same setup's "bucket."
    # Equivalently, increment permutationsInspected by k and validSetupsFound by 1.
    k = factorial(23)
    for c in COLORS:
      for s in SIZES:
        p = c+s
        pcount = sum(1 if b == p else 0 for b in board)
        if pcount == 1:
          # This could be any of the 3 copies of the piece.
          k *= 3
        elif pcount == 2:
          k *= 6
        elif pcount == 3:
          k *= 6
    permutationsInspected += k
    canonicalSetupsFound += 1 if (isCanonical and isSetup) else 0
    validSetupsFound += 1 if (isSetup) else 0
    canonicalMidsFound += 1 if (isCanonical) else 0
    validMidsFound += 1

    if t % 100000 == 0:
      print("%d iterations: estimate\n valid setups: %.08e (%.08e canonical)\n midgame: %.08e (%.08e canonical)" % (
        t, validSetupsFound * factorial(63) / permutationsInspected,
        canonicalSetupsFound * factorial(63) / permutationsInspected,
        validMidsFound * factorial(63) / permutationsInspected,
        canonicalMidsFound * factorial(63) / permutationsInspected,
      ))
