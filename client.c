#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "game.h"
#include "save.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 4096

#define REQUEST_CREATE 1
#define REQUEST_JOIN 2
#define REQUEST_MOVE 3
#define REQUEST_RESUME 4

struct request {
    int type;
    int value;
    int player;
    int gameId;
};

static void print_board(struct game* game) {
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
        printf("\n");
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct timeval recv_timeout = {1, 0};
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("Connect 4\n");
    printf("  1) Create a new game\n");
    printf("  2) Join a game\n");

    int choice = 0;
    scanf("%d", &choice);
    fflush(stdout);

    struct request req;
    struct game game;
    int player = -1;

    if (choice == 1) {
        printf("> \n");
        req.type = REQUEST_CREATE;
        req.value = 0;
        req.player = 0;
        req.gameId = -1;
        sendto(sock_fd, &req, sizeof(req), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        recvfrom(sock_fd, &game, sizeof(game), 0, NULL, NULL);
        player = 0;
        print_current_game(&game, 1);
    } else if (choice == 2) {
        int gameId = 0;
        printf("> Game ID? >");
        fflush(stdout);
        scanf("%d", &gameId);
        printf("\n");
        fflush(stdout);

        req.type = REQUEST_JOIN;
        req.value = gameId;
        req.player = 1;
        req.gameId = gameId;
        sendto(sock_fd, &req, sizeof(req), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        recvfrom(sock_fd, &game, sizeof(game), 0, NULL, NULL);
        player = 1;
        print_current_game(&game, 1);
    }

    while (1) {
        struct game incoming;
        ssize_t received = recvfrom(sock_fd, &incoming, sizeof(incoming), 0,
                                    (struct sockaddr *)&server_addr, &server_addr_len);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                req.type = REQUEST_RESUME;
                req.value = game.gameId;
                req.player = player;
                req.gameId = game.gameId;
                sendto(sock_fd, &req, sizeof(req), 0,
                       (struct sockaddr *)&server_addr, sizeof(server_addr));
            }
        }
        if (received != sizeof(incoming)) {
            continue;
        }
        game = incoming;

        if (game.winner == player) {
            print_current_game(&game, 0);
            printf("YOU WIN!\n");
            break;
        }
        if (game.winner != -1) {
            print_current_game(&game, 0);
            printf("YOU LOSE!\n");
            break;
        }

        if (game.current_player == player) {
            print_current_game(&game, 0);
            printf("\n");
            printf("Input Move Row: > ");
            int col = 0;
            scanf("%d", &col);
            printf("\n");
            fflush(stdout);
            req.type = REQUEST_MOVE;
            req.value = col;
            req.player = player;
            req.gameId = game.gameId;
            ssize_t sent =sendto(sock_fd, &req, sizeof(req), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
        } else {
            print_current_game(&game, 1);
        }
    }

    close(sock_fd);
    return 0;
}
