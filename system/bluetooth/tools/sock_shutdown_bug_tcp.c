

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>

int main(int argc, char **argv) {
    int fd;
    int ret;
    long flags;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = 12348;

    fd = socket(PF_INET, SOCK_STREAM, 0);

    ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("%d errno %d %s\n", __LINE__, errno, strerror(errno));
    }
    
    ret = listen(fd, 1);
    if (ret < 0) {
        printf("%d errno %d %s\n", __LINE__, errno, strerror(errno));
    }

    sleep(2);

    close(fd);

    sleep(2);

    fd = socket(PF_INET, SOCK_STREAM, 0);

    ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("%d errno %d %s\n", __LINE__, errno, strerror(errno));
    }
    
    sleep(2000000000);

    return 0;
}

