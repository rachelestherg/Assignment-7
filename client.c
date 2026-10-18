#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "game.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 4096

#define REQUEST_CREATE 1
#define REQUEST_JOIN 2
#define REQUEST_MOVE 3

struct request {
    int type;
    int value;
};

static void print_board(struct game* game) {
    printf("\n");
    for (int col = 0; col < BOARD_COLS; col++) {
        printf(" %d", col);
    }
    printf("\n");

    for (int row = 0; row < BOARD_ROWS; row++) {
        printf("|");
        for (int col = 0; col < BOARD_COLS; col++) {
            int cell = game->board.cells[row][col];
            printf("%c|", cell == 0 ? 'X' : cell == 1 ? 'O' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

static void print_current_game(struct game* game, int waiting) {
    printf("Current Game\n");
    printf("---------------------\n");
    printf("GameID: %d\n\n", game->gameId);
    print_board(game);
    if (waiting) {
        printf("waiting for other player...\n");
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("Connect 4\n");
    printf("  1) Create a new game\n");
    printf("  2) Join a game\n");
    printf("> ");

    int choice = 0;
    scanf("%d", &choice);

    struct request req;
    struct game game;
    int player = -1;

    if (choice == 1) {
        req.type = REQUEST_CREATE;
        req.value = 0;
        sendto(sock_fd, &req, sizeof(req), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        recvfrom(sock_fd, &game, sizeof(game), 0, NULL, NULL);
        player = 0;
        print_current_game(&game, 1);
    } else if (choice == 2) {
        int gameId = 0;
        printf("Game ID? > ");
        scanf("%d", &gameId);

        req.type = REQUEST_JOIN;
        req.value = gameId;
        sendto(sock_fd, &req, sizeof(req), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        recvfrom(sock_fd, &game, sizeof(game), 0, NULL, NULL);
        player = 1;
        print_current_game(&game, 1);
    }

    while (1) {
        struct game incoming;
        recvfrom(sock_fd, &incoming, sizeof(incoming), 0, NULL, NULL);
        game = incoming;

        if (game.current_player == player) {
            print_current_game(&game, 0);
            printf("Input Move Row: > ");
            int col = 0;
            scanf("%d", &col);
            req.type = REQUEST_MOVE;
            req.value = col;
            sendto(sock_fd, &req, sizeof(req), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
        } else {
            print_current_game(&game, 1);
        }

        if (game.winner == player) {
            printf("YOU WIN!\n");
            break;
        }
        if (game.winner != -1) {
            printf("YOU LOSE!\n");
            break;
        }
    }

    close(sock_fd);
    return 0;
}
