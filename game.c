#include "game.h"
#include <stdlib.h>
#include <stddef.h>

/*
Takes as input a pointer to a connect4 game board, a player (0/1), and a int column.
Places a new piece in `col` on the game board.
*/
int make_move(struct game* g, int player, int col) {
    if (g == NULL || (player != 0 && player != 1)) {
        return 0;
    }

    if (col < 0 || col >= BOARD_COLS) {
        return 0;
    }

    for (int row = BOARD_ROWS - 1; row >= 0; --row) {
        if (g->board.cells[row][col] == EMPTY) {
            g->board.cells[row][col] = player;
            g->current_player = (player == 0) ? 1 : 0;
            g->winner = check_win(g);
            return 1;
        }
    }

    return 0;
}
 
/*
Takes as input a pointer to a connect4 game board.
Returns 0 or 1 if player 0 or 1 has won respectively.
Returns -1 if there is no winner.
*/
int check_win(struct game* g) {
    if (g == NULL) {
        return -1;
    }

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int col = 0; col < BOARD_COLS; ++col) {
            int piece = g->board.cells[row][col];

            if (piece == EMPTY) {
                continue;
            }

            if (col <= BOARD_COLS - 4 &&
                piece == g->board.cells[row][col + 1] &&
                piece == g->board.cells[row][col + 2] &&
                piece == g->board.cells[row][col + 3]) {
                return piece;
            }

            if (row <= BOARD_ROWS - 4 &&
                piece == g->board.cells[row + 1][col] &&
                piece == g->board.cells[row + 2][col] &&
                piece == g->board.cells[row + 3][col]) {
                return piece;
            }

            if (row <= BOARD_ROWS - 4 && col <= BOARD_COLS - 4 &&
                piece == g->board.cells[row + 1][col + 1] &&
                piece == g->board.cells[row + 2][col + 2] &&
                piece == g->board.cells[row + 3][col + 3]) {
                return piece;
            }

            if (row <= BOARD_ROWS - 4 && col >= 3 &&
                piece == g->board.cells[row + 1][col - 1] &&
                piece == g->board.cells[row + 2][col - 2] &&
                piece == g->board.cells[row + 3][col - 3]) {
                return piece;
            }
        }
    }

    return -1;
}
 
/*
Initializes an existing game with an empty board.
*/
void new_game(struct game* g, int gameId) {
    if (g == NULL) {
        return;
    }

    g->gameId = gameId;
    g->current_player = 0;
    g->winner = -1;

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int col = 0; col < BOARD_COLS; ++col) {
            g->board.cells[row][col] = EMPTY;
        }
    }
}
