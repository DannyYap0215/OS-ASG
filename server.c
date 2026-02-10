#define _GNU_SOURCE // Required for dprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <unistd.h>     
#include <sys/mman.h>   
#include <sys/wait.h>   
#include <pthread.h>    
#include <signal.h>     
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <errno.h>
#include "game.h"       

#define PORT 8080

// Global Pointer to Shared Memory
SharedGameState *game_state;

// Persistence: Load Scores 
void load_scores() {
    FILE *fp = fopen("scores.txt", "r");
    if (fp) {
        int id, score;
        while (fscanf(fp, "%d %d", &id, &score) != EOF) {
            if (id < MAX_PLAYERS) game_state->player_scores[id] = score;
        }
        fclose(fp);
        printf("[Server] Scores loaded from scores.txt\n");
    } else {
        printf("[Server] No scores found. Starting fresh.\n");
        for (int i = 0; i < MAX_PLAYERS; i++) game_state->player_scores[i] = 0;
    }
}

// Persistence: Save Scores 
void save_scores() {
    FILE *fp = fopen("scores.txt", "w");
    if (!fp) {
        perror("Failed to save scores");
        return;
    }
    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (game_state->player_scores[i] > 0) {
            fprintf(fp, "%d %d\n", i, game_state->player_scores[i]);
        }
    }
    fclose(fp);
    printf("[Server] Scores saved to scores.txt\n");
}

// Initialization 
void setup_shared_memory() {
    game_state = mmap(NULL, sizeof(SharedGameState), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (game_state == MAP_FAILED) { perror("mmap failed"); exit(1); }

    // Init Data
    game_state->player_count = 0;
    game_state->current_player_turn = 1; 
    game_state->game_over = 0;
    game_state->turn_count = 0;
    game_state->log_head = 0;
    game_state->log_tail = 0;

    for(int i=0; i<BOARD_SIZE; i++) 
        for(int j=0; j<BOARD_SIZE; j++) game_state->board[i][j] = 0;

    load_scores(); // Load persistence

    // Init Mutexes (Process Shared)
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&game_state->board_mutex, &mutex_attr);
    pthread_mutex_init(&game_state->log_mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr); 

    // Init Cond Var
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&game_state->log_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    printf("[Server] Shared memory initialized.\n");
}

// Cleanup 
void handle_shutdown(int sig) {
    printf("\n[Server] Shutting down...\n");
    save_scores(); // Save persistence
    
    pthread_mutex_destroy(&game_state->board_mutex);
    pthread_mutex_destroy(&game_state->log_mutex);
    pthread_cond_destroy(&game_state->log_cond);
    munmap(game_state, sizeof(SharedGameState));
    exit(0);
}

// Zombie Reaper 
void handle_sigchld(int sig) {
    int saved_errno = errno; 
    pid_t pid;
    
    // Reap all dead children
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        printf("\n[Server] Child Process %d terminated and reaped successfully.\n", pid);
        printf("[Server] Current Process List (Checking for Zombies...):\n");
        printf("---------------------------------------------------\n");
        system("ps -f -C server"); 
        printf("---------------------------------------------------\n");
    }
    
    errno = saved_errno;
}

// Logger Thread 
void *logger_thread(void *arg) {
    FILE *fp = fopen("game.log", "a");
    if (!fp) pthread_exit(NULL);

    while (1) {
        pthread_mutex_lock(&game_state->log_mutex);
        while (game_state->log_head == game_state->log_tail) {
            pthread_cond_wait(&game_state->log_cond, &game_state->log_mutex);
        }
        char *msg = game_state->log_buffer[game_state->log_head];
        fprintf(fp, "%s\n", msg);
        fflush(fp); 
        printf("[Log] %s\n", msg); 
        game_state->log_head = (game_state->log_head + 1) % 50;
        pthread_mutex_unlock(&game_state->log_mutex);
    }
    fclose(fp);
    return NULL;
}

void log_event(char *message) {
    pthread_mutex_lock(&game_state->log_mutex);
    snprintf(game_state->log_buffer[game_state->log_tail], 128, "%s", message);
    game_state->log_tail = (game_state->log_tail + 1) % 50;
    pthread_cond_signal(&game_state->log_cond);
    pthread_mutex_unlock(&game_state->log_mutex);
}

// Game Logic 
int check_win(int player_id) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j <= BOARD_SIZE - WIN_COUNT; j++) {
            int count = 0;
            for (int k = 0; k < WIN_COUNT; k++) 
                if (game_state->board[i][j+k] == player_id) count++;
            if (count == WIN_COUNT) return 1;

            count = 0;
            for (int k = 0; k < WIN_COUNT; k++) 
                if (game_state->board[j+k][i] == player_id) count++;
            if (count == WIN_COUNT) return 1;
        }
    }
    return 0;
}

void serialize_board(char *buffer) {
    strcpy(buffer, "\n--- GAME BOARD ---\n   ");
    char temp[32];
    for(int c=0; c<BOARD_SIZE; c++) { sprintf(temp, "%d ", c); strcat(buffer, temp); }
    strcat(buffer, "\n");
    for (int r = 0; r < BOARD_SIZE; r++) {
        sprintf(temp, "%d |", r); strcat(buffer, temp);
        for (int c = 0; c < BOARD_SIZE; c++) {
            int cell = game_state->board[r][c];
            if (cell == 0) strcat(buffer, ". ");
            else { sprintf(temp, "%d ", cell); strcat(buffer, temp); }
        }
        strcat(buffer, "|\n");
    }
    strcat(buffer, "------------------\n");
}

// Client Worker: Handles Game Logic for One Player 
void handle_client(int client_socket, int player_id) {
    char buffer[1024];
    char board_view[1024];
    int row, col;

    printf("[Player %d] Process started (PID %d)\n", player_id, getpid());

    while (1) {
        // 1. CHECK GAME OVER 
        if (game_state->game_over) {
            if (game_state->winner == player_id) 
                dprintf(client_socket, "GAME OVER: You Won! Total Wins: %d\n", game_state->player_scores[player_id]);
            else 
                dprintf(client_socket, "GAME OVER: Player %d Won.\n", game_state->winner);
            break; // Exit loop to close connection
        }

        // 2. CHECK TURN 
        if (game_state->current_player_turn != player_id) { 
            // Save CPU by sleeping while waiting
            sleep(1); 
            continue; 
        }

        // 3. IT IS MY TURN 
        // A. Send the Board View
        serialize_board(board_view);
        dprintf(client_socket, "%s\nYOUR TURN! Enter 'ROW COL' (e.g. 1 2) or 'exit': ", board_view);

        // B. Read Input
        memset(buffer, 0, 1024);
        int bytes_read = read(client_socket, buffer, 1024);
        
        // Handle unexpected disconnect (ctrl+c)
        if (bytes_read <= 0) {
            goto DISCONNECT;
        }

        // C. Check for "exit" command
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline
        if (strcmp(buffer, "exit") == 0) {
            goto DISCONNECT;
        }

        // D. Parse Coordinates
        if (sscanf(buffer, "%d %d", &row, &col) != 2) {
            dprintf(client_socket, "Invalid Format. Use: ROW COL\n");
            continue;
        }

        // 4. CRITICAL SECTION (Updating Shared Memory) 
        pthread_mutex_lock(&game_state->board_mutex);

        // Validation: Bounds check and Occupied check
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || game_state->board[row][col] != 0) {
            dprintf(client_socket, "Invalid Move! Spot taken or out of bounds.\n");
            pthread_mutex_unlock(&game_state->board_mutex); 
            continue; // Retry input
        }

        // Execute Move
        game_state->board[row][col] = player_id;
        game_state->turn_count++;
        
        // Log the move
        char log_msg[64];
        sprintf(log_msg, "Player %d moved to %d,%d", player_id, row, col);
        log_event(log_msg);
        
        // Check Win Condition
        f (check_win(player_id)) {
        game_state->game_over = 1;
        game_state->winner = player_id;
        game_state->player_scores[player_id]++; 
        log_event("Game Over. Winner found.");
    } 
    else if (game_state->turn_count >= (BOARD_SIZE * BOARD_SIZE)) {
        // No winner, but board is full (25 moves for 5x5)
        game_state->game_over = 1;
        game_state->winner = 0; // 0 indicates a Draw
        log_event("Game Over. It's a DRAW.");
    }

        // TURN SKIPPING 
        // This loop finds the next active player, skipping anyone who quit.
        int next_player = game_state->current_player_turn;
        do {
            next_player = (next_player % game_state->player_count) + 1;
            // Loop safety: stops if we get back to ourselves or find an active player
        } while (game_state->active_players[next_player] == 0 && next_player != player_id);

        game_state->current_player_turn = next_player;

        pthread_mutex_unlock(&game_state->board_mutex);
        // END CRITICAL SECTION 
        
        dprintf(client_socket, "Move accepted. Waiting for other players...\n");
    }

    // Normal Game End Exit
    close(client_socket);
    exit(0);

DISCONNECT:
    // Handle Graceful or Forced Disconnection
    pthread_mutex_lock(&game_state->board_mutex);
    
    // Mark as Inactive so the Scheduler skips me
    game_state->active_players[player_id] = 0; 
    
    // If it was my turn, force pass it to someone else
    if (game_state->current_player_turn == player_id) {
        int next_player = player_id;
        do {
            next_player = (next_player % game_state->player_count) + 1;
        } while (game_state->active_players[next_player] == 0 && next_player != player_id);
        game_state->current_player_turn = next_player;
    }
    
    char d_msg[64];
    sprintf(d_msg, "Player %d disconnected.", player_id);
    log_event(d_msg);
    
    pthread_mutex_unlock(&game_state->board_mutex);
    
    printf("[Server] Player %d disconnected.\n", player_id);
    close(client_socket);
    exit(0);
}

// Scheduler 
void *scheduler_thread(void *arg) {
    int last_turn = 0;
    while (1) {
        if (game_state && game_state->current_player_turn != last_turn) {
            printf("[Scheduler] Turn: Player %d\n", game_state->current_player_turn);
            last_turn = game_state->current_player_turn;
        }
        sleep(1);
    }
    return NULL;
}

// Main 
int main() {
    signal(SIGINT, handle_shutdown); 
    signal(SIGCHLD, handle_sigchld); // REAPER

    setup_shared_memory();

    pthread_t log_tid, sched_tid;
    pthread_create(&log_tid, NULL, logger_thread, NULL);
    pthread_create(&sched_tid, NULL, scheduler_thread, NULL);
    log_event("Server started.");

    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1, addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("[Server] Listening on port %d...\n", PORT);

    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        pthread_mutex_lock(&game_state->board_mutex);
        int current_id = ++game_state->player_count; 
        game_state->active_players[current_id] = 1;
        pthread_mutex_unlock(&game_state->board_mutex);

        printf("[Server] Player %d connected.\n", current_id);

        if (fork() == 0) {
            close(server_fd); 
            handle_client(client_socket, current_id); 
            exit(0); 
        } else {
            close(client_socket); 
        }
    }
    return 0;
}