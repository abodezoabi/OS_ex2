#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <getopt.h>
#include <signal.h>
#include <netdb.h>
#include <sys/un.h>
#include <stdbool.h>
#define MAX_ARGS 10
#define BUFSIZE 1024

// Global variables to hold server and client socket file descriptors
int server_socket_fd = -1;
int client_socket_fd = -1;

// Function to print an error message and exit
void print_error_and_exit(const char *msg) {
    perror(msg);                // Print the error message
    exit(EXIT_FAILURE);         // Exit the program with failure status
}

// Function to parse host and port from a string
void parse_host_port(char *host_port, char **host, int *port) {
    char *comma = strchr(host_port, ',');    // Find the comma in the host:port string
    if (comma == NULL) {                     // If no comma is found, print an error and exit
        fprintf(stderr, "Invalid host:port format\n");
        exit(EXIT_FAILURE);
    }

    *comma = '\0';              // Replace the comma with a null terminator
    *host = host_port;          // Set the host part
    *port = atoi(comma + 1);    // Convert the port part to an integer
}

// Function to create a TCP server
int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error_and_exit("socket creation failed");
    }

    memset(&servaddr, 0, sizeof(servaddr));  // Zero out the server address structure

    // Filling server information
    servaddr.sin_family = AF_INET;           // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;   // Accept connections from any IP address
    servaddr.sin_port = htons(port);         // Convert port to network byte order

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        print_error_and_exit("bind failed");
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        print_error_and_exit("listen failed");
    }

    return sockfd;           // Return the server socket file descriptor
}

// Function to create a TCP client
int create_tcp_client(char *host, int port) {
    int sockfd;
    struct sockaddr_in servaddr;
    struct hostent *server;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error_and_exit("socket creation failed");
    }

    server = gethostbyname(host);   // Get the server address
    if (server == NULL) {           // If no such host, print an error and exit
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));  // Zero out the server address structure
    servaddr.sin_family = AF_INET;           // IPv4
    memcpy(&servaddr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);  // Copy server address
    servaddr.sin_port = htons(port);         // Convert port to network byte order

    // Connect the socket to the server address
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        print_error_and_exit("connect failed");
    }

    return sockfd;           // Return the client socket file descriptor
}

// Function to handle bidirectional communication
void handle_bidirectional_communication(int port, bool is_server, char *command, bool use_unix, char *unix_path) {
    int fd;

    if (is_server) {
        // Create TCP server
        int server_fd = create_tcp_server(port);      // Create TCP server socket
        if (server_fd < 0) {
            print_error_and_exit("create_tcp_server");
        }

        // Accept client connection
        fd = accept(server_fd, NULL, NULL);           // Accept a connection from a client
        if (fd < 0) {
            print_error_and_exit("accept");
        }

        close(server_fd); // Close the listening socket
    } else {
        // Create TCP client and connect to the server
        fd = create_tcp_client("127.0.0.1", port);    // Create TCP client and connect to the server
        if (fd < 0) {
            print_error_and_exit("create_tcp_client");
        }
    }

    // Fork a process to run the command
    pid_t pid = fork();                               // Create a new process
    if (pid == -1) {
        print_error_and_exit("fork");
    } else if (pid == 0) {
        // Child process

        // Redirect input and output to the socket
        dup2(fd, STDIN_FILENO);                       // Redirect standard input to the socket
        dup2(fd, STDOUT_FILENO);                      // Redirect standard output to the socket
        close(fd);                                    // Close the socket file descriptor

        // Execute the command
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);  // Execute the command using the shell

        // If execl returns, it must have failed
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(fd);                                    // Close the socket file descriptor

        // Wait for the child process to terminate
        waitpid(pid, NULL, 0);                        // Wait for the child process to finish
    }
    close(fd);  // Close the socket file descriptor
}

int main(int argc, char *argv[]) {
    char *command = NULL;            // Command to execute
    char *input_option = NULL;       // Input option string
    char *output_option = NULL;      // Output option string
    char *b_option = NULL;           // Bidirectional communication option
    char *to_option = NULL;          // Timeout option (unused)
    int timeout = 0;                 // Timeout value (unused)

    int opt;
    while ((opt = getopt(argc, argv, "i:o:b:e:t:")) != -1) {  // Parse command-line options
        switch (opt) {
            case 'i':
                input_option = optarg;       // Get input option
                break;
            case 'o':
                output_option = optarg;      // Get output option
                break;
            case 'b':
                b_option = optarg;           // Get bidirectional communication option
                break;
            case 'e':
                command = optarg;            // Get command to execute
                break;
            default:
                // Print usage message and exit if an invalid option is provided
                fprintf(stderr, "Usage: %s [-e <command>] [-i <input_option>] [-o <output_option>] [-b <b_option>] [-t <tcp_to_tcp_option>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!command && optind < argc) {
        command = argv[optind];  // Get the command from the remaining arguments
    }

    int server_fd = -1, client_fd = -1;     // Server and client socket file descriptors
    int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;  // Input and output file descriptors

    if (input_option != NULL) {
        // Handle input redirection
        if (strncmp(input_option, "TCPS", 4) == 0) {
            int port = atoi(input_option + 4);          // Parse the port number
            server_fd = create_tcp_server(port);        // Create TCP server
            input_fd = accept(server_fd, NULL, NULL);   // Accept client connection
            if (input_fd < 0)
                print_error_and_exit("accept");
        } else if (strncmp(input_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(input_option + 4, &host, &port); // Parse host and port
            client_fd = create_tcp_client(host, port);      // Create TCP client
            input_fd = client_fd;                           // Set input file descriptor to client socket
        }
    }

    if (output_option != NULL) {
        // Handle output redirection
        if (strncmp(output_option, "TCPS", 4) == 0) {
            int port;
            sscanf(output_option + 4, "%d", &port);  // Parse the port number
            output_fd = create_tcp_server(port);     // Create TCP server
            output_fd = accept(output_fd, NULL, NULL); // Accept connection for TCP server
            input_fd = output_fd;                    // Set input file descriptor to output file descriptor
            if (output_fd < 0)
                print_error_and_exit("accept");
        } else if (strncmp(output_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(output_option + 4, &host, &port); // Parse host and port
            client_fd = create_tcp_client(host, port);        // Create TCP client
            output_fd = client_fd;                            // Set output file descriptor to client socket
        }
    }

    if (b_option != NULL) {
        // Handle bidirectional communication
        if (strncmp(b_option, "TCPS", 4) == 0) {
            int port;
            sscanf(b_option + 4, "%d", &port);       // Parse the port number
            handle_bidirectional_communication(port, true, command, false, NULL); // Start bidirectional communication as server
            return 0;
        } else if (strncmp(b_option, "TCPC", 4) == 0) {
            int port;
            sscanf(b_option + 4, "%d", &port);       // Parse the port number
            handle_bidirectional_communication(port, false, command, false, NULL); // Start bidirectional communication as client
            return 0;
        }
    }

    if (command != NULL) {
        // Fork a child process to execute the command
        pid_t pid = fork();                        // Create a new process
        if (pid == -1) {
            print_error_and_exit("fork");
        } else if (pid == 0) {
            // Child process

            // Redirect input if necessary
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);      // Redirect standard input to input file descriptor
                close(input_fd);                   // Close the original input file descriptor
            }

            // Redirect output if necessary
            if (output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO);    // Redirect standard output to output file descriptor
                close(output_fd);                  // Close the original output file descriptor
            }

            // Execute the command
            execl("/bin/sh", "sh", "-c", command, (char *)NULL);  // Execute the command using the shell

            // If execl returns, it must have failed
            perror("execl");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            if (server_fd != -1)
                close(server_fd);                 // Close the server socket file descriptor
            if (client_fd != -1)
                close(client_fd);                 // Close the client socket file descriptor

            // Wait for the child to terminate
            waitpid(pid, NULL, 0);                // Wait for the child process to finish
        }
    }

    // Close the server or client socket if necessary
    if (server_fd != -1)
        close(server_fd);                         // Close the server socket file descriptor
    if (client_fd != -1)
        close(client_fd);                         // Close the client socket file descriptor

    return 0;                                     // Return success
}
