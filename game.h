#ifndef GAME_H
#define GAME_H

#include <pthread.h>

#define BOARD_SIZE 5       // 5x5 Grid
#define WIN_COUNT 4        // Need 4 in a row to win
#define MAX_PLAYERS 5      // Assignment requires support for up to 5

// This structure will be placed in Shared Memory
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE]; // 0=Empty, 1=P1, 2=P2, etc.
    int player_count;                  // Number of active players (3-5)
    int current_player_turn;           // Whose turn is it? (1, 2, 3...)
    int game_over;                     // 0=Running, 1=Win, 2=Draw
    int winner;                        // ID of the winner (0 if none)
    int turn_count;                    // Total moves made (for Draw check)
    int player_scores[MAX_PLAYERS];
    int active_players[MAX_PLAYERS];

    char log_buffer[50][128];  
    int log_head;  // Where the Logger reads from
    int log_tail;  // Where the Players write to
    
    // Mutex specifically for the log (separate from board to avoid lag)
    pthread_mutex_t log_mutex; 
    pthread_cond_t log_cond;

    // Synchronization Primitives (REQUIRED by Assignment)
    pthread_mutex_t board_mutex;       // Protects board access
    pthread_cond_t turn_cond;          // Signals when turn changes
} SharedGameState;

#endif