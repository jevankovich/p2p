#pragma once

// #include <time.h>

#define P2P_PACKET_MAX_LEN 16

/* Struct containing all of the information and callbacks needed by p2p at the
 * socket layer.
 * 
 * When constructing a p2p node, it effectively wraps the provided socket
 * interface and allows you to use it like you would a socket over the id
 * space instead of the socket's notion of address.
 */
struct p2p_socket {
    size_t addr_len;
    ssize_t (*recvfrom)(const void *aux,
                        size_t aux_len,
                        char *buf,
                        size_t buf_len,
                        void *peer_addr,
                        const struct timespec *timeout);
    ssize_t (*sendto)(const void *aux,
                      size_t aux_len,
                      const char *buf,
                      size_t buf_len,
                      const void *peer_addr);
};

// Opaque struct representing the state of a p2p node
struct p2p;

// Initializes a p2p structure
struct p2p *init_node(struct p2p_socket impl, const void *aux, size_t aux_len);
// Frees a p2p structure initialized by init_node
void free_node(struct p2p *node);

// Performs all of the maintenance operations on a node such as handling timed out packets, receiving packets, etc.
int poll_node(struct p2p *node, struct timespec *timeout);