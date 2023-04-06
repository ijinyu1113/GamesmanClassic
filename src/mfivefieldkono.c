/************************************************************************
**
** NAME:        mfivefieldkono.c
**
** DESCRIPTION: Five-Field Kono
**
** AUTHOR:      Andrew Lee
**
** DATE:        2023-02-24
**
************************************************************************/

#include <stdio.h>
#include <math.h>
#include "gamesman.h"




/* GLOBAL VARIABLES */

/* The person who implemented this game */
STRING kAuthorName = "Andrew Lee";

/* Full name of the game */
STRING kGameName = "Five-Field Kono";

/* Name of the game for databases */
STRING kDBName = "fivefieldkono";

/* How big our POSITION hash can get -- note that this isn't the same
as the upper bound on the number of positions of the game for unefficient 
(< 100% efficient) hashing methods */
// 2 * 1189188000 = 2378376000
POSITION gNumberOfPositions = 2378376000; 

/* The hash value of the initial position of the board */
POSITION gInitialPosition = 0;

/* The hash value of any invalid position */
POSITION kBadPosition = -1;

/* There can be different moves available to each player */
BOOLEAN kPartizan = TRUE;

/* It is possible to tie or draw */
BOOLEAN kTieIsPossible = TRUE;

/* The game is loopy */
BOOLEAN kLoopy = TRUE;

/* GetCanonicalPosition will be implemented */
BOOLEAN kSupportsSymmetries = TRUE;

/* TODO: No clue what this does */
BOOLEAN kDebugDetermineValue = FALSE;

/* For initializing the game in Tcl non-generically */
void* gGameSpecificTclInit = NULL;

/* Useful when there are variants available */
BOOLEAN kGameSpecificMenu = FALSE;

/* Enables debug menu for... debugging */
BOOLEAN kDebugMenu = FALSE;

/* Help strings for human players */
STRING kHelpGraphicInterface = "";
STRING kHelpTextInterface = "";
STRING kHelpOnYourTurn = "";
STRING kHelpStandardObjective = "";
STRING kHelpReverseObjective = "";
STRING kHelpTieOccursWhen = "";
STRING kHelpExample = "";

/* 12!/(4!4!4!) = 34650 */ 
POSITION max_even_hash = 34650;

/* 13!/(3!3!7!) = 34320 */
POSITION max_odd_hash = 34320;

/* optimized factorial lookup: 0 to 25 */
long fact_array[14];




/* BOARD DEFINITION */

/* A Five-Field Kono board consists of 25 spots, but has two connected 
components (such that pieces that start out in one cannot possibly move
into the other). Since clever solving might be possible due to this, we
store them separately. */
// TRUE = your turn (piece x), FALSE = opponent turn (piece o)
typedef struct {
  char even_component[12];
  char odd_component[13];
  BOOLEAN oppTurn;
} FFK_Board;




/* FUNCTIONAL DECLARATION */

/* Solving functions. */
void InitializeGame();
POSITION GetInitialPosition();
MOVELIST *GenerateMoves(POSITION hash);
POSITION GetCanonicalPosition(POSITION position);
POSITION DoMove(POSITION hash, MOVE move);
VALUE Primitive(POSITION position);

/* Solving helper functions. */
void evaluateEven(int currPos, int newPos, MOVELIST **moves, char *even_component, BOOLEAN oppTurn);
void evaluateOdd(int currPos, int newPos, MOVELIST **moves, char *odd_component, BOOLEAN oppTurn);
int convertCharToInt(char char_component);

/* Transformation functions. */
void permute(char* target, int* map, int size);
void flipComponent(FFK_Board* board, BOOLEAN evenComponent);
void switchBoard(FFK_Board* board);
char oppositePiece(char board_char);

/* Tier Functions for TierGamesman Support */
TIERLIST *getTierChildren(TIER tier);
TIERPOSITION numberOfTierPositions(TIER tier);
UNDOMOVELIST *GenerateUndoMovesToTier(POSITION position, TIER tier);
POSITION UndoMove(POSITION position, UNDOMOVE undoMove);
POSITION swapTurn(POSITION position);

/* Board hashing functions. */
POSITION Hash(FFK_Board* board);
POSITION compute_hash(char board_component[], int slots, int num_x, int num_o);
FFK_Board* Unhash(POSITION in);
void compute_unhash(char board_component[], POSITION in, int slots, int num_x, int num_o);
POSITION rearrangements(int slots, int x, int o);
void precompute_fact(long fact_array[], int limit);
int factorial(int n);
POSITION swapTurn(POSITION hash);
char swapPiece(char board_char);

/* Board value functions. */
BOOLEAN isWin(FFK_Board* board);
BOOLEAN isLose(FFK_Board* board);
BOOLEAN isTie(FFK_Board* board);

/* Move hashing functions. */
MOVE hashMove(int oldPos, int newPos);
void unhashMove(MOVE mv, int *oldPos, int *newPos);

/* Other stuff we don't care about for now. */
STRING MoveToString(MOVE move);




/* INITIAL BOARD DESCRIPTION */

/* Amount of 'o' pieces initially in each component. */
int initial_even_num_o = 4;
int initial_odd_num_o = 3;

/* Amount of 'x' pieces initially in each component. */
int initial_even_num_x = 4;
int initial_odd_num_x = 3;

/* Amount of pieces in the even component of the baord. */
int even_comp_size = 12;

/* Amount of pieces in the odd component of the baord. */
int odd_comp_size = 13;

/* Describes the initial piece positions in the even part of the board. */
char initial_even_component[12] =
{'o', 'o', 'o', '-', 'o', '-', '-', 'x', '-', 'x', 'x', 'x'};

/* Describes the initial piece positions in the even part of the board. */
char initial_odd_component[13] =
{'o', 'o', 'o', '-', '-', '-', '-', '-', '-', '-', 'x', 'x', 'x'};




/* BOARD TRANSFORMATION ARRAYS */

/* Describes a board transformation of a vertical flip (a reflection
across the y-axis, so to say). The piece at original[i] should end up 
at destination[array[i]], where 'array' is one of the arrays below. */
int even_xflip_pos[12] = {10, 11, 7, 8, 9, 5, 6, 2, 3, 4, 0, 1};
int odd_xflip_pos[13] = {10, 11, 12, 8, 9, 5, 6, 7, 3, 4, 0, 1, 2};

/* Describes a board transformation of a horizontal flip (a reflection
across the y-axis, so to say). The piece at original[i] should end up 
at destination[array[i]], where 'array' is one of the arrays below. */
int even_yflip_pos[12] = {1, 0, 4, 3, 2, 6, 5, 9, 8, 7, 11, 10};
int odd_yflip_pos[13] = {2, 1, 0, 4, 3, 7, 6, 5, 9, 8, 12, 11, 10};




/* SOLVING FUNCIONS */

/* Initialize any global variables or data structures needed. */
void InitializeGame() {
  precompute_fact(fact_array, 13);
  gMoveToStringFunPtr = &MoveToString;
  gInitialPosition = GetInitialPosition();
  gCanonicalPosition = GetCanonicalPosition;

  /* Tier-Related Initialization */
  gTierChildrenFunPtr = &getTierChildren;
  gNumberOfTierPositionsFunPtr = &numberOfTierPositions;
  gInitialTierPosition = gInitialPosition;
  kSupportsTierGamesman = TRUE;
  kExclusivelyTierGamesman = TRUE;
  gInitialTier = 0; // There will only be one tier and its ID will be 0
  gUnDoMoveFunPtr = &UndoMove;
  gGenerateUndoMovesToTierFunPtr = &GenerateUndoMovesToTier;
}

/* Return the hash value of the initial position. */
POSITION GetInitialPosition() {
  FFK_Board *initial_board = malloc(sizeof(FFK_Board));
  for (int i = 0; i < even_comp_size; i++) 
  initial_board->even_component[i] = initial_even_component[i];
  for (int i = 0; i < odd_comp_size; i++) 
  initial_board->odd_component[i] = initial_odd_component[i];
  initial_board->oppTurn = FALSE;
  POSITION result = Hash(initial_board);
  free(initial_board);
  return result;
}

/* Return a linked list of possible moves. */
MOVELIST *GenerateMoves(POSITION hash) {
  FFK_Board *newboard = Unhash(hash);
  MOVELIST *moves = NULL;
  for (int i = 0; i < even_comp_size; i++) {
    if (i != 4 && i != 9) {
      evaluateEven(i, i - 2, &moves, newboard->even_component, newboard->oppTurn);
      evaluateEven(i, i + 3, &moves, newboard->even_component, newboard->oppTurn);
    }
    if (i != 2 && i != 7) {
      evaluateEven(i, i + 2, &moves, newboard->even_component, newboard->oppTurn);
      evaluateEven(i, i - 3, &moves, newboard->even_component, newboard->oppTurn);
    }
  }
  for (int j = 0; j < odd_comp_size; j++) {
    if (j != 0 && j != 5 && j != 10) {
      evaluateOdd(j, j + 2, &moves, newboard->odd_component, newboard->oppTurn);
      evaluateOdd(j, j - 3, &moves, newboard->odd_component, newboard->oppTurn);
    }
    if (j != 2 && j != 7 && j != 12) {
      evaluateOdd(j, j - 2, &moves, newboard->odd_component, newboard->oppTurn);
      evaluateOdd(j, j + 3, &moves, newboard->odd_component, newboard->oppTurn);
    }
  }
  free(newboard);
  return moves;
}

/* Return the resulting position from making 'move' on 'position'. */
POSITION DoMove(POSITION hash, MOVE move) {
  // Get current board
  FFK_Board* board = Unhash(hash);

  // Get move information (from, to = indices in board[25])
  int from, to;
  unhashMove(move, &from, &to);

  // Change the oppTurn --> !oppTurn to reflect change in turn
  board->oppTurn = !(board->oppTurn);

  if (from < 12) {
    // Piece to be moved is in board->even_component[12] which means
    // that 'to' is as well; mutate board accordingly
    char piece = board->even_component[from];
    board->even_component[from] = '-';
    board->even_component[to] = piece;
  } else {
    // Piece to be moved is in board->odd_component[13] which means
    // that 'to' is as well
    from = from - 12;
    to = to - 12;

    // Mutate board accordingly
    char piece = board->odd_component[from];
    board->odd_component[from] = '-';
    board->odd_component[to] = piece;
  }

  // Compute hash for post-move
  POSITION result = Hash(board);
  free(board);
  return result;
}

/* Symmetry Handling: Return the canonical position. For reference:
"A" - refers to the even component of the board.
"B" - refers to the odd component of the board.
"X_e" - refers to the X component of the board with no
        transformations applied.
"X_f" - refers to the X component of the board with a
        y-axis flip applied (defined by flipComponent()).
"(STATE)_s" - refers to the STATE of the board with a
              switch (defined by switchComponent()) applied.
So for example, the board in its base state is "A_e B_e," and doing 
switchComponent() on this is "(A_e B_e)_s." Need I say more? */
POSITION GetCanonicalPosition(POSITION position) {
  FFK_Board* board = Unhash(position);
  POSITION symmetries[8];
  POSITION canonical = 0;

  // 'Flip only' symmetries
  symmetries[0] = position;  // A_e B_e
  flipComponent(board, TRUE);
  symmetries[1] = Hash(board); // A_f B_e
  flipComponent(board, FALSE);
  symmetries[2] = Hash(board); // A_f B_f
  flipComponent(board, TRUE);
  symmetries[3] = Hash(board); // A_e B_f

  // 'Board switched' symmetries
  switchBoard(board);
  symmetries[4] = Hash(board); // (A_e B_f)_s
  flipComponent(board, TRUE);
  symmetries[5] = Hash(board); // (A_f B_f)_s
  flipComponent(board, FALSE);
  symmetries[6] = Hash(board); // (A_f B_e)_s
  flipComponent(board, TRUE);
  symmetries[7] = Hash(board); // (A_e B_e)_s
  
  // Choose the biggest hash as canonical 
  for (int i = 0; i < 8; i++) {
    if (symmetries[i] > canonical) {
      canonical = symmetries[i];
    }
  }

  free(board);
  return canonical;
}

/* Return lose, win, tie, or undecided. See src/core/types.h
for the value enum definition. */
VALUE Primitive(POSITION position) {
  FFK_Board *board = Unhash(position);
  BOOLEAN is_win = isWin(board);
  BOOLEAN is_lose = isLose(board);
  BOOLEAN is_tie = isTie(board);
  free(board);
  if (is_win || is_lose) {
    return lose;
  }
  if (is_tie) {
    return tie;
  }
  return undecided;
}

/* The tier graph is just a single tier with id=0. */
TIERLIST *getTierChildren(TIER tier) {
  return CreateTierlistNode(0, NULL);
}

/* We use a single tier for this entire game. This
is returns the upper bound */
TIERPOSITION numberOfTierPositions(TIER tier) {
  return gNumberOfPositions;
}

/* Return a linked list of all possible moves that could have been made in
order to arrive at the input position. The movement rules of FFK are
nice in that we can reuse GenerateMoves on the position with the turn 
swapped. The result of GenerateMoes can be our undoMoves, except that we 
need to filter out moves that come from primitive positions.
There is only one tier, so all undoMoves will lead to previous positions
that are in the same tier, so tier is ignored. */
UNDOMOVELIST *GenerateUndoMovesToTier(POSITION position, TIER tier) {
  MOVELIST *moves = GenerateMoves(swapTurn(position));
  MOVELIST *head = moves;
  UNDOMOVELIST *undoMoves = NULL;
  while (moves != NULL) {
    if (Primitive(UndoMove(position, moves->move)) == undecided) {
      undoMoves = CreateUndoMovelistNode(moves->move, undoMoves);
    }
    moves = moves->next;
  }
  FreeMoveList(head);
  return undoMoves;
}

/* Return the parent position given the undoMove. */
POSITION UndoMove(POSITION position, UNDOMOVE undoMove) {
  return swapTurn(DoMove(swapTurn(position), undoMove));
}


/* SOLVING HELPER FUNCTIONS */

void evaluateEven(int currPos, int newPos, MOVELIST **moves, char *even_component, BOOLEAN oppTurn) {
  if (newPos < 0 || newPos >= even_comp_size) {
    return;
  }
  int currElem = convertCharToInt(even_component[currPos]);
  int newElem = convertCharToInt(even_component[newPos]);
  int match = oppTurn ? 1 : 2;
  if (currElem == match && newElem == 0) {
    *moves = CreateMovelistNode(hashMove(currPos, newPos), *moves);
  }
}

void evaluateOdd(int currPos, int newPos, MOVELIST **moves, char *odd_component, BOOLEAN oppTurn) {
  if (newPos < 0 || newPos >= odd_comp_size) {
    return;
  }
  int currElem = convertCharToInt(odd_component[currPos]);
  int newElem = convertCharToInt(odd_component[newPos]);
  int match = oppTurn ? 1 : 2;
  if (currElem == match && newElem == 0) {
    *moves = CreateMovelistNode(hashMove(12 + currPos, 12 + newPos), *moves);
  }
}

/* Converts a character to its ternary integer equivalent
for hash calculations. */
int convertCharToInt(char char_component) {
  if (char_component == '-') {
    return 0;
  } else if (char_component == 'o') {
    return 1;
  } else if (char_component == 'x') {
    return 2;
  }
  return -1;
}




/* TRANSFORMATION FUNCTIONS */

/* Performs an in-place rearrangement of the contents of TARGET as outlined
by a transformation array MAP, assuming they are both the same SIZE. Does not 
allocate memory and is linear in the size of the array being permuted. */
void permute(char* target, int* map, int size) {
  char temp[size];
  for (int i = 0; i < size; i++) {
    temp[i] = target[map[i]];
  }
  memcpy(target, temp, size * sizeof(char));
}

/* Transforms the board by flipping it horizontally (a reflection
about the y-axis). */
void flipComponent(FFK_Board* board, BOOLEAN evenComponent) {
  if (evenComponent) {
    permute(board->even_component, even_yflip_pos, even_comp_size);
  } else {
    permute(board->odd_component, odd_yflip_pos, odd_comp_size);
  }
}

/* Switches whose turn it is, replaces each piece with one of the opposite
color, and flips the board across the x-axis. */
void switchBoard(FFK_Board* board) {
  // Flip across the x-axis
  permute(board->even_component, even_xflip_pos, even_comp_size);
  permute(board->odd_component, odd_xflip_pos, odd_comp_size);

  // Replace pieces with opposites
  for (int i = 0; i < even_comp_size; i++)
    board->even_component[i] = oppositePiece(board->even_component[i]);
  for (int j = 0; j < odd_comp_size; j++)
    board->odd_component[j] = oppositePiece(board->odd_component[j]);

  // Switch whose turn it is
  board->oppTurn = !(board->oppTurn);
}

/* Replaces a board piece to the opponent's piece type relative to whose
piece it is. */
char oppositePiece(char board_char) {
  if (board_char == 'o') return 'x';
  else if (board_char == 'x') return 'o';
  else return '-';
}




/* BOARD HASHING FUNCTIONS */

/* Returns the index that IN would have in the alphabetical ordering of all
possible strings composed of the same kinds and amounts of characters. */
POSITION Hash(FFK_Board* board) {
  // Collect the number of slots <-- 12 + 13 = 25 slots
  POSITION even_hash = compute_hash(board->even_component, even_comp_size, initial_even_num_x, initial_even_num_o);
  POSITION odd_hash = compute_hash(board->odd_component, odd_comp_size, initial_odd_num_x, initial_odd_num_o);
  int turn = board->oppTurn ? 1 : 0;

  // odd_hash * (2*max_even_hash) + (2**turn)*even_hash <= 2.4 billion positions
  return odd_hash * (2 * max_even_hash) + ((turn * max_even_hash) + even_hash);
}

POSITION compute_hash(char board_component[], int slots, int num_x, int num_o) {
  POSITION total = 0;
  int arr_len = slots;
  for (int i = 0; i < arr_len; i++) {
    int t1 = rearrangements(slots - 1, num_x, num_o);
    int t2 = t1 + rearrangements(slots - 1, num_x, num_o - 1);
    if (board_component[i] == 'o') {
      total += t1;
      num_o -= 1;
    } else if (board_component[i] == 'x') {
      total += t2;
      num_x -= 1;
    }
    slots -= 1;
  }
  return total;
}

/* The inverse process of hash. */
FFK_Board* Unhash(POSITION in) {
  FFK_Board* newBoard = (FFK_Board *) malloc(sizeof(FFK_Board));
  POSITION even_hash = in % (2*max_even_hash);
  POSITION odd_hash = in/(2*max_even_hash);
  if (even_hash >= max_even_hash) newBoard->oppTurn = TRUE;
  else newBoard->oppTurn = FALSE;
  if (even_hash >= max_even_hash) even_hash -= max_even_hash;
  compute_unhash(newBoard->even_component, even_hash, even_comp_size, initial_even_num_x, initial_even_num_o);
  compute_unhash(newBoard->odd_component, odd_hash, odd_comp_size, initial_odd_num_x, initial_odd_num_o);

  return newBoard;
}

void compute_unhash(char board_component[], POSITION in, int slots, int num_x, int num_o) {
  int idx = 0;
  while (slots > 0) {
    int t1 = rearrangements(slots - 1, num_x, num_o);
    int t2 = t1 + rearrangements(slots - 1, num_x, num_o - 1);
    if (in < t1) {
      board_component[idx] = '-';
    } else if (in < t2) {
      board_component[idx] = 'o';
      in -= t1;
      num_o -= 1;
    } else {
      board_component[idx] = 'x';
      in -= t2;
      num_x -= 1;
    }
    slots -= 1;
    idx += 1;
  }
}

/* Returns the amount of ways to put X x's and O o's into SLOTS slots. */
POSITION rearrangements(int slots, int x, int o) {
  if ((slots < 0) || (x < 0) || (o < 0) || (slots < (o + x))) return 0;
  // Essentially returns the number of ways to place the -'s, x's, and o's in the slots
  return fact_array[slots]/(fact_array[x]*fact_array[o]*fact_array[slots - x - o]);
}

void precompute_fact(long fact_array[], int limit) {
  fact_array[0] = 1;
  for (long i = 1; i <= limit; i++) {
    fact_array[i] = i * fact_array[i-1];
  }
}

/* Swap the turn and return the new hash */
POSITION swapTurn(POSITION hash) {
  BOOLEAN turn;
  POSITION even_hash = hash % (2*max_even_hash);
  POSITION odd_hash = hash / (2*max_even_hash);
  // if even_hash >= max_even_hash it was the opponent's turn
  // hence, set the turn to FALSE to swap it to your turn
  // and vice versa for
  if (even_hash >= max_even_hash) {
    turn = FALSE;
    even_hash -= max_even_hash;
  } else turn = TRUE;
  return odd_hash * (2 * max_even_hash) + ((turn * max_even_hash) + even_hash);
}



/* BOARD VALUE FUNCTIONS */

/* X wins when all of the spots originally populated by O's are 
filled with X's. */
BOOLEAN isWin(FFK_Board* board) {
  return board->even_component[0] == 'x'
  && board->even_component[1] == 'x'
  && board->even_component[2] == 'x'
  && board->even_component[4] == 'x'
  && board->odd_component[0] == 'x'
  && board->odd_component[1] == 'x'
  && board->odd_component[2] == 'x';
}

/* O wins when all of the spots originally populated by X's are 
filled with O's. */
BOOLEAN isLose(FFK_Board* board) {
  return board->even_component[11] == 'o'
  && board->even_component[10] == 'o'
  && board->even_component[9] == 'o'
  && board->even_component[7] == 'o'
  && board->odd_component[12] == 'o'
  && board->odd_component[11] == 'o'
  && board->odd_component[10] == 'o';
}

/* If there are no moves left to be made, then the game is a tie. */
BOOLEAN isTie(FFK_Board* board) {
  MOVELIST *possible_moves = GenerateMoves(Hash(board));
  if (possible_moves == NULL) {
    FreeMoveList(possible_moves);
    return TRUE;
  }
  FreeMoveList(possible_moves);
  return FALSE;
}




/* MOVE HASHING FUNCTIONS */

/* Uses the start and destination index of a piece in the connected
component of the board it belongs to and whose turn it is to generate
a unique hash for a game state graph edge. */
MOVE hashMove(int oldPos, int newPos) {
  // 0b<base 25 newPos, base 25 oldPos>
  return (25*newPos) + oldPos;
}

/* Obtains the start and destination index of a piece in the connected
component of the board it belongs to and whose turn it is based on
a unique hash for a game state graph edge. */
void unhashMove(MOVE mv, int *oldPos, int *newPos) {
  // 0b<base 25 newPos, base 25 oldPos>
  *oldPos = mv % 25;
  *newPos = mv/25;
}




/* TEXTUI FUNCTIONS */

/* This will print the board in the following format:

  (o) (o) (o) (o) (o)
    \ / \ / \ / \ /  
    / \ / \ / \ / \  
  (o) ( ) ( ) ( ) (o)
    \ / \ / \ / \ /  
    / \ / \ / \ / \  
  ( ) ( ) ( ) ( ) ( )
    \ / \ / \ / \ /  
    / \ / \ / \ / \  
  (x) ( ) ( ) ( ) (x)
    \ / \ / \ / \ /  
    / \ / \ / \ / \  
  (x) (x) (x) (x) (x)

*/
void PrintPosition(POSITION position, STRING playerName, BOOLEAN usersTurn) {
  FFK_Board* board = Unhash(position);
  if (usersTurn) printf("%s's move.\n", playerName);
  char* fb = malloc(sizeof(char)*25);
  for (int i = 1; i < 26; i++) {
    if (i % 2 == 0) {
      char curr = board->even_component[(i/2)-1];
      fb[i-1] = (curr == '-') ? ' ' : curr;
    } else {
      char curr = board->odd_component[(i-1)/2];
      fb[i-1] = (curr == '-') ? ' ' : curr;
    }
  }
  for (int j = 0; j < 4; j++) 
    printf("(%c) (%c) (%c) (%c) (%c) \n  \\ / \\ / \\ / \\ /   \n  / \\ / \\ / \\ / \\   \n", fb[5*j], fb[(5*j)+1], fb[(5*j)+2], fb[(5*j)+3], fb[(5*j)+4]);
  printf("(%c) (%c) (%c) (%c) (%c) \n", fb[20], fb[21], fb[22], fb[23], fb[24]);
  free(fb);
  free(board);
}

void PrintComputersMove(MOVE computersMove, STRING computersName) {
  /* YOUR CODE HERE */
}

USERINPUT GetAndPrintPlayersMove(POSITION position, MOVE *move, STRING playerName) {
  /* YOUR CODE HERE */
  /* YOUR CODE HERE */
  USERINPUT ret;

	do {
    //PrintMove(*move);
		printf("%8s's move:  ", playerName);

		ret = HandleDefaultTextInput(position, move, playerName);
		if(ret != Continue)
			return(ret);

	}
	while (TRUE);
	return(Continue); /* this is never reached, but lint is now happy */
}

/* Return whether the input text signifies a valid move. Rows are letters, and
columns are numbers:
- A piece in rows {a, c, e} can only go to one of {b, d} and vice versa.
- A piece in columns {1, 3, 5} can only go to one of {2, 4} and vice versa.
- Both the rows and columns must differ in 'distance' by exactly 1.
Example valid moves: {"a1-b2", "b2-c3", "e4-d5"}. */
BOOLEAN ValidTextInput(STRING input) {
  // Check for obvious malformations
  if (strlen(input) != 5) return FALSE;
  if (input[2] != '-') return FALSE;

  // Extract characters from string
  char c1 = input[0];
  char r1 = input[1];
  char c2 = input[3];
  char r2 = input[4];

  // Determine if both slots are on the board using ASCII ranges
  if (c1 < 97 || c1 > 101 || c2 < 97 || c2 > 101) return FALSE;
  if (r1 < 49 || r1 > 53 || r2 < 49 || r2 > 53) return FALSE;
  
  // Use ASCII values to determine 'distance', which guarantees that
  // the piece moves along a valid edge and that it moves a distance
  // of exactly 1
  if (abs(r1 - r2) != 1 || abs(c1 - c2) != 1) return FALSE;

  return TRUE;
}

/* Assume the text input signifies a valid move. Return
the move hash corresponding to the move. */
MOVE ConvertTextInputToMove(STRING input) {
  // Convert string to 0-indexed coords. of a 5x5 board matrix
  int colLetter1 = ((int) input[0]) - 97;
  int rowNumber1 = ((int) input[1]) - 49;
  int colLetter2 = ((int) input[3]) - 97;
  int rowNumber2 = ((int) input[4]) - 49;

  // Flatten 5x5 matrix indices to a 25-length list index
  int from = colLetter1 + (5*rowNumber1);
  int to = colLetter2 + (5*rowNumber2);

  // The 12 and 13 indexed 
  int oddFrom, oddTo, evenFrom, evenTo;
  BOOLEAN evenMove = TRUE;

  // If the first column index is even, then we are on the odd component
  if (colLetter1 % 2 == 0) {
    oddFrom = from/2;
    oddTo = to/2;
  } else {
    evenMove = FALSE;
    evenFrom = (from-1)/2;
    evenTo = (to-1)/2;
  }

  // Encode into our little silly move hash standard
  if (evenMove) return (MOVE) evenFrom+(25*evenTo);
  return (MOVE) (12+oddFrom)+(25*(12+oddTo));
}

/* Return the string representation of the move. 
Ideally this matches with what the user is supposed to
type in. */
STRING MoveToString(MOVE move) {
  /* YOUR CODE HERE */
  return NULL;
}

/* Basically just print the move. */
void PrintMove(MOVE move) {
  /* YOUR CODE HERE */
  printf("%d", move);
}




/* VARIANT FUNCTIONS */

/* Amount of variants supported. */
int NumberOfOptions() { return 1; }

/* Return the current variant ID (0 in this case). */
int getOption() { return 0; }

/* The input is a variant id. This function sets any global variables
or data structures according to the variant specified by the variant id. 
But for now you have one variant so don't worry about this. */
void setOption(int option) { }




/* INTERACT FUNCTIONS */

POSITION InteractStringToPosition(STRING board) { return 0; }

STRING InteractPositionToString(POSITION position) { return NULL; }

STRING InteractPositionToEndData(POSITION position) { return NULL; }

STRING InteractMoveToString(POSITION position, MOVE move) { return MoveToString(move); }




/* UNUSED INTERFACE IMPLEMENTATIONS */

/* Debug menu for... debugging */
void DebugMenu() {}

/* Should be configured when Tcl is initialized non-generically */
void SetTclCGameSpecificOptions(int theOptions[]) {}

/* For implementing more than one variant */
void GameSpecificMenu() {}
