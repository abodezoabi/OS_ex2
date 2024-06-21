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

void print_error_and_exit(const char *msg) {
    perror(msg);  // Print error message
    exit(EXIT_FAILURE);  // Exit program with failure status
}

void parse_host_port(char *host_port, char **host, int *port) {
    char *comma = strchr(host_port, ',');
    if (comma == NULL) {
        fprintf(stderr, "Invalid host:port format\n");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    *comma = '\0';  // Null-terminate the host string
    *host = host_port;  // Set host pointer to beginning of host string
    *port = atoi(comma + 1);  // Parse port from string
}

int create_udp_server(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_error_and_exit("socket creation failed");  // Print error message and exit
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        print_error_and_exit("bind failed");  // Print error message and exit
    }

    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    return sockfd;
}

int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error_and_exit("socket creation failed");  // Print error message and exit
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        print_error_and_exit("bind failed");  // Print error message and exit
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        print_error_and_exit("listen failed");  // Print error message and exit
    }

    return sockfd;
}

int create_tcp_client(char *host, int port) {
    int sockfd;
    struct sockaddr_in servaddr;
    struct hostent *server;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error_and_exit("socket creation failed");  // Print error message and exit
    }

    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy(&servaddr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    servaddr.sin_port = htons(port);

    // Connect the socket to the server address
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        print_error_and_exit("connect failed");  // Print error message and exit
    }

    return sockfd;
}

int create_udp_client(char *host, int port) {
    int client_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0)
        print_error_and_exit("socket");  // Print error message and exit

    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    sendto(client_fd, "Ready2!\n", strlen("Ready2!"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        print_error_and_exit("connect");  // Print error message and exit

    return client_fd;
}

void wait_for_udp_client(int server_fd, struct sockaddr_in *client_addr, socklen_t *client_len) {
    char buffer[1];
    recvfrom(server_fd, buffer, 1, MSG_PEEK, (struct sockaddr *)client_addr, client_len);
    // Send a starter message to the client
    recvfrom(server_socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)client_addr, client_len);
}

void handle_chat(int input_fd, int output_fd) {
    struct pollfd fds[2];
    char buffer[BUFSIZE];
    int nfds = 2;

    fds[0].fd = input_fd;
    fds[0].events = POLLIN;

    fds[1].fd = output_fd;
    fds[1].events = POLLIN;

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            print_error_and_exit("poll");  // Print error message and exit
        }

        if (fds[0].revents & POLLIN) {
            ssize_t bytes_read = read(input_fd, buffer, BUFSIZE);
            if (bytes_read <= 0) {
                break;
            }
            write(output_fd, buffer, bytes_read);
            fsync(output_fd);
        }

        if (fds[1].revents & POLLIN) {
            ssize_t bytes_read = read(output_fd, buffer, BUFSIZE);
            if (bytes_read <= 0) {
                break;
            }
            write(input_fd, buffer, bytes_read);
            fsync(input_fd);
        }
    }
}

void handle_udp_to_tcp(int udp_fd, int tcp_fd) {
    char buffer[BUFSIZE];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    while (1) {
        ssize_t n = recvfrom(udp_fd, buffer, BUFSIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            close(client_socket_fd);
            close(tcp_fd);
            close(udp_fd);

            EXIT_SUCCESS;
        }

        ssize_t bytes_written = write(tcp_fd, buffer, n);
        if (bytes_written < 0) {
            close(client_socket_fd);
            close(tcp_fd);
            close(udp_fd);
            EXIT_SUCCESS;  // exit
        }
        fsync(tcp_fd);
    }
}
// Signal handler for alarm signal
void alarm_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning

    // Close server socket if it's open
    if (server_socket_fd != -1) {
        close(server_socket_fd);
    }

    // Close client socket if it's open
    if (client_socket_fd != -1) {
        close(client_socket_fd);
    }

    exit(EXIT_SUCCESS);  // Exit the current process
}

void create_and_handle_unix_dgram_server(const char *path, const char *command) {
    int sockfd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[256]; // Increase buffer size to accommodate larger initial messages

    // Create a Unix domain socket
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("socket");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    // Set up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the address
    unlink(server_addr.sun_path); // Remove any existing file at the address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    // Wait for a message from the client
    client_len = sizeof(client_addr);
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
    if (n < 0) {
        perror("recvfrom");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    }

    // Fork a child process to execute the command
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    } else if (pid == 0) {
        // Child process
        // Redirect stdin to the socket for receiving client input
        if (dup2(sockfd, STDIN_FILENO) < 0) {
            perror("dup2");  // Print error message
            exit(EXIT_FAILURE);  // Exit program with failure status
        }
        // Execute the command
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        perror("execl");  // Print error message
        exit(EXIT_FAILURE);  // Exit program with failure status
    } else {
        // Parent process
        close(sockfd);  // Close the socket in the parent process

        // Wait for the child process to terminate
        waitpid(pid, NULL, 0);
    }

}

int create_unix_dgram_client(const char *path) {

    int client_fd; // File descriptor for the client socket
    struct sockaddr_un client_addr; // Struct for Unix domain socket address

    client_fd = socket(AF_UNIX, SOCK_DGRAM, 0); // Create a Unix domain datagram socket
    if (client_fd < 0) { // Check for socket creation failure
        perror("socket"); // Print error message
        exit(EXIT_FAILURE); // Exit the program on failure
    }

    memset(&client_addr, 0, sizeof(client_addr)); // Zero out the address struct
    client_addr.sun_family = AF_UNIX; // Set the address family to AF_UNIX
    strncpy(client_addr.sun_path, path, sizeof(client_addr.sun_path) - 1); // Copy the path to the address struct

    printf("Client connecting to %s\n", client_addr.sun_path); // Print the path being connected to
    if (connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) { // Connect to the server
        perror("connect"); // Print error message on failure
        exit(EXIT_FAILURE); // Exit the program on failure
    }

    return client_fd; // Return the client socket file descriptor
}

int create_unix_stream_server(const char *path) {
    int server_fd; // File descriptor for the server socket
    struct sockaddr_un server_addr; // Struct for Unix domain socket address

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create a Unix domain stream socket
    if (server_fd < 0) { // Check for socket creation failure
        print_error_and_exit("socket"); // Print error message and exit
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Zero out the address struct
    server_addr.sun_family = AF_UNIX; // Set the address family to AF_UNIX
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1); // Copy the path to the address struct

    unlink(path); // Remove the file if it already exists

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // Bind the socket to the address
        print_error_and_exit("bind"); // Print error message and exit on failure
    }

    if (listen(server_fd, 5) < 0) { // Listen for incoming connections
        print_error_and_exit("listen"); // Print error message and exit on failure
    }

    return server_fd; // Return the server socket file descriptor
}

int create_unix_stream_client(const char *path) {
    int client_fd; // File descriptor for the client socket
    struct sockaddr_un client_addr; // Struct for Unix domain socket address

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create a Unix domain stream socket
    if (client_fd < 0) { // Check for socket creation failure
        print_error_and_exit("socket"); // Print error message and exit
    }

    memset(&client_addr, 0, sizeof(client_addr)); // Zero out the address struct
    client_addr.sun_family = AF_UNIX; // Set the address family to AF_UNIX
    strncpy(client_addr.sun_path, path, sizeof(client_addr.sun_path) - 1); // Copy the path to the address struct

    if (connect(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) { // Connect to the server
        print_error_and_exit("connect"); // Print error message and exit on failure
    }

    return client_fd; // Return the client socket file descriptor
}

void handle_bidirectional_communication(int port, bool is_server, char *command, bool use_unix, char *unix_path) {
    int fd; // File descriptor for communication

    if (use_unix) { // Check if using Unix domain sockets
        if (is_server) {
            // Create Unix Domain Stream server
            int server_fd = create_unix_stream_server(unix_path); // Create Unix stream server
            if (server_fd < 0) {
                print_error_and_exit("create_unix_stream_server"); // Print error message and exit on failure
            }

            // Accept client connection
            fd = accept(server_fd, NULL, NULL); // Accept connection from client
            if (fd < 0) {
                print_error_and_exit("accept"); // Print error message and exit on failure
            }

            close(server_fd); // Close the listening socket
        } else {
            // Create Unix Domain Stream client and connect to the server
            fd = create_unix_stream_client(unix_path); // Create Unix stream client
            if (fd < 0) {
                print_error_and_exit("create_unix_stream_client"); // Print error message and exit on failure
            }
        }
    } else {
        if (is_server) {
            // Create TCP server
            int server_fd = create_tcp_server(port); // Create TCP server
            if (server_fd < 0) {
                print_error_and_exit("create_tcp_server"); // Print error message and exit on failure
            }

            // Accept client connection
            fd = accept(server_fd, NULL, NULL); // Accept connection from client
            if (fd < 0) {
                print_error_and_exit("accept"); // Print error message and exit on failure
            }

            close(server_fd); // Close the listening socket
        } else {
            // Create TCP client and connect to the server
            fd = create_tcp_client("127.0.0.1", port); // Create TCP client, assume server is on localhost
            if (fd < 0) {
                print_error_and_exit("create_tcp_client"); // Print error message and exit on failure
            }
        }
    }

    // Fork a process to run the command
    pid_t pid = fork(); // Fork the process
    if (pid == -1) {
        print_error_and_exit("fork"); // Print error message and exit on failure
    } else if (pid == 0) {
        // Child process

        // Redirect input and output to the socket
        dup2(fd, STDIN_FILENO); // Redirect stdin to the socket
        dup2(fd, STDOUT_FILENO); // Redirect stdout to the socket
        close(fd); // Close the socket file descriptor

        // Execute the command
        execl("/bin/sh", "sh", "-c", command, (char *)NULL); // Execute the command

        // If execl returns, it must have failed
        perror("execl"); // Print error message
        exit(EXIT_FAILURE); // Exit the child process
    } else {
        // Parent process
        close(fd); // Close the socket file descriptor

        // Wait for the child process to terminate
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }
    close(fd); // Close the socket file descriptor
}

int main(int argc, char *argv[]) {
    char *command = NULL; // Pointer to store the command
    char *input_option = NULL; // Pointer to store the input option
    char *output_option = NULL; // Pointer to store the output option
    char *b_option = NULL; // Pointer to store the bidirectional communication option
    char *to_option = NULL; // Pointer to store the TCP to TCP redirection option
    int timeout = 0; // Variable to store the timeout value

    int opt; // Variable to store the command-line option
    while ((opt = getopt(argc, argv, "i:o:b:e:t:")) != -1) { // Parse command-line options
        switch (opt) {
            case 'i':
                input_option = optarg; // Set input option
                break;
            case 'o':
                output_option = optarg; // Set output option
                break;
            case 'b':
                b_option = optarg; // Set bidirectional communication option
                break;
            case 'e':
                command = optarg; // Set command to execute
                break;
            case 't':
                timeout = atoi(optarg); // Set timeout value
                break;
            default:
                fprintf(stderr, "Usage: %s [-e <command>] [-i <input_option>] [-o <output_option>] [-b <b_option>] [-t <tcp_to_tcp_option>]\n", argv[0]); // Print usage message
                exit(EXIT_FAILURE); // Exit the program on invalid option
        }
    }

    if (!command && optind < argc) {
        command = argv[optind]; // Set command if provided as positional argument
    }

    int server_fd = -1, client_fd = -1; // File descriptors for server and client
    int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO; // File descriptors for input and output

    if (timeout > 0) {
        signal(SIGALRM, alarm_handler); // Set up signal handler for alarm
        alarm(timeout); // Set alarm with the specified timeout
    }
    if (input_option != NULL && output_option != NULL &&
        strncmp(input_option, "UDPS", 4) == 0 && strncmp(output_option, "TCPC", 4) == 0) {
        // Special case for UDP server to TCP client forwarding with command execution
        int udp_port = atoi(input_option + 4); // Parse UDP port from input option
        struct sockaddr_in client_addr; // Struct for client address
        socklen_t client_len = sizeof(client_addr); // Length of client address

        server_fd = create_udp_server(udp_port); // Create UDP server
        if (server_fd < 0) {
            fprintf(stderr, "bind failed: Address already in use\n"); // Print error message
            exit(EXIT_FAILURE); // Exit on failure
        }
        wait_for_udp_client(server_fd, &client_addr, &client_len); // Wait for UDP client to connect

        char *host;
        int tcp_port;
        parse_host_port(output_option + 4, &host, &tcp_port); // Parse host and port for TCP client
        client_fd = create_tcp_client(host, tcp_port); // Create TCP client

        if (command != NULL) {
            // Fork a child process to execute the command
            pid_t pid = fork(); // Fork the process
            if (pid == -1) {
                print_error_and_exit("fork"); // Print error message and exit on failure
            } else if (pid == 0) {
                // Child process

                // Redirect input if necessary
                if (server_fd != -1) {
                    dup2(server_fd, STDIN_FILENO); // Redirect stdin to server socket
                    close(server_fd); // Close server socket file descriptor
                }

                // Redirect output if necessary
                if (client_fd != -1) {
                    dup2(client_fd, STDOUT_FILENO); // Redirect stdout to client socket
                    close(client_fd); // Close client socket file descriptor
                }

                // Execute the command
                execl("/bin/sh", "sh", "-c", command, (char *)NULL); // Execute the command

                // If execl returns, it must have failed
                perror("execl"); // Print error message
                exit(EXIT_FAILURE); // Exit the child process
            } else {
                // Parent process
                if (server_fd != -1)
                    close(server_fd); // Close server socket file descriptor
                if (client_fd != -1)
                    close(client_fd); // Close client socket file descriptor

                // Wait for the child to terminate
                waitpid(pid, NULL, 0); // Wait for the child process to finish
            }
        }

        // Handle UDP to TCP forwarding in the parent process
        handle_udp_to_tcp(server_fd, client_fd); // Forward data from UDP to TCP
    }

    if (input_option != NULL) {
        // Handle input redirection
        if (strncmp(input_option, "TCPS", 4) == 0) {
            int port = atoi(input_option + 4); // Parse port number from input option
            server_fd = create_tcp_server(port); // Create TCP server
            input_fd = accept(server_fd, NULL, NULL); // Accept client connection
            if (input_fd < 0)
                print_error_and_exit("accept"); // Print error message and exit on failure
        } else if (strncmp(input_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(input_option + 4, &host, &port); // Parse host and port for TCP client
            client_fd = create_tcp_client(host, port); // Create TCP client
            input_fd = client_fd; // Set input file descriptor to client socket
        } else if (strncmp(input_option, "UDPS", 4) == 0) {
            int port = atoi(input_option + 4); // Parse port number from input option
            struct sockaddr_in client_addr; // Struct for client address
            socklen_t client_len = sizeof(client_addr); // Length of client address

            server_fd = create_udp_server(port); // Create UDP server
            wait_for_udp_client(server_fd, &client_addr, &client_len); // Wait for UDP client to connect
            input_fd = server_fd; // Set input file descriptor to server socket
        } else if (strncmp(input_option, "UDSSD", 5) == 0) {
            create_and_handle_unix_dgram_server(input_option + 5, command); // Create and handle Unix datagram server
            exit(EXIT_SUCCESS); // Exit after handling datagrams
        } else if (strncmp(input_option, "UDSSS", 5) == 0) {
            input_fd = create_unix_stream_server(input_option + 5); // Create Unix stream server
            input_fd = accept(input_fd, NULL, NULL); // Accept connection for stream server
            if (input_fd < 0)
                print_error_and_exit("accept"); // Print error message and exit on failure
        } else if (strncmp(input_option, "UDSCS", 5) == 0) {
            input_fd = create_unix_stream_client(input_option + 5); // Create Unix stream client
        }
    }

    if (output_option != NULL) {
        // Handle output redirection
        if (strncmp(output_option, "TCPS", 4) == 0) {
            int port;
            sscanf(output_option + 4, "%d", &port); // Parse the port number
            output_fd = create_tcp_server(port); // Create TCP server
            output_fd = accept(output_fd, NULL, NULL); // Accept connection for TCP server
            input_fd = output_fd; // Set input file descriptor to output file descriptor
            if (output_fd < 0)
                print_error_and_exit("accept"); // Print error message and exit on failure
        }
        if (strncmp(output_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(output_option + 4, &host, &port); // Parse host and port for TCP client
            client_fd = create_tcp_client(host, port); // Create TCP client
            output_fd = client_fd; // Set output file descriptor to client socket
        } else if (strncmp(output_option, "UDPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(output_option + 4, &host, &port); // Parse host and port for UDP client
            client_fd = create_udp_client(host, port); // Create UDP client
            output_fd = client_fd; // Set output file descriptor to client socket
        } else if (strncmp(output_option, "UDSCD", 5) == 0) {
            output_fd = create_unix_dgram_client(output_option + 5); // Create Unix datagram client
        } else if (strncmp(output_option, "UDSCS", 5) == 0) {
            output_fd = create_unix_stream_client(output_option + 5); // Create Unix stream client
        } else if (strncmp(output_option, "UDSSS", 5) == 0) {
            output_fd = create_unix_stream_server(output_option + 5); // Create Unix stream server
            output_fd = accept(output_fd, NULL, NULL); // Accept connection for stream server
            if (output_fd < 0)
                print_error_and_exit("accept"); // Print error message and exit on failure
        }
    }

    if (b_option != NULL) {
        if (strncmp(b_option, "TCPS", 4) == 0) {
            int fd;
            int port;
            sscanf(b_option + 4, "%d", &port); // Parse port number from bidirectional option
            handle_bidirectional_communication(port, true, command, false,
                                               NULL); // Handle bidirectional communication as TCP server
                                               return 0;
        }else if (strncmp(b_option, "TCPC", 4) == 0) {
            int port;
            sscanf(b_option + 14, "%d", &port); // Parse port number from bidirectional option
            handle_bidirectional_communication(port, false, command, false, NULL); // Handle bidirectional communication as TCP client
            return 0;
        } else if (strncmp(b_option, "UDSSS", 5) == 0) {
            handle_bidirectional_communication(0, true, command, true, b_option + 5); // Handle bidirectional communication as Unix stream server
            return 0;
        } else if (strncmp(b_option, "UDSCS", 5) == 0) {
            handle_bidirectional_communication(0, false, command, true, b_option + 5); // Handle bidirectional communication as Unix stream client
            return 0;
        }
    }

    if (command != NULL) {
        // Fork a child process to execute the command
        pid_t pid = fork(); // Fork the process
        if (pid == -1) {
            print_error_and_exit("fork"); // Print error message and exit on failure
        } else if (pid == 0) {
            // Child process

            // Redirect input if necessary
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO); // Redirect stdin to input file descriptor
                close(input_fd); // Close input file descriptor
            }

            // Redirect output if necessary
            if (output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO); // Redirect stdout to output file descriptor
                close(output_fd); // Close output file descriptor
            }

            // Execute the command
            execl("/bin/sh", "sh", "-c", command, (char *)NULL); // Execute the command

            // If execl returns, it must have failed
            perror("execl"); // Print error message
            exit(EXIT_FAILURE); // Exit the child process
        } else {
            // Parent process
            if (server_fd != -1)
                close(server_fd); // Close server socket file descriptor
            if (client_fd != -1)
                close(client_fd); // Close client socket file descriptor

            // Wait for the child to terminate
            waitpid(pid, NULL, 0); // Wait for the child process to finish
        }
    } else {
        // No command provided, act as a chat application
        handle_chat(input_fd, output_fd); // Handle chat functionality
    }

    // Close the server or client socket if necessary
    if (server_fd != -1)
        close(server_fd); // Close server socket file descriptor
    if (client_fd != -1)
        close(client_fd); // Close client socket file descriptor

    return 0; // Return 0 to indicate success
}
