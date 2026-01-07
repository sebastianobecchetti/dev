#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BOARD_WIDTH 12
#define BOARD_HEIGHT 22
#define TETROMINO_SIZE 4

// The playfield
unsigned char board[BOARD_HEIGHT][BOARD_WIDTH];

// Game state
int score = 0;
bool game_over = false;
int fall_speed = 1000; // Initial fall speed in milliseconds

// Tetromino shapes (I, O, T, S, Z, L, J)
const int tetromino[7][4][TETROMINO_SIZE][TETROMINO_SIZE] = {
    // I
    {{{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}},
     {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}}},
    // O
    {{{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}}},
    // T
    {{{0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}}},
    // S
    {{{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
     {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
     {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}},
    // Z
    {{{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}},
     {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}}},
    // L
    {{{0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0}},
     {{1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}},
     {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
     {{0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0}}},
    // J
    {{{0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0}},
     {{0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0}},
     {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
     {{0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}}}
};

// Current falling piece
struct {
    int x, y;
    int type;
    int rotation;
} current_piece;

// Function Prototypes
void init_game();
void spawn_piece();
bool check_collision(int px, int py, int prot);
void lock_piece();
void clear_lines();
void draw_board();
void game_loop();

void init_game() {
    srand(time(NULL));
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = (j == 0 || j == BOARD_WIDTH - 1 || i == BOARD_HEIGHT - 1) ? 2 : 0;
        }
    }
    score = 0;
    game_over = false;
    spawn_piece();
}

void spawn_piece() {
    current_piece.y = 0;
    current_piece.x = BOARD_WIDTH / 2 - TETROMINO_SIZE / 2;
    current_piece.type = rand() % 7;
    current_piece.rotation = 0;
    if (check_collision(current_piece.x, current_piece.y, current_piece.rotation)) {
        game_over = true;
    }
}

bool check_collision(int px, int py, int prot) {
    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino[current_piece.type][prot][i][j] && board[py + i][px + j]) {
                return true;
            }
        }
    }
    return false;
}

void lock_piece() {
    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino[current_piece.type][current_piece.rotation][i][j]) {
                board[current_piece.y + i][current_piece.x + j] = 1;
            }
        }
    }
}

void clear_lines() {
    int lines_cleared = 0;
    for (int i = BOARD_HEIGHT - 2; i >= 0; i--) {
        bool line_full = true;
        for (int j = 1; j < BOARD_WIDTH - 1; j++) {
            if (board[i][j] == 0) {
                line_full = false;
                break;
            }
        }

        if (line_full) {
            lines_cleared++;
            for (int k = i; k > 0; k--) {
                for (int j = 1; j < BOARD_WIDTH - 1; j++) {
                    board[k][j] = board[k - 1][j];
                }
            }
            // Redraw the cleared line from the top
            for (int j = 1; j < BOARD_WIDTH - 1; j++) {
                board[0][j] = 0;
            }
            i++; // Check the same line again
        }
    }
    score += lines_cleared * lines_cleared * 100;
    if (lines_cleared > 0 && fall_speed > 100) {
        fall_speed -= 25; // Increase speed
    }
}

void draw_board() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == 2) {
                mvprintw(i, j * 2, "[]");
            } else if (board[i][j] == 1) {
                mvprintw(i, j * 2, "##");
            } else {
                mvprintw(i, j*2, "  ");
            }
        }
    }

    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino[current_piece.type][current_piece.rotation][i][j]) {
                mvprintw(current_piece.y + i, (current_piece.x + j) * 2, "##");
            }
        }
    }
    mvprintw(2, (BOARD_WIDTH + 2) * 2, "Score: %d", score);
    mvprintw(4, (BOARD_WIDTH + 2) * 2, "Use Arrow Keys");
    mvprintw(5, (BOARD_WIDTH + 2) * 2, "Up: Rotate");
    refresh();
}

void game_loop() {
    int ch;
    nodelay(stdscr, TRUE);

    long timer = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long last_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;


    while (!game_over) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        long current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        long delta_time = current_time - last_time;
        last_time = current_time;
        timer += delta_time;

        if ((ch = getch()) != ERR) {
            switch (ch) {
                case KEY_LEFT:
                    if (!check_collision(current_piece.x - 1, current_piece.y, current_piece.rotation))
                        current_piece.x--;
                    break;
                case KEY_RIGHT:
                    if (!check_collision(current_piece.x + 1, current_piece.y, current_piece.rotation))
                        current_piece.x++;
                    break;
                case KEY_DOWN:
                    if (!check_collision(current_piece.x, current_piece.y + 1, current_piece.rotation))
                        current_piece.y++;
                    timer = 0; // Reset fall timer
                    break;
                case KEY_UP:
                    int next_rotation = (current_piece.rotation + 1) % 4;
                    if (!check_collision(current_piece.x, current_piece.y, next_rotation))
                        current_piece.rotation = next_rotation;
                    break;
                case 'q':
                case 'Q':
                    game_over = true;
                    break;
            }
        }

        if (timer > fall_speed) {
            timer = 0;
            if (!check_collision(current_piece.x, current_piece.y + 1, current_piece.rotation)) {
                current_piece.y++;
            } else {
                lock_piece();
                clear_lines();
                spawn_piece();
            }
        }

        erase();
        draw_board();
        usleep(10000); // 10ms sleep to reduce CPU usage
    }
}


int main() {
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled
    noecho();             // Don't echo() while we do getch
    keypad(stdscr, TRUE); // Enable function keys (like arrow keys)
    curs_set(0);          // Hide cursor

    init_game();
    game_loop();

    endwin(); // End curses mode

    printf("Game Over!\nFinal Score: %d\n", score);

    return 0;
}
