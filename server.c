#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Structure to hold process information
typedef struct {
    char name[256];
    int pid;
    long unsigned int user_time;
    long unsigned int kernel_time;
} ProcessInfo;

// Function to get the top two CPU-consuming processes
void get_top_two_processes(ProcessInfo processes[2]) {
    DIR *proc_dir = opendir("/proc");
    struct dirent *entry;
    char filepath[256], stat_buf[BUFFER_SIZE];
    int pid, fd;
    ProcessInfo temp;
    ProcessInfo top_two[2] = {{0}};

    if (!proc_dir) {
        perror("Error opening /proc");
        return;
    }

    // Iterate through /proc to find processes
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            pid = atoi(entry->d_name);  // Process ID is the directory name
            snprintf(filepath, sizeof(filepath), "/proc/%d/stat", pid);

            // Open /proc/[pid]/stat to read process statistics
            fd = open(filepath, O_RDONLY);
            if (fd != -1) {
                read(fd, stat_buf, sizeof(stat_buf) - 1);
                stat_buf[sizeof(stat_buf) - 1] = '\0'; // Ensure null termination

                sscanf(stat_buf, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
                        &temp.pid, temp.name, &temp.user_time, &temp.kernel_time);

                long unsigned int total_time = temp.user_time + temp.kernel_time;

                // Compare and store top two CPU consumers
                if (total_time > top_two[0].user_time + top_two[0].kernel_time) {
                    top_two[1] = top_two[0];
                    top_two[0] = temp;
                } else if (total_time > top_two[1].user_time + top_two[1].kernel_time) {
                    top_two[1] = temp;
                }

                close(fd);
            }
        }
    }

    // Store the results in the provided array
    processes[0] = top_two[0];
    processes[1] = top_two[1];
    closedir(proc_dir);
}

// Thread function to handle each client connection
void *handle_client(void *client_socket) {
    int sock = *((int *)client_socket);
    free(client_socket);

    ProcessInfo top_two[2];
    get_top_two_processes(top_two);

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "Top Process 1: Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n"
             "Top Process 2: Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n",
             top_two[0].name, top_two[0].pid, top_two[0].user_time, top_two[0].kernel_time,
             top_two[1].name, top_two[1].pid, top_two[1].user_time, top_two[1].kernel_time);

    send(sock, response, strlen(response), 0);
    close(sock);

    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the IP and port
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

    // Main loop to accept and handle clients
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == -1) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected\n");

        pthread_t thread_id;
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        // Create a new thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) != 0) {
            perror("Failed to create thread");
            free(client_sock);
        }
    }

    close(server_socket);
    return 0;
}
