# Computer Networks - Socket Programming Assignment

## Overview
This repository contains the code and report for the **Socket Programming Assignment** as part of our **Computer Networks** course.

## Contents
- `client.c`: Client-side code for establishing a TCP connection with the server.
- `server.c`: Multithreaded server-side code for handling multiple clients and providing CPU usage information.
- `single.c`: Single-threaded server implementation for comparison.
- `select.c`: Server code using the `select()` system call to handle multiple connections without threads.
- `report.docx`: Detailed report with explanations, observations, and performance analysis with screenshots.

## Instructions
1. **To compile the programs**:
   - Use the following command to compile the client and server:
     ```
     gcc client.c -o client
     gcc server.c -o server
     gcc single.c -o single_server
     gcc select.c -o select_server
     ```

2. **To run the programs**:
   - Start the server on one VM/container:
     ```
     ./server
     ```
   - Run the client on another VM/container and specify the server IP and number of concurrent connections:
     ```
     ./client <server_IP> <number_of_connections>
     ```

3. **Performance Analysis**:
   - Use the `taskset` command to pin processes to specific CPUs for performance evaluation.
   - Use `perf` to measure performance metrics like CPU clocks, context switches, and cache misses.

## Authors
- Shourya Sharma (2020129)
- Mohit Nagar (2021069)
