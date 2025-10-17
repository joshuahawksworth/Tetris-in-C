#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// platform-specific includes for keyboard input
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
#endif

// constants
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define EMPTY 0
#define FILLED 1

int SHAPES[7][4][4] = {
    // I shape
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
    // O shape
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
    // T shape
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},
    // S shape
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
    // Z shape
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
    // J shape
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},
    // L shape
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}
};

typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    int current_shape[4][4];
    int current_x;
    int current_y;
    int score;
    bool game_over;
} Game;

// function prototypes
void init_game(Game *game);
void draw_board(Game *game);
bool check_collision(Game *game, int offset_x, int offset_y);
void merge_piece(Game *game);
void spawn_piece(Game *game);
void clear_lines(Game *game);
void rotate_piece(Game *game);
void move_piece(Game *game, int dx, int dy);
int kbhit(void);
char getch_wrapper(void);

// platform-specific keyboard handling
#ifndef _WIN32
int kbhit(void) {
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

char getch_wrapper(void) {
    char ch;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#else
char getch_wrapper(void) {
    return getch();
}
#endif

void init_game(Game *game) {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            game->board[i][j] = EMPTY;
        }
    }
    game->score = 0;
    game->game_over = false;
    spawn_piece(game);
}

void draw_board(Game *game) {
    system("clear || cls");
    printf("TETRIS - Score: %d\n", game->score);
    printf("┌");
    for (int i = 0; i < BOARD_WIDTH; i++) printf("──");
    printf("┐\n");
    
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        printf("│");
        for (int j = 0; j < BOARD_WIDTH; j++) {
            bool is_current = false;
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (game->current_shape[y][x] && 
                        game->current_x + x == j && 
                        game->current_y + y == i) {
                        is_current = true;
                    }
                }
            }
            if (is_current) {
                printf("██");
            } else if (game->board[i][j]) {
                printf("▓▓");
            } else {
                printf("  ");
            }
        }
        printf("│\n");
    }
    
    printf("└");
    for (int i = 0; i < BOARD_WIDTH; i++) printf("──");
    printf("┘\n");
    printf("Controls: A/D - Move, W - Rotate, S - Drop, Q - Quit\n");
}

bool check_collision(Game *game, int offset_x, int offset_y) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (game->current_shape[y][x]) {
                int new_x = game->current_x + x + offset_x;
                int new_y = game->current_y + y + offset_y;
                
                if (new_x < 0 || new_x >= BOARD_WIDTH || 
                    new_y >= BOARD_HEIGHT) {
                    return true;
                }
                if (new_y >= 0 && game->board[new_y][new_x]) {
                    return true;
                }
            }
        }
    }
    return false;
}

void merge_piece(Game *game) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (game->current_shape[y][x]) {
                int board_y = game->current_y + y;
                int board_x = game->current_x + x;
                if (board_y >= 0 && board_y < BOARD_HEIGHT && 
                    board_x >= 0 && board_x < BOARD_WIDTH) {
                    game->board[board_y][board_x] = FILLED;
                }
            }
        }
    }
}

void spawn_piece(Game *game) {
    int shape_type = rand() % 7;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            game->current_shape[y][x] = SHAPES[shape_type][y][x];
        }
    }
    game->current_x = BOARD_WIDTH / 2 - 2;
    game->current_y = 0;
    
    if (check_collision(game, 0, 0)) {
        game->game_over = true;
    }
}

void clear_lines(Game *game) {
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (game->board[i][j] == EMPTY) {
                full = false;
                break;
            }
        }
        if (full) {
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    game->board[k][j] = game->board[k-1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++) {
                game->board[0][j] = EMPTY;
            }
            game->score += 100;
            i++;
        }
    }
}

void rotate_piece(Game *game) {
    int temp[4][4];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            temp[y][x] = game->current_shape[3-x][y];
        }
    }
    
    int old_shape[4][4];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            old_shape[y][x] = game->current_shape[y][x];
            game->current_shape[y][x] = temp[y][x];
        }
    }
    
    if (check_collision(game, 0, 0)) {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                game->current_shape[y][x] = old_shape[y][x];
            }
        }
    }
}

void move_piece(Game *game, int dx, int dy) {
    if (!check_collision(game, dx, dy)) {
        game->current_x += dx;
        game->current_y += dy;
    } else if (dy > 0) {
        merge_piece(game);
        clear_lines(game);
        spawn_piece(game);
    }
}

int main() {
    srand(time(NULL));
    Game game;
    init_game(&game);
    
    clock_t last_fall = clock();
    double fall_speed = 0.5;
    
    while (!game.game_over) {
        if (kbhit()) {
            char c = getch_wrapper();
            switch (c) {
                case 'a':
                case 'A':
                    move_piece(&game, -1, 0);
                    break;
                case 'd':
                case 'D':
                    move_piece(&game, 1, 0);
                    break;
                case 'w':
                case 'W':
                    rotate_piece(&game);
                    break;
                case 's':
                case 'S':
                    move_piece(&game, 0, 1);
                    break;
                case 'q':
                case 'Q':
                    game.game_over = true;
                    break;
            }
        }
        
        double elapsed = (double)(clock() - last_fall) / CLOCKS_PER_SEC;
        if (elapsed > fall_speed) {
            move_piece(&game, 0, 1);
            last_fall = clock();
        }
        
        draw_board(&game);
        
#ifdef _WIN32
        Sleep(50);
#else
        usleep(50000);
#endif
    }
    
    printf("\nGame Over! Final Score: %d\n", game.score);
    return 0;
}