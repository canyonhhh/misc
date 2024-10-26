#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

struct game {
	char board[4][4];
	int turn;
	int player_sockets[MAX_CLIENTS];
};

void initialize_game(struct game *g) {
	(void)memset(g->board, (int)'-', sizeof(g->board));
	g->turn = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		g->player_sockets[i] = -1;
	}
}

bool check_for_win(struct game *g) {
	// Check rows for 3 in a row
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			if (g->board[i][j] != '-' && g->board[i][j] == g->board[i][j+1] && g->board[i][j] == g->board[i][j+2]) {
				return true;
			}
		}
	}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			if (g->board[j][i] != '-' && g->board[j][i] == g->board[j+1][i] && g->board[j][i] == g->board[j+2][i]) {
				return true;
			}
		}
	}

	// Check diagonals for 3 in a row
	if ((g->board[0][0] != '-' && g->board[0][0] == g->board[1][1] && g->board[0][0] == g->board[2][2]) ||
			(g->board[1][1] != '-' && g->board[1][1] == g->board[2][2] && g->board[1][1] == g->board[3][3])) {
		return true;
	}

	if ((g->board[0][3] != '-' && g->board[0][3] == g->board[1][2] && g->board[0][3] == g->board[2][1]) ||
			(g->board[1][2] != '-' && g->board[1][2] == g->board[2][1] && g->board[1][2] == g->board[3][0])) {
		return true;
	}

	if (g->board[0][1] != '-' && g->board[0][1] == g->board[1][2] && g->board[0][1] == g->board[2][3]) {
		return true;
	}
	if (g->board[1][0] != '-' && g->board[1][0] == g->board[2][1] && g->board[1][0] == g->board[3][2]) {
		return true;
	}

	if (g->board[0][2] != '-' && g->board[0][2] == g->board[1][1] && g->board[0][2] == g->board[2][0]) {
		return true;
	}
	if (g->board[1][3] != '-' && g->board[1][3] == g->board[2][2] && g->board[1][3] == g->board[3][1]) {
		return true;
	}

	return false;
}

bool check_for_draw(struct game *g) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (g->board[i][j] == '-') {
				return false;
			}
		}
	}
	return true;
}

void print_board(struct game *g) {
	printf("  0 1 2 3\n");
	for (int i = 0; i < 4; i++) {
		printf("%d ", i);
		for (int j = 0; j < 4; j++) {
			printf("%c ", g->board[i][j]);
		}
		printf("\n");
	}
}

int add_client(struct game *g, int socket) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (g->player_sockets[i] == -1) {
			g->player_sockets[i] = socket;
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	int port;
	int listen_socket, client_socket; // server and client socket file descriptors
	struct sockaddr_in server, client; // server and client address structures
	socklen_t client_len; // client address length
	struct game game;
	char buffer[BUFFER_SIZE];
	int maxfd = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[1]);
	if (port < 1024 || port > 65535) {
		fprintf(stderr, "ERROR: Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR: socket");
		exit(EXIT_FAILURE);
	}

	(void)memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET; // IPv4
	server.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all network interfaces
	server.sin_port = htons(port); // listen on the specified port

	if (bind(listen_socket, (struct sockaddr *)&server, sizeof(server)) < 0) { // bind the socket to the server address
		fprintf(stderr, "ERROR: bind");
		exit(EXIT_FAILURE);
	}

	if (listen(listen_socket, MAX_CLIENTS) < 0) { // listen for incoming connections
		fprintf(stderr, "ERROR: listen");
		exit(EXIT_FAILURE);
	}

	initialize_game(&game);

	while (1) {
		fd_set readfds; // set of file descriptors
		FD_ZERO(&readfds); // clear the set
		FD_SET(listen_socket, &readfds); // add the server socket to the set
		maxfd = listen_socket; // maximum file descriptor

		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (game.player_sockets[i] > 0) {
				FD_SET(game.player_sockets[i], &readfds);
				if (game.player_sockets[i] > maxfd) {
					maxfd = game.player_sockets[i];
				}
			}
		}

		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) { // wait for an activity on any of the sockets
			fprintf(stderr, "ERROR: select");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(listen_socket, &readfds)) {
			client_len = sizeof(client);
			if ((client_socket = accept(listen_socket, (struct sockaddr *)&client, &client_len)) < 0) {
				fprintf(stderr, "ERROR: accept");
				exit(EXIT_FAILURE);
			}

			int player = add_client(&game, client_socket);
			if (player < 0) {
				close(client_socket);
				continue;
			}

			if (player == MAX_CLIENTS - 1) {
				char start_message[BUFFER_SIZE] = "START\n";
				for (int i = game.turn + 1; i < MAX_CLIENTS; i++) {
					send(game.player_sockets[i], start_message, strlen(start_message), 0);
				}
				char player_message[BUFFER_SIZE] = "TURN\n";
				send(game.player_sockets[game.turn], player_message, strlen(player_message), 0);
			}
		}

		// Handle player actions
		for (int i = 0; i < MAX_CLIENTS; i++) {

			if (game.player_sockets[i] > 0 && FD_ISSET(game.player_sockets[i], &readfds)) {

				int bytes = recv(game.player_sockets[i], buffer, BUFFER_SIZE, 0);
				if (bytes <= 0) {
					printf("Player %d disconnected\n", i + 1);
					close(game.player_sockets[i]);
					game.player_sockets[i] = -1;

					// Notify remaining players about the disconnection
					char message[BUFFER_SIZE] = "Game terminated due to a player disconnection.\n";
					for (int j = 0; j < MAX_CLIENTS; j++) {
						if (game.player_sockets[j] > 0) {
							send(game.player_sockets[j], message, strlen(message), 0);
							close(game.player_sockets[j]);
							game.player_sockets[j] = -1;
						}
					}

					initialize_game(&game);
					break;
				}

				buffer[bytes] = '\0';
				printf("Player %d: %s\n", i + 1, buffer);

				if (i != game.turn) {
					/*send(game.player_sockets[i], "Not your turn\n", 14, 0);*/
					continue;
				}

				int row, col;
				if (sscanf(buffer, "%d %d", &row, &col) != 2) {
					/*send(game.player_sockets[i], "Invalid input\n", 14, 0);*/
					continue;
				}

				if (row < 0 || row > 3 || col < 0 || col > 3) {
					/*send(game.player_sockets[i], "Invalid input\n", 14, 0);*/
					continue;
				}

				if (game.board[row][col] != '-') {
					/*send(game.player_sockets[i], "Invalid move\n", 13, 0);*/
					continue;
				}

				game.board[row][col] = (i == 0) ? 'X' : (i == 1) ? 'O' : '+';
				game.turn = (game.turn + 1) % MAX_CLIENTS;

				// send move to other players (eg.: X 0 1, O 2 3, + 3 2)
				char move[10];
				snprintf(move, sizeof(move), "%c %d %d\n", game.board[row][col], row, col);
				for (int i = 0; i < MAX_CLIENTS; i++) {
					send(game.player_sockets[i], move, strlen(move), 0);
				}

				send(game.player_sockets[game.turn], "TURN\n", 5, 0);

				if (check_for_win(&game)) {
					send(game.player_sockets[i], "WIN\n", 4, 0);

					for (int i = 0; i < MAX_CLIENTS; i++) {
						if (game.player_sockets[i] != game.player_sockets[(i + 1) % MAX_CLIENTS]) {
							send(game.player_sockets[i], "LOSE\n", 5, 0);
						}
					}


					for (int i = 0; i < MAX_CLIENTS; i++) {
						close(game.player_sockets[i]);
						game.player_sockets[i] = -1;
					}
					initialize_game(&game);
					continue;
				}

				if (check_for_draw(&game)) {
					for (int i = 0; i < MAX_CLIENTS; i++) {
						send(game.player_sockets[i], "DRAW\n", 5, 0);
					}

					for (int i = 0; i < MAX_CLIENTS; i++) {
						close(game.player_sockets[i]);
						game.player_sockets[i] = -1;
					}
					initialize_game(&game);
					continue;
				}

				print_board(&game);

			}
		}
	}

	return 0;
}
