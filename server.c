#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>   // For mmap (Shared Memory)
#include <sys/wait.h>   // For waitpid
#include <pthread.h>    // For threads and mutexes
#include <signal.h>     // For handling Shutdown (Ctrl+C)
#include "game.h"       // Your Blueprint

// Global Pointer to Shared Memory
// We make it global so all functions in this file can see it
SharedGameState *game_state;

// --- Step 1: Initialize Shared Memory ---
void setup_shared_memory() {
    // 1. Allocate the memory using mmap
    // PROT_READ | PROT_WRITE: We can read and write to it
    // MAP_SHARED | MAP_ANONYMOUS: Shared across forks, no file needed
    game_state = mmap(NULL, sizeof(SharedGameState),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (game_state == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // 2. Initialize the Game State
    game_state->player_count = 0;
    game_state->current_player_turn = 1; // Player 1 starts
    game_state->game_over = 0;
    game_state->turn_count = 0;

    // Clear the board
    for(int i=0; i<BOARD_SIZE; i++) {
        for(int j=0; j<BOARD_SIZE; j++) {
            game_state->board[i][j] = 0;
        }
    }

    // 3. Configure Mutex for Cross-Process Sharing
    // THIS IS CRITICAL FOR YOUR ASSIGNMENT
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

    // Initialize the mutex with these shared attributes
    pthread_mutex_init(&game_state->board_mutex, &mutex_attr);
    
    // Clean up attribute object (not needed anymore)
    pthread_mutexattr_destroy(&mutex_attr);

    printf("[Server] Shared memory and mutexes initialized.\n");
}

// --- Step 2: Cleanup on Shutdown ---
// When you press Ctrl+C, this runs to clean up memory
void handle_shutdown(int sig) {
    printf("\n[Server] Shutting down...\n");
    // In a real persistent server, you would save scores.txt here
    
    pthread_mutex_destroy(&game_state->board_mutex);
    munmap(game_state, sizeof(SharedGameState));
    exit(0);
}

int main() {
    // Register the shutdown handler (Ctrl+C)
    signal(SIGINT, handle_shutdown);

    // 1. Setup the memory
    setup_shared_memory();

    // 2. TODO: Create Scheduler and Logger Threads
    // (We will add this next)

    printf("[Server] Waiting for players... (Press Ctrl+C to stop)\n");

    // 3. TODO: Network Loop (Accept -> Fork)
    // (We will add this next)
    
    // Infinite loop to keep server alive for now
    while(1) {
        sleep(1);
    }

    return 0;
}