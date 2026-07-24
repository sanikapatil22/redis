#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


static void die(const char *msg) { // error handling function
    int err = errno; // get the error number
    fprintf(stderr, "[%d] %s\n", err, msg); // print error message to stderr
    abort(); // terminate the program
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0); // create a TCP socket, ipv4, stream, 
    if (fd < 0) { // if fd < 0, error
        die("socket()"); // socket error
    }

    struct sockaddr_in addr = {}; // initialize sockaddr_in structure, set all fields to 0
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = ntohs(1234); // port 1234
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr)); //if rv=0, success, if rv=-1, error
    if (rv) {
        die("connect");
    }

    char msg[] = "lessgo"; // send "lessgo" to server
    write(fd, msg, strlen(msg)); // send message to server

    char rbuf[64] = {}; // buffer to receive message from server
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1); // read message from server
    if (n < 0) { // if n < 0, error
        die("read"); // read error
    }
    printf("server says: %s\n", rbuf); // print message from server
    close(fd); // close socket
    return 0;
}
