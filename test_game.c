// test_game.c
#include <stdio.h>
#include "game.h"
#include "game_logic.c" // Usually you compile separately, but including for quick test

int main() {
    SharedGameState game = {0}; // Initialize empty game
    
    printf("--- Testing 5x5 Tic Tac Toe Logic ---\n");
    
    // Simulate Player 1 doing a vertical win
    make_move(&game, 1, 0, 0); // P1 at (0,0)
    make_move(&game, 2, 0, 1); // P2 at (0,1)
    make_move(&game, 1, 1, 0); // P1 at (1,0)
    make_move(&game, 2, 0, 2); // P2 at (0,2)
    make_move(&game, 1, 2, 0); // P1 at (2,0)
    make_move(&game, 2, 0, 3); // P2 at (0,3)
    
    print_board(&game);
    
    // Winning Move
    make_move(&game, 1, 3, 0); // P1 at (3,0) -> Should trigger Win
    print_board(&game);

    if (game.game_over == 1 && game.winner == 1) {
        printf("SUCCESS: Player 1 Win Detected!\n");
    } else {
        printf("FAIL: Win logic not working.\n");
    }

    return 0;
}