#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_socket;
int client_socket;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        shutdown(server_socket, SHUT_RDWR);
        shutdown(client_socket, SHUT_RDWR);
        close(server_socket);
        close(client_socket);
        remove(FILE_PATH);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    // Open the syslog
    openlog("aesdsocket", 0, LOG_USER);

    // Handle signals
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Create a server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        syslog(LOG_ERR, "Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Reuse a server address after closing the server socket
    int reuseaddr = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    // Bind the server socket to the port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        syslog(LOG_ERR, "Error listening for connections");
        exit(EXIT_FAILURE);
    }

    // Become a daemon
    if (argc == 2 && !strcmp("-d", argv[1])) {
        daemon(0, 0);
    }

    while (1) {
        // Wait for and accept a connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            syslog(LOG_ERR, "Error accepting connection");
            exit(EXIT_FAILURE);
        }

        // Log the connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Append to the file
        int fd = open(FILE_PATH, O_RDWR | O_APPEND | O_CREAT, 0644);
        if (fd < 0) {
            syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
            break;
        }

        // Receive data and append to file
        char buffer[BUFFER_SIZE];
        int bytes_read;
        while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {

            // Null-terminate the buffer
            buffer[bytes_read] = '\0';
            write(fd, buffer, bytes_read);
            syslog(LOG_INFO, "write: %s\n", buffer);

            // End of packet
            if (buffer[bytes_read - 1] == '\n') {
                break;
            }
        }

        // Send the contents of the file back to the client
        lseek(fd, 0, SEEK_SET);
        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }

        // Log the disconnection
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(fd);
        close(client_socket);
    }

    return 0;
}

