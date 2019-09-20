#include <time.h>
#include <stdint.h>

#include "p2p.h"

#define K 20

#define ID_BYTES 16
#define ID_BITS (ID_BYTES * 8)

typedef uint8_t id[ID_BYTES];

struct peer {
    id id;
};

struct p2p {
    uint32_t seqnum;
    id id;

    struct peer xor_peers[ID_BITS][K];

    uint8_t addr[];
};

struct p2p *init_node(void)
{
    return 0;
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