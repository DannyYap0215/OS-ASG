========================================================================
CSN6214 Operating Systems Project
========================================================================

1. MODE SUPPORTED
-----------------
* Deployment Mode: Networked (TCP Sockets).
* Concurrency Architecture: Hybrid Model.
  - Multiprocessing (fork): Used for client isolation and game logic.
  - Multithreading (pthread): Used for the Logger and Scheduler.
  - Shared Memory (mmap): Used for synchronizing the game board and state.

2. HOW TO COMPILE
-----------------
This project uses a Makefile for automated compilation.

* To compile the server and client:
  $ make

* To remove compiled files and logs (clean up):
  $ make clean

3. HOW TO RUN
-------------
You will need at least 3 terminal windows to simulate a multiplayer game.

Step 1: Start the Server (Terminal 1)
  $ ./server

Step 2: Start Player 1 (Terminal 2)
  $ ./client

Step 3: Start Player 2 (Terminal 3)
  $ ./client

(You can add more players by opening more terminals and running ./client)

4. GAMEPLAY COMMANDS
--------------------
When it is your turn, the server will prompt you.

* To Make a Move:
  Format: ROW COL
  Example: 2 2   (Places your piece in the center)
  Example: 0 0   (Places your piece in the top-left)

* To Quit the Game:
  Type: exit
  (The server will detect this and automatically skip your turn).

5. HOW TO CHECK STATUS (VERIFY ZOMBIE REAPING)
----------------------------------------------
To prove that the server correctly cleans up dead processes (Zombies):

1. While the game is running, open a new terminal.
2. Run this command:
   $ ps -f -C server

3. Verification:
   - You should see ONE Parent process.
   - You should see ONE Child process per active player.
   - You should NOT see any <defunct> or 'Z' processes.

6. GAME RULES SUMMARY
---------------------
* Game: 5x5 Tic-Tac-Toe Variant.
* Players: Supports 3 to 5 simultaneous players.
* Winning Condition: The first player to get 4 consecutive pieces in a row 
  (Horizontal or Vertical).
* Draw Condition: If the board fills up (25 moves) with no winner.