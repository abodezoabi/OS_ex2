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

#define MAX_ARGS 10
#define BUFSIZE 1024

void print_error_and_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void parse_host_port(char *host_port, char **host, int *port) {
    char *comma = strchr(host_port, ',');
    if (comma == NULL) {
        fprintf(stderr, "Invalid host:port format\n");
        exit(EXIT_FAILURE);
    }

    *comma = '\0';
    *host = host_port;
    *port = atoi(comma + 1);
}

int create_tcp_server(int port) {
    int server_fd, optval = 1;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        print_error_and_exit("socket");

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
        print_error_and_exit("setsockopt");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        print_error_and_exit("bind");

    if (listen(server_fd, 5) < 0)
        print_error_and_exit("listen");

    return server_fd;
}

int create_tcp_client(char *host, int port) {
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
        print_error_and_exit("socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        print_error_and_exit("inet_pton");
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        print_error_and_exit("connect");

    return client_fd;
}

void handle_io(int input_fd, int output_fd) {
    char buffer[BUFSIZE];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(input_fd, buffer, BUFSIZE)) > 0) {
        bytes_written = write(output_fd, buffer, bytes_read);
        if (bytes_written < 0) {
            print_error_and_exit("write");
        }
        // Ensure that the output is flushed immediately
        fsync(output_fd);
    }
    if (bytes_read < 0) {
        print_error_and_exit("read");
    }
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
            print_error_and_exit("poll");
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

void handle_tcp_to_tcp(int tcp_server_fd, int tcp_client_fd) {
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    char buffer[BUFSIZE];

    // Accept the incoming connection from the TCP client
    int new_socket_fd = accept(tcp_server_fd, (struct sockaddr *)&cliaddr, &len);
    if (new_socket_fd < 0) {
        print_error_and_exit("accept");
    }

    handle_chat(new_socket_fd, tcp_client_fd);

    close(new_socket_fd);
}

int main(int argc, char *argv[]) {
    char *command = NULL;
    char *input_option = NULL;
    char *output_option = NULL;
    char *b_option = NULL;
    char *to_option = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "i:o:b:e:t:")) != -1) {
        switch (opt) {
            case 'i':
                input_option = optarg;
                break;
            case 'o':
                output_option = optarg;
                break;
            case 'b':
                b_option = optarg;
                break;
            case 'e':
                command = optarg;
                break;
            case 't':
                to_option = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-e <command>] [-i <input_option>] [-o <output_option>] [-b <b_option>] [-t <tcp_to_tcp_option>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!command && optind < argc) {
        command = argv[optind];
    }

    int server_fd = -1, client_fd = -1;
    int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;

    if (input_option != NULL) {
        // Handle input redirection
        if (strncmp(input_option, "TCPS", 4) == 0) {
            int port = atoi(input_option + 4);
            server_fd = create_tcp_server(port);
            input_fd = accept(server_fd, NULL, NULL);
            if (input_fd < 0)
                print_error_and_exit("accept");
        } else if (strncmp(input_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(input_option + 4, &host, &port);
            client_fd = create_tcp_client(host, port);
            input_fd = client_fd;
        }
    }

    if (output_option != NULL) {
        // Handle output redirection
        if (strncmp(output_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(output_option + 4, &host, &port);
            client_fd = create_tcp_client(host, port);
            output_fd = client_fd;
        }
    }

    if (b_option != NULL) {
        // Handle bidirectional communication
        if (strncmp(b_option, "TCPS", 4) == 0) {
            int port = atoi(b_option + 4);
            server_fd = create_tcp_server(port);
            input_fd = accept(server_fd, NULL, NULL);
            if (input_fd < 0)
                print_error_and_exit("accept");
            output_fd = input_fd;
        } else if (strncmp(b_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(b_option + 4, &host, &port);
            client_fd = create_tcp_client(host, port);
            input_fd = client_fd;
            output_fd = client_fd;
        }
    }

    if (to_option != NULL) {
        // Handle TCP to TCP redirection
        int tcp_server_port = atoi(to_option);
        server_fd = create_tcp_server(tcp_server_port);
        if (output_option != NULL && strncmp(output_option, "TCPC", 4) == 0) {
            char *host;
            int port;
            parse_host_port(output_option + 4, &host, &port);
            client_fd = create_tcp_client(host, port);
            handle_tcp_to_tcp(server_fd, client_fd);
        } else {
            fprintf(stderr, "Invalid or missing output option for TCP to TCP redirection\n");
            exit(EXIT_FAILURE);
        }
        close(server_fd);
        close(client_fd);
        return 0;
    }

    if (command != NULL) {
        // Fork a child process to execute the command
        pid_t pid = fork();
        if (pid == -1) {
            print_error_and_exit("fork");
        } else if (pid == 0) {
            // Child process

            // Redirect input if necessary
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Redirect output if necessary
            if (output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            // Execute the command
            execl("/bin/sh", "sh", "-c", command, (char *)NULL);

            // If execl returns, it must have failed
            perror("execl");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            if (server_fd != -1)
                close(server_fd);
            if (client_fd != -1)
                close(client_fd);

            // Wait for the child to terminate
            waitpid(pid, NULL, 0);
        }
    } else {
        // No command provided, act as a chat application
        handle_chat(input_fd, output_fd);
    }

    // Close the server or client socket if necessary
    if (server_fd != -1)
        close(server_fd);
    if (client_fd != -1)
        close(client_fd);

    return 0;
}
