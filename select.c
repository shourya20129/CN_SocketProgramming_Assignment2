#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30

typedef struct {
    char name[256];
    int pid;
    long unsigned int user_time;
    long unsigned int kernel_time;
} ProcessInfo;

// Function to get the top two CPU-consuming processes
void get_top_two_processes(ProcessInfo processes[2]) {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    long unsigned int total_time = 0;
    ProcessInfo temp;
    ProcessInfo top_two[2] = {{0}};
    
    // Loop through potential PIDs in /proc directory
    for (int i = 0; i < 32768; i++) {
        snprintf(buffer, sizeof(buffer), "/proc/%d/stat", i);
        int fd = open(buffer, O_RDONLY);
        if (fd == -1) continue; // Skip if open fails
        read(fd, buffer, sizeof(buffer));
        
        // Parse the process information
        sscanf(buffer, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
               &temp.pid, temp.name, &temp.user_time, &temp.kernel_time);
        total_time = temp.user_time + temp.kernel_time;

        // Update top two processes based on CPU time
        if (total_time > top_two[0].user_time + top_two[0].kernel_time) {
            top_two[1] = top_two[0];
            top_two[0] = temp;
        } else if (total_time > top_two[1].user_time + top_two[1].kernel_time) {
            top_two[1] = temp;
        }

        close(fd);
    }

    // Store the results in the provided array
    processes[0] = top_two[0];
    processes[1] = top_two[1];
}

int main() {
    int server_socket, new_socket, client_socket[MAX_CLIENTS], activity, max_sd, sd;
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds; // Set of socket descriptors
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Initialize all client_socket[] to 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

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

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        // Wait for activity on any socket
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If something happened on the master socket, it's an incoming connection
        if (FD_ISSET(server_socket, &readfds)) {
            new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
            if (new_socket < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            printf("New connection from %s on port %d\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Add new socket to array of clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets at index %d\n", i);
                    break;
                }
            }
        }

        // Process IO on other sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // Connection closed
                    getpeername(sd, (struct sockaddr *)&client_addr, &addr_len);
                    printf("Client disconnected, IP %s, Port %d\n", 
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    buffer[valread] = '\0'; // Null-terminate the received string

                    // Send CPU process info
                    ProcessInfo top_two[2];
                    get_top_two_processes(top_two);
                    snprintf(buffer, sizeof(buffer),
                             "Top Process 1: Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n"
                             "Top Process 2: Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n",
                             top_two[0].name, top_two[0].pid, top_two[0].user_time, top_two[0].kernel_time,
                             top_two[1].name, top_two[1].pid, top_two[1].user_time, top_two[1].kernel_time);

                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
