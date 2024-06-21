#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_SIZE 9
#define AI_MARK 'X'
#define PLAYER_MARK 'O'
#define EMPTY '_'

// Function declarations
void print_board(char board[]);
void print_error_and_exit();
int validate_input(int argc, char *argv[]);
void initialize_board(char board[]);
void ai_move(char board[], int strategy[]);
void player_move(char board[]);
int check_winner(char board[], char mark);
void check_game_status(char board[]);

int main(int argc, char *argv[]) {
    char board[BOARD_SIZE];
    int move_strategy[BOARD_SIZE];

    // Validate the input arguments
    if (!validate_input(argc, argv)) {
        printf("Validation failed: Invalid input\n");
        print_error_and_exit();
    }

    // Convert the strategy string into an array of integers
    for (int i = 0; i < BOARD_SIZE; i++) {
        move_strategy[i] = argv[1][i] - '1';
    }

    // Initialize the game board
    initialize_board(board);
    // Print the initial state of the board
    print_board(board);
    fflush(stdout);

    // Main game loop
    while (1) {
        // AI makes its move
        ai_move(board, move_strategy);
        // Print the board after AI's move
        print_board(board);
        fflush(stdout);
        // Check if the game has ended after AI's move
        check_game_status(board);

        // Player makes their move
        player_move(board);
        // Print the board after player's move
        print_board(board);
        fflush(stdout);
        // Check if the game has ended after player's move
        check_game_status(board);
    }

    return 0;
}

// Print error message and exit the program
void print_error_and_exit() {
    exit(EXIT_FAILURE);
}

int validate_input(int argc, char *argv[]) {
    // Check if the number of arguments is less than 2 or if the length of the first argument is not 9
    if (argc < 2 || strlen(argv[1]) != 9) {
        return 0; // Return 0 indicating validation failure
    }

    int digit_count[10] = {0}; // Initialize an array to count the occurrences of each digit
    for (int i = 0; i < 9; i++) {
        char c = argv[1][i];
        // Check if the character is not between '1' and '9'
        if (c < '1' || c > '9') {
            return 0; // Return 0 indicating validation failure
        }
        digit_count[c - '0']++; // Increment the count of the current digit
    }

    // Check if each digit from 1 to 9 occurs exactly once
    for (int i = 1; i <= 9; i++) {
        if (digit_count[i] != 1) {
            return 0; // Return 0 indicating validation failure
        }
    }

    return 1; // Return 1 indicating validation success
}

// Initialize the game board with empty spaces
void initialize_board(char board[]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        board[i] = EMPTY;
    }
}

// Print the current state of the game board
void print_board(char board[]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%c ", board[i]);
        if ((i + 1) % 3 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

// Make the AI's move based on the provided strategy
void ai_move(char board[], int strategy[]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[strategy[i]] == EMPTY) {
            board[strategy[i]] = AI_MARK;
            printf("AI played:%d\n", strategy[i] + 1);
            return;
        }
    }
}

// Prompt the player to make their move and update the board
void player_move(char board[]) {
    int move;
    while (1) {
        printf("Enter your move (1-9): \n");
        if (scanf("%d", &move) != 1 || move < 1 || move > BOARD_SIZE) {
            printf("Invalid move. Try again.\n");
            while (getchar() != '\n'); // Clear the input buffer
        } else {
            move--;  // Adjust for 0-based indexing
            if (board[move] != EMPTY) {
                printf("Error, position already taken. Try again.\n");
            } else {
                printf("You played:%d\n", move+1);
                board[move] = PLAYER_MARK;
                return; // Valid move, exit loop
            }
        }
    }
}

// Check if the specified mark (AI or player) has won
int check_winner(char board[], char mark) {
    int win_positions[8][3] = {
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Columns
            {0, 4, 8}, {2, 4, 6}  // Diagonals
    };

    for (int i = 0; i < 8; i++) {
        if (board[win_positions[i][0]] == mark &&
            board[win_positions[i][1]] == mark &&
            board[win_positions[i][2]] == mark) {
            return 1;
        }
    }
    return 0;
}

// Check the game status to see if there's a winner or if it's a draw
void check_game_status(char board[]) {
    if (check_winner(board, AI_MARK)) {
        printf("I win\n");
        exit(EXIT_SUCCESS);
    }
    if (check_winner(board, PLAYER_MARK)) {
        printf("I lost\n");
        exit(EXIT_SUCCESS);
    }
    int draw = 1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == EMPTY) {
            draw = 0;
            break;
        }
    }
    if (draw) {
        printf("DRAW\n");
        exit(EXIT_SUCCESS);
    }
}
