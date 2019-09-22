#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include <sys/random.h>

#include "p2p.h"

#define K 20

#define ID_BYTES 16
#define ID_BITS (ID_BYTES * 8)

typedef uint8_t id[ID_BYTES];

struct peer {
    int alive;
    id id;
    void *addr; // When implementing a network layer on top of this, this will be populated by a callback or something
};

struct p2p {
    uint32_t seqnum;
    id id;

    uint8_t buckets[ID_BITS]; // Each bucket points to a peer list that would contain a node of distance [i]
    struct peer *peers[ID_BITS]; // Each pointer points to an array of K peers

    size_t addr_len;
    uint8_t addr[];
};

struct p2p *init_node(void *addr, size_t addr_len)
{
    ssize_t err;
    struct peer *bucket;

    struct p2p *node = malloc(sizeof(*node) + addr_len);
    if (node == NULL) {
        goto fail;
    }

    node->seqnum = 0;
    err = getrandom(node->id, sizeof(node->id), 0);
    if (err != sizeof(node->id)) {
        goto fail;
    }

    memset(&node->buckets, 0, sizeof(node->buckets));
    for (int i = 0; i < ID_BITS; i++) {
        node->peers[i] = NULL;
    }

    bucket = calloc(K, sizeof(*bucket));
    if (bucket == NULL) {
        goto fail;
    }

    node->peers[0] = bucket;

    node->addr_len = addr_len;
    memcpy(node->addr, addr, addr_len);

    return node;

fail:
    free(node);
    return NULL;
}

uint8_t leading_zeros(uint8_t x);
uint8_t peer_distance(id *a, id *b)
{
    uint8_t count = 0;
    for (int i = 0; i < ID_BYTES; i++) {
        if (*a[i] == *b[i]) {
            count += 8;
        } else {
            count += leading_zeros(*a[i] ^ *b[i]);
            break;
        }
    }

    return count;
}

uint8_t leading_zeros(uint8_t x)
{
    uint8_t lz = 8;
    while (x != 0) {
        x >>= 1;
        lz--;
    }

    return lz;
}