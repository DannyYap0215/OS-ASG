========================================================================
CSN6214 Operating Systems - Assignment
Concurrent Networked Board Game Interpreter (Hybrid Architecture)
========================================================================

1. PROJECT OVERVIEW
-------------------
This project implements a multiplayer game server using a Hybrid Concurrency 
Model (Multiprocessing + Multithreading). It allows 3 to 5 players to 
connect via TCP sockets to play a turn-based board game.

The system utilizes:
- fork(): For creating isolated processes for each client connection.
- pthreads: For background tasks (Scheduler and Logger) in the server.
- Shared Memory (mmap): For synchronizing game state across processes.
- Process-Shared Mutexes: For safe synchronization between processes.
- TCP Sockets: For client-server communication.

2. GAME DESCRIPTION: "5x5 Tic-Tac-Toe"
--------------------------------------
A variant of Tic-Tac-Toe designed for 3-5 players.

- Board: A 5x5 Grid.
- Players: Supports 3 to 5 simultaneous players.
- Objective: The first player to align 4 of their pieces horizontally 
  or vertically wins the game.
- Draw Condition: If the board fills up (25 moves) without a winner.

3. ARCHITECTURE & MODE
----------------------
Deployment Mode: Multi-machine Mode (TCP Sockets)
- Although tested on a single machine (localhost), the architecture uses 
  IPv4 TCP sockets (AF_INET), allowing clients to theoretically connect 
  from different machines on the same network.

Concurrency Model:
- Main Server Process: Listens for connections.
- Child Processes: One per player (handles game logic and communication).
- Threads (in Parent): 
    1. Logger Thread (writes to game.log)
    2. Scheduler Thread (monitors turn cycles)

4. COMPILATION INSTRUCTIONS
---------------------------
The project includes a Makefile for easy compilation.

Prerequisites: GCC Compiler, Linux Environment (MiniOS/Ubuntu).

Step 1: Open a terminal in the project directory.
Step 2: Run the make command:
    $ make

This will generate two executables: 'server' and 'client'.

To clean up compiled files and logs:
    $ make clean

5. HOW TO RUN AND PLAY
----------------------
You will need multiple terminal windows.

Step 1: Start the Server
    Terminal 1: $ ./server
    (The server initializes shared memory and waits for connections.)

Step 2: Connect Players
    Terminal 2: $ ./client
    Terminal 3: $ ./client
    Terminal 4: $ ./client
    (Minimum 2 players needed to start moving, but built for 3-5).

Step 3: Gameplay
    - The server will prompt the player whose turn it is.
    - Enter your move as "ROW COL" (separated by a space).
    - Example: To move to Row 1, Column 2, type:
      1 2

Step 4: Persistence
    - Upon game completion, scores are saved to 'scores.txt'.
    - Logs are recorded in real-time to 'game.log'.
    - Press Ctrl+C in the server terminal to shut down safely.

6. TESTING AND VERIFICATION (ZOMBIE REAPING)
--------------------------------------------
The server includes a "Visual Proof" mechanism to demonstrate that 
Child Processes are correctly cleaned up (reaped) to prevent Zombies.

Method A: Automated Visual Proof
    1. Connect a client using ./client
    2. Type "exit" in the client terminal.
    3. Observe the Server Terminal.
       - The server will detect the disconnect.
       - It will print "Child Process [PID] terminated".
       - It will automatically run the system command 'ps -f -C server'.
       - You should see ONLY the parent process and remaining clients. 
         No <defunct> processes will appear.

Method B: Manual Verification
    1. Start the server and multiple clients.
    2. Open a new terminal window.
    3. Run the command:
       $ ps -f -C server
    4. Verify that the Process Tree shows one Parent and multiple Children.
    5. Kill one client (Ctrl+C).
    6. Run the command again. The corresponding Child PID should be gone immediately.