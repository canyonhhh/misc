#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define GRID_SIZE 4

char grid[GRID_SIZE][GRID_SIZE];
bool game_started = false;
char status_message[BUFFER_SIZE] = "Waiting for other players...";

void draw_grid(WINDOW *win, int current_row, int current_col) {
	werase(win);
	box(win, 0, 0);

	for (int i = 0; i < GRID_SIZE; i++) {
		for (int j = 0; j < GRID_SIZE; j++) {
			mvwaddch(win, i * 2 + 1, j * 4 + 2, grid[i][j] == '-' ? ' ' : grid[i][j]);
		}
	}

	if (game_started) {
		mvwaddch(win, current_row * 2 + 1, current_col * 4 + 2, ' ' | A_REVERSE);
	}
	wrefresh(win);
}

void initialize_grid() {
	for (int i = 0; i < GRID_SIZE; i++) {
		for (int j = 0; j < GRID_SIZE; j++) {
			grid[i][j] = '-';
		}
	}
}

void update_status(WINDOW *status_win, const char *message) {
	werase(status_win);
	mvwprintw(status_win, 1, 1, "%s", message);
	wrefresh(status_win);
}

int main(int argc, char *argv[]) {
	int port;
	int sockfd;
	struct sockaddr_in serveraddr;

	char buffer[BUFFER_SIZE] = {0};

	if (argc != 3){
		fprintf(stderr,"USAGE: %s <ip> <port>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]);

	if ((port < 1) || (port > 65535)){
		fprintf(stderr, "ERROR: Invalid port number.\n");
		exit(EXIT_FAILURE);
	}


	printf("AF_INET: %d\n", AF_INET);
	printf("SOCK_STREAM: %d\n", SOCK_STREAM);


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr,"ERROR: Cannot open socket.\n");
		exit(EXIT_FAILURE);
	}

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET; // ipv4
	serveraddr.sin_port = htons(port);
	if ( inet_aton(argv[1], &serveraddr.sin_addr) <= 0 ) { // Convert IP address to binary
		fprintf(stderr,"ERROR: Invalid remote IP address.\n");
		exit(EXIT_FAILURE);
	}

	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) { // Connect to server
		fprintf(stderr,"ERROR: Cannot connect to server.\n");
		exit(EXIT_FAILURE);
	}

	// Initialize ncurses
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	// Set non-blocking input
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

	initialize_grid();

	WINDOW *grid_win = newwin(GRID_SIZE * 2 + 1, GRID_SIZE * 4 + 1, (LINES - (GRID_SIZE * 2 + 1)) / 2, (COLS - (GRID_SIZE * 4 + 1)) / 2);
	WINDOW *status_win = newwin(3, COLS - 2, LINES - 3, 1);
	box(status_win, 0, 0);
	update_status(status_win, status_message);
	keypad(grid_win, TRUE);

	int current_row = 0, current_col = 0;

	while (1) {
		fd_set readfds; // Set of file descriptors
		FD_ZERO(&readfds); // Clear the set
		FD_SET(sockfd, &readfds); // Add sockfd to the set
		FD_SET(STDIN_FILENO, &readfds); // Add STDIN_FILENO to the set
		int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO; 

		struct timeval tv;
		tv.tv_sec = 1;

		if (select(maxfd + 1, &readfds, NULL, NULL, &tv) < 0) { // Wait for input
			fprintf(stderr, "ERROR: select() failed.\n");
			break;
		}

		if (FD_ISSET(sockfd, &readfds)) { // Check if sockfd is in the set
			char buffer[BUFFER_SIZE] = {0};
			int bytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
			if (bytes < 0) {
				fprintf(stderr, "ERROR: recv() failed.\n");
				break;
			} else if (bytes > 0) {
				if (strncmp(buffer, "START", 5) == 0) {
					game_started = true;
					strcpy(status_message, "Waiting...");
				} 
				else if (strncmp(buffer, "TURN", 4) == 0) {
					strcpy(status_message, "Your turn");
					game_started = true;
				} 
				else if (strncmp(buffer, "WIN", 3) == 0 || strncmp(buffer, "LOSE", 4) == 0 || strncmp(buffer, "DRAW", 4) == 0) {
					game_started = false;
					strncpy(status_message, buffer, sizeof(status_message) - 1);
				} else {
					char symbol;
					int row, col;
					if (sscanf(buffer, "%c %d %d", &symbol, &col, &row) == 3) {
						grid[col][row] = symbol;
						strcpy(status_message, "Waiting...");
					}
				}
				update_status(status_win, status_message);
				if (game_started) {
					draw_grid(grid_win, current_row, current_col);
				}
			} else {
				printw("Server disconnected.\n");
				break;
			}
		}

		if (game_started && FD_ISSET(STDIN_FILENO, &readfds)) {
			int ch = wgetch(grid_win);
			switch (ch) {
				case KEY_UP: case 'k':
					current_row = current_row > 0 ? current_row - 1 : current_row;
					break;
				case KEY_DOWN: case 'j':
					current_row = current_row < GRID_SIZE - 1 ? current_row + 1 : current_row;
					break;
				case KEY_LEFT: case 'h':
					current_col = current_col > 0 ? current_col - 1 : current_col;
					break;
				case KEY_RIGHT: case 'l':
					current_col = current_col < GRID_SIZE - 1 ? current_col + 1 : current_col;
					break;
				case '\n':
					snprintf(buffer, BUFFER_SIZE, "%d %d", current_row, current_col);
					send(sockfd, buffer, strlen(buffer) + 1, 0);
					break;
			}
			draw_grid(grid_win, current_row, current_col);
		}
	}

	delwin(grid_win);
	delwin(status_win);
	endwin();
	close(sockfd);

	return 0;
}
