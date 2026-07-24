#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    fprintf(stderr, "client says: %s\n", rbuf);

    char wbuf[] = "Sanika";
    write(connfd, wbuf, strlen(wbuf));
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0); // create a TCP socket, ipv4, stream
    if (fd < 0) { // if fd < 0, error
        die("socket()"); // socket error
    }

    // this is needed for most server applications
    int val = 1; // set SO_REUSEADDR to 1
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set socket option to reuse address

    // bind
    struct sockaddr_in addr = {}; // initialize sockaddr_in structure, set all fields to 0
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = ntohs(1234); // port 1234
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr)); // bind socket to address and port
    if (rv) { // if rv != 0, error
        die("bind()"); // bind error
    }

    // listen
    rv = listen(fd, SOMAXCONN); // listen for incoming connections, SOMAXCONN is the maximum number of pending connections
    if (rv) { // if rv != 0, error
        die("listen()"); // listen error
    }

    while (true) { // wait forever for incoming connections
        // accept
        struct sockaddr_in client_addr = {}; // initialize sockaddr_in structure for client address, set all fields to 0
        socklen_t addrlen = sizeof(client_addr); // size of client address structure
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen); // accept incoming connection, returns a new socket file descriptor for the connection
        if (connfd < 0) { // if connfd < 0, error
            continue;   // error
        }

        do_something(connfd); // do something with the connection, read and write data
        close(connfd); // close the connection socket
    }

    return 0;
}




// Start Server
//      │
//      ▼
// Create Socket
//      │
//      ▼
// Configure Socket
//      │
//      ▼
// Bind IP + Port
//      │
//      ▼
// Listen for Clients
//      │
//      ▼
// Wait Forever
//      │
//      ▼
// Client Connects
//      │
//      ▼
// Accept Connection
//      │
//      ▼
// Read Message
//      │
//      ▼
// Print Message
//      │
//      ▼
// Send Reply
//      │
//      ▼
// Close Client Connection
//      │
//      ▼
// Wait for Next Client