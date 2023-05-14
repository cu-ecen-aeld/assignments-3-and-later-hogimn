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
#include <pthread.h>
#include "queue.h"

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

static int server_socket;

typedef struct node_s node_t;
struct node_s {
    int socket_fd;
    pthread_t thread_id;
    SLIST_ENTRY(node_s) entries;
};

SLIST_HEAD(node_list, node_s) head;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
timer_t timer;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting %d", signal);

        // Gracefully exit each child thread
        node_t *node;
        node_t *temp_node;
        SLIST_FOREACH_SAFE(node, &head, entries, temp_node) {
            pthread_cancel(node->thread_id);
        }
        SLIST_FOREACH_SAFE(node, &head, entries, temp_node) {
            pthread_join(node->thread_id, NULL);
        }

        // Free linked list
        while (!SLIST_EMPTY(&head)) {
            node = SLIST_FIRST(&head);
            SLIST_REMOVE_HEAD(&head, entries);
            free(node);
        }

        // Free mutex
        pthread_mutex_destroy(&mutex);

        // Shutdown server
        shutdown(server_socket, SHUT_RDWR);

        // Close server socket
        close(server_socket);

        // Remove the file
        remove(FILE_PATH);

        exit(EXIT_SUCCESS);
    }
}

void *connection_handler(void *arg) {
    // Block SIGINT/SIGTERM in this child thread
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Receive argument from server and cast type
    node_t *node = (node_t *)arg;
    int client_socket = node->socket_fd;

    // Lock the mutex to prevent concurrent writes
    pthread_mutex_lock(&mutex);

    // Open the file with append mode
    int fd = open(FILE_PATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
        exit(EXIT_FAILURE);
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

    // Release the mutex to allow another thread to write
    pthread_mutex_unlock(&mutex);

    // Send the contents of the file back to the client
    lseek(fd, 0, SEEK_SET);
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Log the disconnection
    syslog(LOG_INFO, "Closed connection from %d", client_socket);
    close(fd);
    close(client_socket);
}

void *timer_handler(void *arg) {
    time_t curr_time;
    struct tm *time_info;
    char timestamp[64];
    int fd;

    while (1) {
        // Lock mutex to ensure atomicity of file writes
        pthread_mutex_lock(&mutex);

        // Get current time
        curr_time = time(NULL);
        time_info = localtime(&curr_time);

        // Format timestamp
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %T %z\n", time_info);

        // Open file for appending
        fd = open(FILE_PATH, O_RDWR | O_APPEND | O_CREAT, 0644);
        if (fd == -1) {
            syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Write timestamp to file
        write(fd, timestamp, strlen(timestamp));

        // Close file
        close(fd);

        // Unlock mutex
        pthread_mutex_unlock(&mutex);

        // Sleep 10 seconds
        sleep(10);
    }
}

int main(int argc, char *argv[]) {

    int client_socket;

    // Initialize singly-linked-list
    SLIST_INIT(&head);

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

    // Create the timer thread
    pthread_t timer_thread;
    if (pthread_create(&timer_thread, NULL, timer_handler, NULL)) {
        syslog(LOG_ERR, "Could not create thread");
        exit(EXIT_FAILURE);
    }

    // Insert thread info into the list
    node_t *node = (node_t *)malloc(sizeof(node));
    node->thread_id = timer_thread;
    SLIST_INSERT_HEAD(&head, node, entries);

    while (1) {
        // Wait for and accept a connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            syslog(LOG_ERR, "Error accepting connection");
            exit(EXIT_FAILURE);
        }

        // Log the connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Create argument to be passed to the handler
        node_t *node = (node_t *)malloc(sizeof(node));
        node->socket_fd = client_socket;

        // Create a new thread for each connection
        pthread_t handler_thread;
        if (pthread_create(&handler_thread, NULL, connection_handler, (void *)node) < 0) {
            syslog(LOG_ERR, "Could not create thread");
            exit(EXIT_FAILURE);
        }

        // Insert thread info into the list
        node->thread_id = handler_thread;
        SLIST_INSERT_HEAD(&head, node, entries);
    }

    return 0;
}

