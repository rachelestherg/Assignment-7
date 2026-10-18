#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "game.h"
#include "save.h"

#define PORT 8080
#define MAX_GAMES 100
#define MAX_CLIENTS 128

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

struct client_slot {
    int active;
    int gameId;
    int player;
    struct sockaddr_in addr;
    socklen_t addr_len;
};

static struct game games[MAX_GAMES];
static struct client_slot clients[MAX_CLIENTS];
static int next_game_id = 0;

static int load_saved_games(void) {
    int loaded = 0;
    for (int i = 0; i < MAX_GAMES; ++i) {
        char path[128];
        snprintf(path, sizeof(path), "game_%d.save", i);
        if (loadgame((struct gameDB*)&games[i], path) == 0) {
            games[i].gameId = i;
            loaded = 1;
            if (next_game_id <= i) {
                next_game_id = i + 1;
            }
        }
    }
    return loaded;
}

static void send_game_to_client(int sock_fd, const struct sockaddr_in* dest, socklen_t dest_len,
                               const struct game* game_state) {
    ssize_t sent = sendto(sock_fd, game_state, sizeof(*game_state), 0,
           (struct sockaddr *)dest, dest_len);
}

static int find_client_index(const struct sockaddr_in* addr, socklen_t addr_len) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active &&
            clients[i].addr_len == addr_len &&
            clients[i].addr.sin_family == addr->sin_family &&
            clients[i].addr.sin_port == addr->sin_port &&
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr) {
            return i;
        }
    }
    return -1;
}

static int add_client(const struct sockaddr_in* addr, socklen_t addr_len) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i].active) {
            clients[i].active = 1;
            clients[i].gameId = -1;
            clients[i].player = -1;
            clients[i].addr = *addr;
            clients[i].addr_len = addr_len;
            return i;
        }
    }
    return -1;
}

static void save_game_state(int gameId) {
    char path[128];
    snprintf(path, sizeof(path), "game_%d.save", gameId);
    savegame((struct gameDB*)&games[gameId], path);
}

static void broadcast_game(int sock_fd, int gameId) {
    if (gameId < 0 || gameId >= MAX_GAMES) {
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].gameId == gameId) {
            send_game_to_client(sock_fd, &clients[i].addr, clients[i].addr_len, &games[gameId]);
        }
    }
}

int main(int argc, char* argv[]) {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "1") == 0) {
        load_saved_games();
    }

    printf("Message board server listening on port %d\n", PORT);

    while (1) {
        struct request req;
        socklen_t client_len = sizeof(client_addr);
        ssize_t received = recvfrom(sock_fd, &req, sizeof(req), 0,
                                    (struct sockaddr *)&client_addr,
                                    &client_len);

        if (received != sizeof(req)) {
            continue;
        } 

        int client_idx = find_client_index(&client_addr, client_len);
        if (client_idx < 0) {
            client_idx = add_client(&client_addr, client_len);
        }

        if (client_idx < 0) {
            continue;
        }

        if (req.type == REQUEST_CREATE) {
            if (next_game_id >= MAX_GAMES) {
                continue;
            }
            int gameId = next_game_id++;
            new_game(&games[gameId], gameId);
            clients[client_idx].gameId = gameId;
            clients[client_idx].player = 0;
            save_game_state(gameId);
            send_game_to_client(sock_fd, &clients[client_idx].addr, clients[client_idx].addr_len, &games[gameId]);
        } else if (req.type == REQUEST_JOIN) {
            int gameId = req.value;
            if (gameId >= 0 && gameId < MAX_GAMES && gameId < next_game_id) {
                clients[client_idx].gameId = gameId;
                clients[client_idx].player = 1;
                broadcast_game(sock_fd, gameId);
                save_game_state(gameId);
            }
        } else if (req.type == REQUEST_MOVE) {
            int gameId = clients[client_idx].gameId;
            int player = clients[client_idx].player;
            int col = req.value;

            if (gameId < 0 && req.gameId >= 0 && req.gameId < next_game_id &&
                req.gameId < MAX_GAMES && (req.player == 0 || req.player == 1)) {
                gameId = req.gameId;
                player = req.player;
                clients[client_idx].gameId = gameId;
                clients[client_idx].player = player;
            }

            if (gameId >= 0 && gameId < MAX_GAMES &&
                gameId < next_game_id &&
                games[gameId].winner == -1 &&
                games[gameId].current_player == player &&
                col >= 0 && col < BOARD_COLS) {
                if (make_move(&games[gameId], player, col)) {
                    broadcast_game(sock_fd, gameId);
                    save_game_state(gameId);
                }
            }
        } else if (req.type == REQUEST_RESUME) {
            int gameId = req.value;
            int player = req.player;

            if (req.gameId != gameId || req.player != games[gameId].current_player) {
                continue;
            }
            
            if (gameId >= 0 && gameId < next_game_id && gameId < MAX_GAMES &&
                (player == 0 || player == 1) && games[gameId].gameId == gameId) {
                client_idx = find_client_index(&client_addr, client_len);
                clients[client_idx].gameId = gameId;
                clients[client_idx].player = player;
                send_game_to_client(sock_fd, &clients[client_idx].addr, clients[client_idx].addr_len, &games[gameId]);
            }
        } 
    }

    close(sock_fd);
    return 0;
}
