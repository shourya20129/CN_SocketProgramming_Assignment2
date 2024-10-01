#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to handle each client connection
void handle_client(int new_socket) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "Server Response: No process information to display.\n");

    // Send the generic message back to the client
    send(new_socket, response, strlen(response), 0);
    
    // Close the client socket
    close(new_socket);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the IP and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Single-threaded server listening on port %d\n", PORT);

    // Main loop to accept and handle clients sequentially
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == -1) {
            perror("Accept failed");
            continue;
        }

        printf("New connection from %s on port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Handle client in the same thread (single-threaded)
        handle_client(new_socket);
    }

    close(server_socket);
    return 0;
}
