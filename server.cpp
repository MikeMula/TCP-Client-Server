//////////////////////////////////
//
// Multi-process TCP Server
// Author: Mike KK
//
/////////////////////////////////

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <string.h>
#include <vector>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;

#define BUFFER_LEN 1024

// keep track of open socket file descriptors
std::vector<int> open_fds;
char BUFFER[BUFFER_LEN];

void clientHandler(int);

void terminationHandler(int n)
{
    cout << "\nCleaning up..." << endl;

    for (int i = 0; i < open_fds.size(); i++)
    {
        close(open_fds[i]);
    }

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // Register signal to handle process terminationHandler
    signal(SIGINT, terminationHandler);

    sockaddr_in tcp_server_addr = {0};
    socklen_t tcp_server_addr_len = sizeof(tcp_server_addr);

    sockaddr_in tcp_client_addr = {0};
    socklen_t tcp_client_addr_len = sizeof(tcp_client_addr);

    int tcp_server_fd, tcp_client_fd;
    unsigned short int server_port;

    // Create a TCP socket to listen to incoming connections
    tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_fd < 0)
    {
        perror("Failed to create a TCP socket.");
        exit(EXIT_FAILURE);
    }

    open_fds.push_back(tcp_server_fd);

    // Fill in the address
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = 0; // let OS choose port number
    tcp_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to a PORT
    if (bind(tcp_server_fd, (sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0)
    {
        perror("Unable to bind TCP socket.");
        terminationHandler(EXIT_FAILURE);
    }

    // Listen for connection requests on selected PORT
    if (listen(tcp_server_fd, 5) < 0)
    {
        perror("Error in listen.");
        terminationHandler(EXIT_FAILURE);
    }

    // Get port assigned by OS and publish it
    getsockname(tcp_server_fd, (sockaddr *)&tcp_server_addr, &tcp_server_addr_len);
    server_port = ntohs(tcp_server_addr.sin_port);

    cout << "\n[Server] Port: " << server_port << endl;

    // Create server to handle incoming connections
    while (1)
    {
        // Accept new incoming client connection
        tcp_client_fd = accept(tcp_server_fd, (sockaddr *)&tcp_client_addr, &tcp_client_addr_len);
        if (tcp_client_fd < 0)
        {
            perror("Error on accepting");
            terminationHandler(EXIT_FAILURE);
        }

        open_fds.push_back(tcp_client_fd);

        // Create a child process to handle each new client
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork() failed.");
            terminationHandler(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // child process to handle this connection
            cout << "[Server] Handling new connection...\n"
                 << endl;
            clientHandler(tcp_client_fd);
        }
    }

    return 0;
}

void clientHandler(int tcp_client_fd)
{
    int bytes_sent;

    // Send welcome to connected client
    const char *welcome_message = "\nHello!  Welcome from the server!\n";
    const char *response_message = "[Server] Response: Got your message!\n";
    bytes_sent = send(tcp_client_fd, welcome_message, strlen(welcome_message), 0);
    if (bytes_sent < 0)
    {
        perror("There was an error sending a message.");
        close(tcp_client_fd);
        exit(EXIT_FAILURE);
    }

    while (read(tcp_client_fd, BUFFER, BUFFER_LEN))
    {
        cout << "[Server] Received: " << BUFFER << endl;

        bytes_sent = send(tcp_client_fd, response_message, strlen(response_message), 0);
        if (bytes_sent < 0)
        {
            perror("There was an error sending a message.");
            close(tcp_client_fd);
            exit(EXIT_FAILURE);
        }
    }

    cout << "[Server] Client exiting now...\n"
         << endl;
    close(tcp_client_fd);
}