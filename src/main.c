#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
#endif

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define EMPTY 0
#define FILLED 1

// ANSI color codes
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN "\033[96m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_PURPLE "\033[95m"
#define COLOR_GREEN "\033[92m"
#define COLOR_RED "\033[91m"
#define COLOR_BLUE "\033[94m"
#define COLOR_ORANGE "\033[38;5;208m"
#define COLOR_GRAY "\033[90m"

int SHAPES[7][4][4] = {
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, // I
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}}, // O
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}}, // T
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}}, // S
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}}, // Z
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}}, // J
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}  // L
};

const char* COLORS[7] = {
    COLOR_CYAN, COLOR_YELLOW, COLOR_PURPLE, 
    COLOR_GREEN, COLOR_RED, COLOR_BLUE, COLOR_ORANGE
};

typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    int board_colors[BOARD_HEIGHT][BOARD_WIDTH];
    int current_shape[4][4];
    int next_shape[4][4];
    int current_type;
    int next_type;
    int current_x;
    int current_y;
    int score;
    int high_score;
    int lines_cleared;
    int level;
    bool game_over;
    bool paused;
} Game;

void init_game(Game *game);
void draw_board(Game *game);
bool check_collision(Game *game, int offset_x, int offset_y, int shape[4][4]);
void merge_piece(Game *game);
void spawn_piece(Game *game);
void clear_lines(Game *game);
void rotate_piece(Game *game);
void move_piece(Game *game, int dx, int dy);
void hard_drop(Game *game);
int calculate_ghost_y(Game *game);
void load_high_score(Game *game);
void save_high_score(Game *game);
int kbhit(void);
char getch_wrapper(void);

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

void load_high_score(Game *game) {
    FILE *file = fopen("highscore.txt", "r");
    if (file) {
        fscanf(file, "%d", &game->high_score);
        fclose(file);
    } else {
        game->high_score = 0;
    }
}

void save_high_score(Game *game) {
    if (game->score > game->high_score) {
        game->high_score = game->score;
        FILE *file = fopen("highscore.txt", "w");
        if (file) {
            fprintf(file, "%d", game->high_score);
            fclose(file);
        }
    }
}

void copy_shape(int dest[4][4], int src[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            dest[y][x] = src[y][x];
        }
    }
}

void init_game(Game *game) {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            game->board[i][j] = EMPTY;
            game->board_colors[i][j] = 0;
        }
    }
    game->score = 0;
    game->lines_cleared = 0;
    game->level = 1;
    game->game_over = false;
    game->paused = false;
    load_high_score(game);
    
    game->next_type = rand() % 7;
    copy_shape(game->next_shape, SHAPES[game->next_type]);
    spawn_piece(game);
}

int calculate_ghost_y(Game *game) {
    int ghost_y = game->current_y;
    while (!check_collision(game, 0, ghost_y - game->current_y + 1, game->current_shape)) {
        ghost_y++;
    }
    return ghost_y;
}

void draw_board(Game *game) {
    system("cls");
    
    printf("TETRIS\n");
    printf("Score: %d  High: %d  Lines: %d  Level: %d\n\n", 
           game->score, game->high_score, game->lines_cleared, game->level);
    
    if (game->paused) {
        printf("          PAUSED\n");
        printf("    Press P to continue\n\n");
    }
    
    int ghost_y = calculate_ghost_y(game);
    
    printf("    +");
    for (int i = 0; i < BOARD_WIDTH; i++) printf("--");
    printf("+   Next:\n");
    
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        printf("    |");
        for (int j = 0; j < BOARD_WIDTH; j++) {
            bool is_current = false;
            bool is_ghost = false;
            
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (game->current_shape[y][x]) {
                        if (game->current_x + x == j && game->current_y + y == i) {
                            is_current = true;
                        }
                        if (game->current_x + x == j && ghost_y + y == i) {
                            is_ghost = true;
                        }
                    }
                }
            }
            
            if (is_current) {
                printf("%s[]%s", COLORS[game->current_type], COLOR_RESET);
            } else if (is_ghost && !game->paused) {
                printf("%s::%s", COLOR_GRAY, COLOR_RESET);
            } else if (game->board[i][j]) {
                printf("%s##%s", COLORS[game->board_colors[i][j]], COLOR_RESET);
            } else {
                printf("  ");
            }
        }
        printf("|");
        
        if (i >= 2 && i < 6) {
            printf("   ");
            for (int x = 0; x < 4; x++) {
                if (game->next_shape[i-2][x]) {
                    printf("%s[]%s", COLORS[game->next_type], COLOR_RESET);
                } else {
                    printf("  ");
                }
            }
        }
        printf("\n");
    }
    
    printf("    +");
    for (int i = 0; i < BOARD_WIDTH; i++) printf("--");
    printf("+\n\n");
    printf("Controls: A/D-Move  W-Rotate  S-Soft Drop  SPACE-Hard Drop\n");
    printf("          P-Pause   Q-Quit\n");
}

bool check_collision(Game *game, int offset_x, int offset_y, int shape[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (shape[y][x]) {
                int new_x = game->current_x + x + offset_x;
                int new_y = game->current_y + y + offset_y;
                
                if (new_x < 0 || new_x >= BOARD_WIDTH || new_y >= BOARD_HEIGHT) {
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
                    game->board_colors[board_y][board_x] = game->current_type;
                }
            }
        }
    }
}

void spawn_piece(Game *game) {
    game->current_type = game->next_type;
    copy_shape(game->current_shape, game->next_shape);
    
    game->next_type = rand() % 7;
    copy_shape(game->next_shape, SHAPES[game->next_type]);
    
    game->current_x = BOARD_WIDTH / 2 - 2;
    game->current_y = 0;
    
    if (check_collision(game, 0, 0, game->current_shape)) {
        game->game_over = true;
        save_high_score(game);
    }
}

void clear_lines(Game *game) {
    int lines = 0;
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (game->board[i][j] == EMPTY) {
                full = false;
                break;
            }
        }
        if (full) {
            lines++;
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    game->board[k][j] = game->board[k-1][j];
                    game->board_colors[k][j] = game->board_colors[k-1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; j++) {
                game->board[0][j] = EMPTY;
            }
            i++;
        }
    }
    
    if (lines > 0) {
        game->lines_cleared += lines;
        int points[] = {0, 100, 300, 500, 800};
        game->score += points[lines] * game->level;
        game->level = 1 + game->lines_cleared / 10;
    }
}

void rotate_piece(Game *game) {
    int temp[4][4];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            temp[y][x] = game->current_shape[3-x][y];
        }
    }
    
    if (!check_collision(game, 0, 0, temp)) {
        copy_shape(game->current_shape, temp);
    }
}

void move_piece(Game *game, int dx, int dy) {
    if (!check_collision(game, dx, dy, game->current_shape)) {
        game->current_x += dx;
        game->current_y += dy;
    } else if (dy > 0) {
        merge_piece(game);
        clear_lines(game);
        spawn_piece(game);
    }
}

void hard_drop(Game *game) {
    while (!check_collision(game, 0, 1, game->current_shape)) {
        game->current_y++;
        game->score += 2;
    }
    merge_piece(game);
    clear_lines(game);
    spawn_piece(game);
}

int main() {
    srand(time(NULL));
    Game game;
    init_game(&game);
    
    clock_t last_fall = clock();
    
    while (!game.game_over) {
        if (kbhit()) {
            char c = getch_wrapper();
            if (!game.paused) {
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
                        game.score += 1;
                        break;
                    case ' ':
                        hard_drop(&game);
                        break;
                    case 'p':
                    case 'P':
                        game.paused = true;
                        break;
                    case 'q':
                    case 'Q':
                        game.game_over = true;
                        break;
                }
            } else {
                if (c == 'p' || c == 'P') {
                    game.paused = false;
                    last_fall = clock();
                }
            }
        }
        
        if (!game.paused) {
            double fall_speed = 0.5 - (game.level - 1) * 0.05;
            if (fall_speed < 0.1) fall_speed = 0.1;
            
            double elapsed = (double)(clock() - last_fall) / CLOCKS_PER_SEC;
            if (elapsed > fall_speed) {
                move_piece(&game, 0, 1);
                last_fall = clock();
            }
        }
        
        draw_board(&game);
        
#ifdef _WIN32
        Sleep(50);
#else
        usleep(50000);
#endif
    }
    
    system("cls");
    printf("\n  GAME OVER!\n\n");
    printf("  Final Score: %d\n", game.score);
    printf("  High Score:  %d\n", game.high_score);
    printf("  Lines:       %d\n", game.lines_cleared);
    printf("  Level:       %d\n\n", game.level);
    printf("  Press R to restart or Q to quit\n");
    
    while (true) {
        if (kbhit()) {
            char c = getch_wrapper();
            if (c == 'r' || c == 'R') {
                init_game(&game);
                main();
                break;
            } else if (c == 'q' || c == 'Q') {
                break;
            }
        }
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif
    }
    
    return 0;
}