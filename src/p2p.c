#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <sys/random.h>

#include "p2p.h"

#define K 20

#define ID_BYTES 16
#define ID_BITS (ID_BYTES * 8)

#define INITIAL_RTT_EST ((struct timeval){.tv_sec = 1, .tv_usec = 0})
#define INITIAL_RTT_VAR ((struct timeval){.tv_sec = 0, .tv_usec = 0})

#define NSEC_IN_SEC 1000000000L

struct pending;
typedef uint8_t id[ID_BYTES];

uint8_t peer_distance(id *a, id *b);
int add_pending(struct p2p *node, struct pending *p);
int remove_pending(struct p2p *node, uint32_t seqnum);

struct peer {
    int alive;
    id id;

    struct timespec rtt_est;
    struct timespec rtt_var;

    void *addr; // When implementing a network layer on top of this, this is populated by the recvfrom callback
};

struct pending {
    uint32_t seqnum;
    struct timespec expires;

    id id;
};

struct p2p {
    uint32_t seqnum;
    id id;

    uint8_t buckets[ID_BITS]; // Each bucket points to a peer list that would contain a node of distance [i]
    struct peer *peers[ID_BITS]; // Each pointer points to an array of K peers

    // The pending array is ordered by seqnum
    size_t pending_len;
    size_t pending_cap;
    struct pending *pending;

    struct p2p_socket impl;

    size_t aux_len;
    char aux[];
};

struct p2p *init_node(struct p2p_socket impl, const void *aux, size_t aux_len)
{
    ssize_t err;
    struct peer *bucket;

    struct p2p *node = malloc(sizeof(*node) + aux_len);
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

    node->pending_len = 0;
    node->pending_cap = K;
    node->pending = malloc(sizeof(*node->pending) * K);
    if (node->pending == NULL) {
        free(bucket);
        goto fail;
    }

    node->aux_len = aux_len;
    memcpy(node->aux, aux, aux_len);

    node->impl = impl;

    return node;

fail:
    free(node);
    return NULL;
}

void free_node(struct p2p *node)
{
    if (!node) {
        return;
    }

    for (int i = 0; i < ID_BITS; i++) {
        free(node->peers[i]);
    }

    free(node->pending);
    free(node);
}

int handle_packet(struct p2p *node, void *packet, size_t packet_len, void *peer_addr);
void add_timespec(struct timespec *tx, const struct timespec *ty);
void sub_timespec(struct timespec *tx, const struct timespec *ty);
int cmp_timespec(const struct timespec *tx, const struct timespec *ty);
int poll_node(struct p2p *node, struct timespec *timeout)
{
    assert(node);
    assert(timeout);

    char buf[P2P_PACKET_MAX_LEN];

    int err;
    struct timespec target;
    struct timespec remaining = *timeout;
    err = clock_gettime(CLOCK_MONOTONIC, &target);
    if (err < 0) {
        return -1;
    }

    add_timespec(&target, timeout);

    while (remaining.tv_sec >= 0) {
        void *peer_addr = malloc(node->impl.addr_len);
        if (peer_addr == NULL) {
            return -1;
        }

        ssize_t len = node->impl.recvfrom(node->aux, node->aux_len, buf, sizeof(buf), peer_addr, &remaining);
        if (len < 0) {
            free(peer_addr);
            return -1;
        }

        // handle_packet takes ownership of peer_addr
        err = handle_packet(node, buf, len, peer_addr);
        if (err < 0) {
            return -1;
        }

        struct timespec cur_time;
        err = clock_gettime(CLOCK_MONOTONIC, &cur_time);
        if (err < 0) {
            return -1;
        }

        remaining = target;
        sub_timespec(&remaining, &cur_time); // remaining = target - cur_time
    }

    // Perform some bookkeeping like removing pending packets from the queue
    for (int i = 0; i < node->pending_len; i++) {
        if (cmp_timespec(&target, &node->pending[i].expires) > 0) {
            remove_pending(node, node->pending[i].seqnum);
        }
    }

    return 0;
}

int handle_packet(struct p2p *node, void *packet, size_t packet_len, void *peer_addr)
{
    free(peer_addr);
    return 0;
}

void add_timespec(struct timespec *tx, const struct timespec *ty)
{
    assert(tx->tv_nsec > 0 && tx->tv_nsec < NSEC_IN_SEC);

    assert(ty->tv_sec > 0);
    assert(ty->tv_nsec > 0 && ty->tv_nsec < NSEC_IN_SEC);

    tx->tv_nsec += ty->tv_nsec;
    if (tx->tv_nsec > NSEC_IN_SEC) {
        tx->tv_nsec -= NSEC_IN_SEC;
        tx->tv_sec++;
    }

    tx->tv_sec += ty->tv_sec;

    assert(tx->tv_nsec > 0 && tx->tv_nsec < NSEC_IN_SEC);
}

void sub_timespec(struct timespec *tx, const struct timespec *ty)
{
    assert(tx->tv_nsec > 0 && tx->tv_nsec < NSEC_IN_SEC);

    assert(ty->tv_sec > 0);
    assert(ty->tv_nsec > 0 && ty->tv_nsec < NSEC_IN_SEC);

    assert(tx->tv_sec > ty->tv_sec ||
           (tx->tv_sec == tx->tv_sec && tx->tv_nsec >= ty->tv_nsec));
        
    tx->tv_nsec -= ty->tv_nsec;
    if (tx->tv_nsec < 0) {
        tx->tv_nsec += NSEC_IN_SEC;
        tx->tv_sec--;
    }

    tx->tv_sec -= ty->tv_sec;

    assert(tx->tv_nsec > 0 && tx->tv_nsec < NSEC_IN_SEC);
}

int cmp_timespec(const struct timespec *tx, const struct timespec *ty)
{
    assert(tx->tv_sec > 0);
    assert(tx->tv_nsec > 0 && tx->tv_nsec < NSEC_IN_SEC);

    assert(ty->tv_sec > 0);
    assert(ty->tv_nsec > 0 && ty->tv_nsec < NSEC_IN_SEC);

    if (tx->tv_sec > ty->tv_sec) {
        return 1;
    } else if (tx->tv_sec < ty->tv_sec) {
        return -1;
    }

    if (tx->tv_nsec > ty->tv_nsec) {
        return 1;
    } else if (tx->tv_nsec < ty->tv_nsec) {
        return -1;
    }

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

int add_pending(struct p2p *node, struct pending *p)
{
    if (node->pending_len == node->pending_cap) {
        // Attempt to grow pending
        size_t new_cap = node->pending_cap * 3 / 2;
        struct pending *new_pending = realloc(node->pending, sizeof(*new_pending) * new_cap);
        if (new_pending == NULL) {
            return -1;
        }

        node->pending_cap = new_cap;
        node->pending = new_pending;
    }

    node->pending[node->pending_len++] = *p;

    return 0;
}

int remove_pending(struct p2p *node, uint32_t seqnum)
{
    if (node->pending_len == 0) {
        return 0;
    }

    for (int i = 0; i < node->pending_len; i++) {
        if (node->pending[i].seqnum == seqnum) {
            size_t span = (node->pending_len - i - 1) * sizeof(*node->pending);
            memmove(&node->pending[i], &node->pending[i+1], span);
            break;
        }
    }

    return 1;
}