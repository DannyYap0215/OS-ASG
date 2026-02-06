#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Global socket variable so the thread can access it
int sock = 0;

// The Receiver Thread 
// This runs in the background. It listens for messages from the Server
// and prints them to your screen immediately.
void *receive_message(void *socket_desc) {
    int valread;
    char buffer[BUFFER_SIZE];
    
    while(1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear buffer
        
        // Blocking call: waits here until data arrives
        valread = read(sock, buffer, BUFFER_SIZE);
        
        if (valread > 0) {
            // Print whatever the server sent
            printf("%s", buffer);
            
            // If server says "Game Over", we can exit (optional logic)
            if (strstr(buffer, "Game Over") != NULL) {
                printf("\nExiting...\n");
                close(sock);
                exit(0);
            }
        } 
        else if (valread == 0) {
            printf("\nServer disconnected.\n");
            close(sock);
            exit(0);
        } else {
            perror("Read error");
            exit(1);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    char send_buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    // 1. Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and Connect to Localhost (127.0.0.1)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed. Is the server running?");
        return -1;
    }

    printf("Connected to Server! Waiting for game to start...\n");

    // 2. Start the Receiver Thread
    // This allows us to see server updates while we are typing
    if (pthread_create(&recv_thread, NULL, receive_message, NULL) != 0) {
        perror("Failed to create thread");
        return -1;
    }

    // 3. Main Loop: Send User Input
    while(1) {
        // Read input from keyboard
        memset(send_buffer, 0, BUFFER_SIZE);
        fgets(send_buffer, BUFFER_SIZE, stdin);

        // Remove the newline character at the end
        send_buffer[strcspn(send_buffer, "\n")] = 0;

        // Allow the player to type "exit" to quit immediately
        if (strcmp(send_buffer, "exit") == 0) { 
            printf("Exiting game...\n");
            close(sock);
            exit(0);
        }

        // Send to server
        if (strlen(send_buffer) > 0) {
            send(sock, send_buffer, strlen(send_buffer), 0);
        }
        
        // Simple exit command for the client side
        if (strcmp(send_buffer, "exit") == 0) {
            break;
        }
    }

    close(sock);
    return 0;
}