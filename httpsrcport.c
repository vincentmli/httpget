#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#define LOCAL_RANDOM_PORT 54321
#define LOCAL_IP_ADDRESS "10.1.72.9"

int main() {
    int ret, fd;
    struct sockaddr_in sa_dst;
    struct sockaddr_in sa_loc;
    char buffer[1024] = "GET / HTTP/1.1\r\nHost: 10.1.72.61\r\n\r\n";

    fd = socket(AF_INET, SOCK_STREAM, 0);

    // Local
    memset(&sa_loc, 0, sizeof(struct sockaddr_in));
    sa_loc.sin_family = AF_INET;
    sa_loc.sin_port = htons(LOCAL_RANDOM_PORT);
    sa_loc.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDRESS);

    ret = bind(fd, (struct sockaddr *)&sa_loc, sizeof(struct sockaddr));
    assert(ret != -1);

    // Remote
    memset(&sa_dst, 0, sizeof(struct sockaddr_in));
    sa_dst.sin_family = AF_INET;
    sa_dst.sin_port = htons(80);
    sa_dst.sin_addr.s_addr = inet_addr("10.1.72.61"); 

    ret = connect(fd, (struct sockaddr *)&sa_dst, sizeof(struct sockaddr));
    assert(ret != -1);

    send(fd, buffer, strlen(buffer), 0);
    recv(fd, buffer, sizeof(buffer), 0);
    printf("%s\r\n", buffer);
}
