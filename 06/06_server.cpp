// stdlib
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// system
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
// C++
#include <vector>


int main() {
    // the listening socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);   //socket(ipv4, tcp, 0) ,0 -> default protocol
    if (fd < 0) {
        die("socket()");
    }
    int val = 1; // allow reuse of the address
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // allow reuse of the address

    // bind
    struct sockaddr_in addr = {}; // initialize to zero
    addr.sin_family = AF_INET;    // IPv4
    addr.sin_port = ntohs(1234);  // port 1234
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));   // bind to the address
    if (rv) {
        die("bind()");
    }

    // set the listen fd to nonblocking mode
    fd_set_nb(fd);

    // listen
    rv = listen(fd, SOMAXCONN); // SOMAXCONN is the maximum number of connections allowed by the system, rv is 0 on success, -1 on error
    if (rv) {
        die("listen()");
    }

    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn; //
    // the event loop
    std::vector<struct pollfd> poll_args;   //Stores the sockets that will be passed to poll().
    while (true) {
        // prepare the arguments of the poll()
        poll_args.clear();
        // put the listening sockets in the first position
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // the rest are connection sockets
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            // always poll() for error
            struct pollfd pfd = {conn->fd, POLLERR, 0};
            // poll() flags from the application's intent
            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);
        }

        // wait for readiness
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR) {
            continue;   // not an error
        }
        if (rv < 0) {
            die("poll");
        }

        // handle the listening socket
        if (poll_args[0].revents) {
            if (Conn *conn = handle_accept(fd)) {
                // put it into the map
                if (fd2conn.size() <= (size_t)conn->fd) {
                    fd2conn.resize(conn->fd + 1);
                }
                assert(!fd2conn[conn->fd]);
                fd2conn[conn->fd] = conn;
            }
        }

        // handle connection sockets
        for (size_t i = 1; i < poll_args.size(); ++i) { // note: skip the 1st
            uint32_t ready = poll_args[i].revents;
            if (ready == 0) {
                continue;
            }

            Conn *conn = fd2conn[poll_args[i].fd];
            if (ready & POLLIN) {
                assert(conn->want_read);
                handle_read(conn);  // application logic
            }
            if (ready & POLLOUT) {
                assert(conn->want_write);
                handle_write(conn); // application logic
            }

            // close the socket from socket error or application logic
            if ((ready & POLLERR) || conn->want_close) {
                (void)close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }   // for each connection sockets
    }   // the event loop
    return 0;
}










// while(true)
//         |
//         |
//         v
// 1. Build pollfd list
//         |
//         |
//         +--> Add listening socket
//         |        |
//         |        v
//         |    events = POLLIN
//         |    (new client check)
//         |
//         |
//         +--> Loop over fd2conn
//                  |
//                  |
//                  v
//           Create pfd for client
//                  |
//                  |
//           if want_read
//                  |
//                  v
//              add POLLIN
                 
//           if want_write
//                  |
//                  v
//              add POLLOUT

//                  |
//                  v

//         poll_args.push_back(pfd)


//         |
//         v

// 2. Call poll()

//         |
//         v

// 🧠 Kernel checks sockets

//         |
//         v

// Kernel sets revents


//         |
//         v

// 3. Handle events


//         |
//         +----------------+
//         |                |
//         v                v

//    POLLIN            POLLOUT

//        |                |
//        v                v

// handle_read()    handle_write()


//        |
//        |
//        v

// 4. Check connection close

// (POLLERR || want_close)

//        |
//        v

// close(fd)

// delete Conn


//        |
//        |
//        v

// Repeat loop








// 1. Variable Description
// Variable	Type	Purpose
// fd	             int	            Listening socket fd (accepts new clients)
// fd2conn	         vector<Conn*>	    Maps fd → Conn object for all clients
// Conn	             struct	            Stores state of one client connection
// conn->fd	         int	            Client socket fd

// want_read	bool	    Server wants to read data from client
// want_write	bool	    Server has response and wants to send data
// want_close	bool	    Connection should be closed
// poll_args	vector<pollfd>	List of sockets given to poll()
// pfd.events	short   	Events server is interested in (POLLIN, POLLOUT)
// pfd.revents	short	    Events returned by kernel after poll()
// ready	    uint32_t	Stores which event happened (POLLIN/POLLOUT)