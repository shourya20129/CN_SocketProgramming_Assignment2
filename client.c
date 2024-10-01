#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    // Check if the correct number of arguments is passed
    if (argc != 3) {
        printf("Usage: %s <server_ip> <num_connections>\n", argv[0]);
        return 1;
    }

    // Extract server IP and number of connections from command-line arguments
    char *server_ip = argv[1];
    int num_connections = atoi(argv[2]);

    // Loop for the specified number of connections
    for (int i = 0; i < num_connections; i++) {
        // Create a socket for each connection
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("Could not create socket");
            return 1;
        }

        // Set up the server address structure
        server.sin_family = AF_INET;
        server.sin_port = htons(PORT);
        if (inet_pton(AF_INET, server_ip, &server.sin_addr) <= 0) {
            perror("Invalid server address");
            close(sock);
            return 1;
        }

        // Attempt to connect to the server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            perror("Connection failed");
            close(sock);
            return 1;
        }

        // Clear the buffer to ensure no leftover data
        memset(buffer, 0, sizeof(buffer));

        // Receive the response from the server
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            close(sock);
            return 1;
        }

        // Ensure the buffer is null-terminated
        buffer[bytes_received] = '\0';

        // Print the server's response
        printf("Server response (Connection %d):\n%s\n", i + 1, buffer);

        // Close the socket for this connection
        close(sock);
    }

    return 0;
}
