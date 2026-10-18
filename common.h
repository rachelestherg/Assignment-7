#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef COMMON_H
#define COMMON_H

#define BOARD_ROWS 6
#define BOARD_COLS 7
#define EMPTY -1

struct board {
    int cells[BOARD_ROWS][BOARD_COLS];
};

struct game {
    int gameId;
    int current_player;
    int winner;
    struct board board;
};

struct gameDB {
    int gameId;
    int current_player;
    int winner;
    struct board board;
};

#endif
