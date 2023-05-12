#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        syslog(LOG_ERR, "Error: Two arguments are required: writefile and writestr");
        exit(EXIT_FAILURE);
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];
    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        syslog(LOG_ERR, "Error: Could not create file %s: %s", writefile, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Error: Could not write to file %s: %s", writefile, strerror(errno));
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "Writing \"%s\" to %s", writestr, writefile);
    close(fd);
    return 0;
}

