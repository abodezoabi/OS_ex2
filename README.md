


Networked Tic-Tac-Toe Game
Overview
This project implements a networked Tic-Tac-Toe game, allowing players to connect and play over various types of network protocols. The game supports communication via TCP, UDP, and Unix Domain Sockets, demonstrating a practical application of networking and inter-process communication in C.

Features
TCP (Transmission Control Protocol): Provides reliable, connection-oriented communication for networked gameplay.
UDP (User Datagram Protocol): Facilitates fast, connectionless communication, demonstrating handling of packet loss and order.
Unix Domain Sockets: Efficient communication for processes on the same host, available in both stream (SOCK_STREAM) and datagram (SOCK_DGRAM) modes.
Computer Science Concepts
Networking Protocols
TCP: Ensures data integrity and order through a reliable connection-oriented protocol.
UDP: Offers a faster, connectionless protocol at the expense of potential packet loss and unordered delivery.
Unix Domain Sockets: Local IPC mechanism providing efficient communication between processes on the same machine.
Inter-Process Communication (IPC)
Unix Domain Sockets: Used for local communication, demonstrating both stream and datagram communication methods.
Socket Programming
Socket Creation: Using socket() to create TCP, UDP, and Unix domain sockets.
Binding and Listening: Binding sockets to addresses and ports, and listening for incoming connections with bind() and listen().
Accepting Connections: Using accept() to handle incoming connections in TCP and Unix domain stream sockets.
Data Transmission: Sending and receiving data using send(), recv(), sendto(), and recvfrom() functions.
Concurrency and Forking
Forking: Using fork() to create child processes for handling multiple clients and running the game process in parallel.
Process Management: Managing parent and child processes to ensure proper game operation.
File Descriptor Management
Redirecting I/O: Using dup2() to redirect standard input and output to socket file descriptors, enabling networked game interaction.
Usage
TCP
TCP Server:
sh
Copy code
./mync -e "./ttt 123456789" -i TCPS<port>
TCP Client:
sh
Copy code
./mync -o TCPC<host>,<port>
UDP
UDP Server:
sh
Copy code
./mync -e "./ttt 123456789" -i UDPS<port>
UDP Client:
sh
Copy code
./mync -o UDPC<host>,<port>
Unix Domain Sockets
Unix Domain Socket Stream Server:

sh
Copy code
./mync -e "./ttt 123456789" -i UDSSS<socket_path>
Unix Domain Socket Stream Client:

sh
Copy code
./mync -o UDSCS<socket_path>
Unix Domain Socket Datagram Server:

sh
Copy code
./mync -e "./ttt 123456789" -i UDSSD<socket_path>
Unix Domain Socket Datagram Client:

sh
Copy code
./mync -o UDSCD<socket_path>
Compilation
Compile the project using a C compiler. Ensure you have gcc installed, then run:

sh
Copy code
make all
Running the Tests
Execute the test cases to verify the functionality of different network protocols:

sh
Copy code
make test
Functions Used
create_tcp_server
Creates and sets up a TCP server socket:

c
Copy code
int create_tcp_server(int port);
create_tcp_client
Creates and connects a TCP client to a server:

c
Copy code
int create_tcp_client(const char *host, int port);
create_udp_server
Creates and sets up a UDP server socket:

c
Copy code
int create_udp_server(int port);
create_udp_client
Creates a UDP client socket:

c
Copy code
int create_udp_client();
create_unix_stream_server
Creates and sets up a Unix domain socket stream server:

c
Copy code
int create_unix_stream_server(const char *path);
create_unix_stream_client
Creates and connects a Unix domain socket stream client:

c
Copy code
int create_unix_stream_client(const char *path);
create_unix_dgram_server
Creates and sets up a Unix domain socket datagram server:

c
Copy code
int create_unix_dgram_server(const char *path);
create_unix_dgram_client
Creates and connects a Unix domain socket datagram client:

c
Copy code
int create_unix_dgram_client(const char *path);
wait_for_udp_client
Waits for a UDP client message to determine the client's address:

c
Copy code
void wait_for_udp_client(int server_fd, struct sockaddr_in *client_addr, socklen_t *client_len);
handle_client
Handles client connections, used for Unix domain socket stream:

c
Copy code
void handle_client(int client_fd, const char *command);
Acknowledgments
This project demonstrates the integration of networking protocols and IPC mechanisms, providing a comprehensive example of practical socket programming and network communication in C. It is a valuable learning tool for understanding how different network protocols and IPC methods work together in real-world applications.
