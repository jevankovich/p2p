#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <netinet/in.h>

#include <unistd.h>

#include "net.h"
#include "p2p.h"

const struct p2p_socket P2P_IMPL = {
    .addr_len = sizeof(struct sockaddr_in6),
    .recvfrom = NULL,
    .sendto = NULL,
};

int main(int argc, char **argv)
{
    struct p2p *node;
    in_port_t port = 0;

    int ctrl = open_control_socket(&port);
    if (ctrl == -1) {
        perror("Failed opening control socket");
        return -1;
    }

    printf("Bound control socket to port %" PRIu16 "\n", port);

    node = init_node(P2P_IMPL, &ctrl, sizeof(port));
    free_node(node);
    node = NULL;

    close(ctrl);
}
