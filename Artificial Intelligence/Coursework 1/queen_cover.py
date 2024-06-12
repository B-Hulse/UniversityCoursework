# Definition of Queen Cover problem
# For use with queue_search.py

from __future__ import print_function
from copy import deepcopy


# State:
# The state is an array of (x,y) tuples, one entry for each queen on the board.
# This is similar to storing a sparce matrix, so when I refer to the 'board' I refer to a theoretical board
# in which every entry whos coordiates are a tuple in 'states' is 1 and all other entries are 0

# A 3x3 board state with two queens would look like
# [(0,1),(1,2)]

def qc_get_initial_state(x, y):
    # The state is simply an array so initially it is an empty array
    return ([])

def qc_possible_actions(state):
    moves = []
    # Check every space on the 'board' and add it to moves if there is not a queen with that positions coordinates
    for i in range(BOARD_X):
        for j in range(BOARD_Y):
            if [i,j] not in state: # Pythonic or what?
                moves += [[i,j]]
    return moves

def qc_successor_state( action, state ):
    queens = deepcopy(state)
    queens += [[action[0],action[1]]]
    return (queens)       

def qc_goal_state( state ):
    # For each position on the 'board' if it is not covered then the goal state is not reached
    for i in range(BOARD_X):
        for j in range(BOARD_Y):
            if not is_covered(i,j,state):
                return False
    print( "\nGOAL STATE:" )
    print_board_state( state )
    return True

def is_covered(x,y,state):
    # Check (x,y) against every queen that has been placed
    for queen in state:
        # For any two points, (x1,y1) and (x2,y2): (y2-y1)/(x2-x1) is the gradient of a line intersecting the two points
        # If that gradient is 1 or -1 then the two points are diagonal to eachother
        if x == queen[0] or y == queen[1] or abs(float(y-queen[1])/float(x-queen[0]))==1:
            return True
    return False

def print_board_state(state):
    board = [ [0 for y in range(BOARD_Y)] for x in range(BOARD_X)]
    for queen in state:
        board[queen[0]][queen[1]] = 1
    for row in board:
        for square in row:
            print ("Q" if square == 1 else "-",end='')
        print()

def qc_print_problem_info():
    print("The Queen Cover problem on a ", BOARD_X, "x", BOARD_Y, " board")


def make_qc_problem(x, y):
    global BOARD_X, BOARD_Y, qc_initial_state
    BOARD_X = x
    BOARD_Y = y
    qc_initial_state = qc_get_initial_state(x,y)
    return (None,
            qc_print_problem_info,
            qc_initial_state,
            qc_possible_actions,
            qc_successor_state,
            qc_goal_state
            )