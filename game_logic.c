#include <stdio.h>
#include <stdlib.h>
#include "game.h"

// --- Helper: Check boundaries ---
int is_valid_move(SharedGameState *game, int row, int col) {
    if (game->game_over) return 0;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return 0; // Out of bounds
    if (game->board[row][col] != 0) return 0; // Already occupied
    return 1;
}

// --- Helper: Check if a specific player has won ---
int check_win(SharedGameState *game, int player_id) {
    int count;

    // 1. Check Rows
    for (int r = 0; r < BOARD_SIZE; r++) {
        count = 0;
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (game->board[r][c] == player_id) count++;
            else count = 0;
            if (count == WIN_COUNT) return 1;
        }
    }

    // 2. Check Columns
    for (int c = 0; c < BOARD_SIZE; c++) {
        count = 0;
        for (int r = 0; r < BOARD_SIZE; r++) {
            if (game->board[r][c] == player_id) count++;
            else count = 0;
            if (count == WIN_COUNT) return 1;
        }
    }

    // 3. Check Diagonal (Top-Left to Bottom-Right)
    // Simplified: Just scanning all possible 4-in-a-row diagonals
    for (int r = 0; r <= BOARD_SIZE - WIN_COUNT; r++) {
        for (int c = 0; c <= BOARD_SIZE - WIN_COUNT; c++) {
            int win = 1;
            for (int k = 0; k < WIN_COUNT; k++) {
                if (game->board[r+k][c+k] != player_id) {
                    win = 0;
                    break;
                }
            }
            if (win) return 1;
        }
    }

    // 4. Check Anti-Diagonal (Top-Right to Bottom-Left)
    for (int r = 0; r <= BOARD_SIZE - WIN_COUNT; r++) {
        for (int c = WIN_COUNT - 1; c < BOARD_SIZE; c++) {
            int win = 1;
            for (int k = 0; k < WIN_COUNT; k++) {
                if (game->board[r+k][c-k] != player_id) {
                    win = 0;
                    break;
                }
            }
            if (win) return 1;
        }
    }

    return 0;
}

// --- Main Action: Make a Move ---
// Returns: 1 if move successful, 0 if invalid
int make_move(SharedGameState *game, int player_id, int row, int col) {
    if (!is_valid_move(game, row, col)) return 0;

    // Apply Move
    game->board[row][col] = player_id;
    game->turn_count++;

    // Check Win
    if (check_win(game, player_id)) {
        game->game_over = 1;
        game->winner = player_id;
    }
    // Check Draw (Board Full)
    else if (game->turn_count >= BOARD_SIZE * BOARD_SIZE) {
        game->game_over = 2; // Draw
    }
    
    return 1;
}

// --- Debug: Print Board to Console ---
void print_board(SharedGameState *game) {
    printf("\n Current Board:\n");
    printf("   0 1 2 3 4\n");
    for (int r = 0; r < BOARD_SIZE; r++) {
        printf("%d |", r);
        for (int c = 0; c < BOARD_SIZE; c++) {
            char symbol = '.';
            if (game->board[r][c] == 1) symbol = 'X';
            else if (game->board[r][c] == 2) symbol = 'O';
            else if (game->board[r][c] == 3) symbol = 'T'; // Triangle?
            else if (game->board[r][c] == 4) symbol = 'S'; // Square?
            else if (game->board[r][c] == 5) symbol = 'P'; // Pentagon?
            printf("%c|", symbol);
        }
        printf("\n");
    }
    printf("\n");
}