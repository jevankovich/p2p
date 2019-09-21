#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "net.h"

int open_control_socket(in_port_t *port)
{
    struct sockaddr_in6 addr;
    socklen_t socklen = sizeof(addr);
    int sock;
    int err;

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        return -1;
    }

    addr = (struct sockaddr_in6){
        .sin6_family = AF_INET6,
        .sin6_port = htons(*port),
        .sin6_addr = in6addr_any,
    };
    err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (err == -1) {
        return -1;
    }

    err = getsockname(sock, (struct sockaddr *)&addr, &socklen);
    if (err == -1) {
        return -1;
    }

    *port = ntohs(addr.sin6_port);

    return sock;
}