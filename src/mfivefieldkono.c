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

/* Factorial Helper Function */
int fact(n) {
  res = 1;
  for (int i = 0; i < n; i++) res = i * res;
  return res;
}

/* Board Struct */
typedef struct {
  char *even_component; // 12 characters + 1 null byte
  char *odd_component; // 13 characters + 1 null byte
} FFK_Board;

/* Char to Ternary Converter */
int convertChar(char char_component) {
  if (char_component == '-') {
    return 0;
  } else if (char_component == 'o') {
    return 1;
  } else if (char_component == 'x') {
    return 2;
  }
  return -1;
}

/* Int to Char Converter */
char convertInt(char int_component) {
  if (int_component == 0) {
    return '-';
  } else if (int_component == 1) {
    return 'o';
  } else if (int_component == 2) {
    return 'x';
  }
  return '\0';
}

/* Hash Function for the Board*/
POSITION hash(FFK_Board *board) {
  /* Base 3 Hash */
  POSITION total = 0;
  int even_len = 12;
  for (int i = 0; i < even_len; i++) {
    total += convertChar(board->even_component[(even_len - 1) - i]) * pow(3, i);
  }
  int odd_len = 13;
  for (int j = 0; j < odd_len, j++) {
    total += convertChar(board->odd_component[(odd_len - 1) - j]) * pow(3, 12 + j)
  }
  return total;
}

/* Unhash function for the Board */
FFK_Board unhash(int hash) {
  FFK_Board *newBoard = malloc(sizeof(struct FFK_Board));
  newBoard.even_component[12] = unhash_char;
  newBoard.odd_component[13] = unhash_char;
  int remain = -1;
  int even_len = 12;
  for (int i = 0; i < even_len; i++) {
    remain = hash % 3;
    hash = floor(hash/3);
    board->even_component[(even_len - 1) - i] = convertInt(remain);
  }
  int odd_len = 13;
  for (int j = 0; j < odd_len; j++) {
    remain = hash % 3;
    hash = floor(hash/3);
    board->even_component[(odd_len - 1) - j] = convertInt(remain);
  }
}

/* X wins when all of the spots originally populated by O's are 
filled with X's. */
bool x_wins(FFK_Board* board) {
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
bool o_wins(FFK_Board* board) {
  return board->even_component[12] == 'o'
  && board->even_component[11] == 'o'
  && board->even_component[10] == 'o'
  && board->even_component[8] == 'o'
  && board->odd_component[13] == 'o'
  && board->odd_component[12] == 'o'
  && board->odd_component[11] == 'o';
}

/* Clockwise 90 degree turn for odd and parallel */
int even_turn_pos[12] = {4, 9, 1, 6, 11, 3, 8, 0, 5, 10, 2, 7};
int odd_turn_pos[13] = {2, 7, 12, 4, 9, 1, 6, 11, 3, 8, 0, 5, 10};

void board_clockwise(FFK_Board* board) {
  /* odd clockwise */
  char *new_even_arr = (char *) malloc(sizeof(board->even_component));
  int even_len = 12;
  for (int i = 0; i < even_len; i++) {
    new_even_arr[i] = board->even_component[even_turn_pos[i]];
  }
  new_even_arr[even_len] = '\0';

  /* even clockwise */
  char *new_odd_arr = (char *) malloc(sizeof(board->odd_component));
  int odd_len = 13;
  for (int j = 0; j < odd_len; j++) {
    new_odd_arr[j] = board->odd_component[odd_turn_pos[j]];
  }
  new_odd_arr[odd_len] = '\0';

  /* free and assign the clockwise new and even array */
  free(board->even_component);
  free(board->odd_component);
  board->even_component = new_even_arr;
  board->odd_component = new_odd_arr;
}

int even_flip_pos[12] = {1, 0, 4, 3, 2, 6, 5, 9, 8, 7, 11, 10};
int odd_flip_pos[13] = {2, 1, 0, 4, 3, 7, 6, 5, 9, 8, 12, 11, 10};

void board_clockwise(FFK_Board* board) {
  /* odd clockwise */
  char *new_even_arr = (char *) malloc(sizeof(board->even_component));
  int even_len = 12;
  for (int i = 0; i < even_len; i++) {
    new_even_arr[i] = board->even_component[even_flip_pos[i]];
  }
  new_even_arr[even_len] = '\0';

  /* even clockwise */
  char *new_odd_arr = (char *) malloc(sizeof(board->odd_component));
  int odd_len = 13;
  for (int j = 0; j < odd_len; j++) {
    new_odd_arr[j] = board->odd_component[odd_flip_pos[j]];
  }
  new_odd_arr[odd_len] = '\0';

  /* free and assign the clockwise new and even array */
  free(board->even_component);
  free(board->odd_component);
  board->even_component = new_even_arr;
  board->odd_component = new_odd_arr;
}

/* If there are no moves left to be made, then the game is a tie.
Don't know if this is actually possible. */
bool no_moves(FFK_Board* board) {
  MOVELIST possible_moves = GenerateMoves(hash(board));
  if (possible_moves == NULL) {
    return true;
  }
  return false;
}

/* IMPORTANT GLOBAL VARIABLES */
STRING kAuthorName = "Andrew Lee";
POSITION gNumberOfPositions = 1189188000; // TODO: Put your number of positions upper bound here.
POSITION gInitialPosition = 0; // TODO: Put the hash value of the initial position.
BOOLEAN kPartizan = TRUE; // TODO: Is the game PARTIZAN i.e. given a board does each player have a different set of moves available to them?
BOOLEAN kTieIsPossible = TRUE; // TODO: Is a tie or draw possible?
BOOLEAN kLoopy = TRUE; // TODO: Is this game loopy?
BOOLEAN kSupportsSymmetries = TRUE; // TODO: Whether symmetries are supported (i.e. whether the GetCanonicalPosition is implemented)

/* Do not change these. */
POSITION GetCanonicalPosition(POSITION);
STRING MoveToString(MOVE);
POSITION kBadPosition = -1;
STRING kGameName = "Five-Field Kono";
STRING kDBName = "fivefieldkono";
BOOLEAN kDebugDetermineValue = FALSE;
void* gGameSpecificTclInit = NULL;

/* You don't have to change these for now. */
BOOLEAN kGameSpecificMenu = FALSE;
BOOLEAN kDebugMenu = FALSE;

/* These variables are not needed for solving but if you have time 
after you're done solving the game you should initialize them 
with something helpful. */
STRING kHelpGraphicInterface = "";
STRING kHelpTextInterface = "";
STRING kHelpOnYourTurn = "";
STRING kHelpStandardObjective = "";
STRING kHelpReverseObjective = "";
STRING kHelpTieOccursWhen = /* Should follow 'A Tie occurs when... */ "";
STRING kHelpExample = "";

/* You don't have to change this. */
void DebugMenu() {}
/* Ignore this function. */
void SetTclCGameSpecificOptions(int theOptions[]) {}
/* Do not worry about this yet because you will only be supporting 1 variant for now. */
void GameSpecificMenu() {}






/*********** BEGIN SOLVING FUNCIONS ***********/

/* TODO: Add a hashing function and unhashing function, if needed. */

/* Initialize any global variables or data structures needed before
solving or playing the game. */
void InitializeGame() {
  gCanonicalPosition = GetCanonicalPosition;
  gMoveToStringFunPtr = &MoveToString;

  /* YOUR CODE HERE */
  
}

/* Return the hash value of the initial position. */
POSITION GetInitialPosition() {
  FFK_Board* initial_board = malloc(sizeof(FFK_Board));
  initial_board->even_component = malloc(sizeof(char)*13);
  nitial_board->odd_componen = malloc(sizeof(char)*14);
  strcpy(initial_board->even_component, "ooo-o--x-xxx");
  strcpy(initial_board->odd_component, "ooo-------xxx");
  return hash(initial_board);
}

/* Return a linked list of moves. */
MOVELIST *GenerateMoves(POSITION position) {
  FFK_Board *newboard =  unhash(position);
  MOVELIST *moves = NULL;
  /* YOUR CODE HERE 
     
     To add to the linked list, do 
     moves = CreateMovelistNode(<the move you're adding>, moves);

     See the function CreateMovelistNode in src/core/misc.c

  */
  
  return moves;
}

/* Return the position that results from making the 
input move on the input position. */
POSITION DoMove(POSITION position, MOVE move) {
  /* YOUR CODE HERE */
  return 0;
}

/* Return lose, win, tie, or undecided. See src/core/types.h
for the value enum definition. */
VALUE Primitive(POSITION position) {
  FFK_Board* board = unhash(position);
  bool x_wins = x_wins(board);
  bool o_wins = o_wins(board);
  bool no_moves = no_moves(board);
  free(board);
  if (x_wins) {
    return win
  }
  if (o_wins) {
    return lose
  }
  if (no_moves) {
    return tie
  }
  return undecided;
}

/* Symmetry Handling: Return the canonical position. */
POSITION GetCanonicalPosition(POSITION position) {
  
  return position;
}

/*********** END SOLVING FUNCTIONS ***********/







/*********** BEGIN TEXTUI FUNCTIONS ***********/

void PrintPosition(POSITION position, STRING playerName, BOOLEAN usersTurn) {
  /* THIS ONE IS MOST IMPORTANT FOR YOUR DEBUGGING */
  /* YOUR CODE HERE */
}

void PrintComputersMove(MOVE computersMove, STRING computersName) {
  /* YOUR CODE HERE */
}

USERINPUT GetAndPrintPlayersMove(POSITION position, MOVE *move, STRING playerName) {
  /* YOUR CODE HERE */
  return Continue;
}

/* Return whether the input text signifies a valid move. */
BOOLEAN ValidTextInput(STRING input) {
  /* YOUR CODE HERE */
  return TRUE;
}

/* Assume the text input signifies a valid move. Return
the move hash corresponding to the move. */
MOVE ConvertTextInputToMove(STRING input) {
  /* YOUR CODE HERE */
  return 0;
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
}

/*********** END TEXTUI FUNCTIONS ***********/









/*********** BEGIN VARIANT FUNCTIONS ***********/

/* How many variants are you supporting? For now, just 1.
Maybe in the future you want to support more variants. */
int NumberOfOptions() {
  /* YOUR CODE HERE MAYBE LATER BUT NOT NOW */
  return 1;
}

/* Return the current variant id (which is 0 in this case since
for now you're only thinking about one variant). */
int getOption() {
  /* YOUR CODE HERE MAYBE LATER BUT NOT NOW */
  return 0;
}

/* The input is a variant id. This function sets any global variables
or data structures according to the variant specified by the variant id. 
But for now you have one variant so don't worry about this. */
void setOption(int option) {
  /* YOUR CODE HERE MAYBE LATER BUT NOT NOW */
}

/*********** END VARIANT-RELATED FUNCTIONS ***********/







/* Don't worry about these Interact functions below yet.
They are used for the AutoGUI which eventually we would
want to implement, but they are not needed for solving. */
POSITION InteractStringToPosition(STRING board) {
  /* YOUR CODE HERE LATER BUT NOT NOW */
  return 0;
}

STRING InteractPositionToString(POSITION position) {
  /* YOUR CODE HERE LATER BUT NOT NOW */
  return NULL;
}

/* Ignore this function. */
STRING InteractPositionToEndData(POSITION position) {
  return NULL;
}

STRING InteractMoveToString(POSITION position, MOVE move) {
  /* YOUR CODE HERE LATER BUT NOT NOW */
  return MoveToString(move);
}