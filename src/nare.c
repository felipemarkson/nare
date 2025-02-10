/* Copyright 2024 Felipe Markson dos Santos Monteiro <fmarkson@outlook.com> */
/*
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "nare.h"

#include <errno.h>            /* errno */
#include <liburing.h>         /* All io_uring_* functions */
#include <linux/time_types.h> /* struct __kernel_timespec */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>           /* malloc, free */
#include <string.h>

typedef struct NareState {
    void* user_data;
    NareCB cb;
} NareState;

typedef struct {
    void** ptrs;
    size_t size;
    size_t cap;
} NareStack;

static int stack_init(NareStack* stack, size_t cap) {
    stack->cap = cap;
    stack->ptrs = malloc(sizeof(*stack->ptrs)*stack->cap); 
    return stack->ptrs == NULL ? -1 : 0;
}
static void stack_deinit(NareStack* stack) {
    free(stack->ptrs);
    stack->ptrs = NULL;
    stack->size = 0;
    stack->cap = 0;
}

static int stack_push(NareStack* stack, void* elem) {
    if (stack->size >= stack->cap)
        return -1; // FULL
    stack->ptrs[stack->size] = elem;
    stack->size++;
    return 0;
}
static void* stack_pop(NareStack* stack) {
    if (stack->size == 0) return NULL; // EMPTY
    void* out = stack->ptrs[stack->size - 1];
    stack->size--;
    return out;
}


struct Nare {
    struct io_uring ring;
    size_t nstates;
    size_t last_state;
    size_t nactive;
    
    // States Pool
    NareState* states;
    NareStack states_available;
};

Nare* Nare_alloc(unsigned int cq_entries, unsigned int sq_entries) {
    Nare* nare = NULL;
    int err = 0;
    struct io_uring_params params = {.cq_entries = cq_entries, .sq_entries = sq_entries};
    nare = malloc(sizeof(*nare));
    if (nare == NULL) goto error;
    memset(nare, 0, sizeof(*nare));
    nare->nstates = cq_entries + sq_entries;
    nare->states = malloc(sizeof(*nare->states)*(nare->nstates));
    if(nare->states == NULL) goto error;
    memset(nare->states, 0, sizeof(*nare->states)*(nare->nstates));

    if (stack_init(&nare->states_available, nare->nstates) < 0)
        goto error;

    for (size_t i = nare->nstates; i > 0; i--) {
        size_t j = i - 1;
        stack_push(&nare->states_available, nare->states + j);
    }

    err = io_uring_queue_init_params(sq_entries, &nare->ring, &params);
    if (err < 0) {
        errno = -err;
        goto error;
    }
    return nare;

error:
    if (nare != NULL)
        Nare_free(nare);
    return NULL;
}

void Nare_free(Nare* nare) {
    if (nare == NULL) return;

    io_uring_queue_exit(&nare->ring);
    stack_deinit(&nare->states_available);
    free(nare->states);
    free(nare);
}

static NareState* Nare_get_state(Nare* nare) {
    return stack_pop(&nare->states_available);;
}

static void Nare_release_state(Nare* nare, NareState* state) {
    memset(state, 0, sizeof(*state));
    stack_push(&nare->states_available, state);
}

size_t Nare_nactive(const Nare* nare) {
    return nare->nactive;
}

int Nare_step(Nare* nare) {
    int ret = 0;
    size_t count = 0;
    unsigned head = 0;
    struct io_uring_cqe* cqe = NULL;
    ret = io_uring_submit_and_wait(&nare->ring, 1);
    if (ret < 0) return ret;
    io_uring_for_each_cqe(&nare->ring, head, cqe) {
        ++count;
        struct NareState* state = (void*)cqe->user_data;
        void* user_data = state->user_data;
        NareCB cb = state->cb;
        Nare_release_state(nare, state);
        if (cb != NULL) cb(nare, cqe->res, user_data);
    }
    io_uring_cq_advance(&nare->ring, count);
    nare->nactive -= count;
    return 0;
}

/* BASIC IO OPS*/

#define NARE_OP_BOILERPLATE(io_uring_op, ...)                 \
    struct io_uring_sqe* sqe = io_uring_get_sqe(&nare->ring); \
    NareState* result = NULL;                                 \
    if (sqe == NULL) return -1;                               \
                                                              \
    result = Nare_get_state(nare);                            \
    if (result == NULL) return -1;                            \
                                                              \
    io_uring_op(sqe, __VA_ARGS__);                            \
    result->cb = cb;                                          \
    result->user_data = user_data;                            \
    io_uring_sqe_set_data(sqe, result);                       \
    nare->nactive++;                                          \
    return 0;

int Nare_openat(Nare* nare, NareCB cb, void* user_data, int dir_fd, const char* path, int flags, mode_t mode) {
    NARE_OP_BOILERPLATE(io_uring_prep_openat, dir_fd, path, flags, mode);
}

int Nare_write(Nare* nare, NareCB cb, void* user_data, int fd, const void* buffer, size_t nbytes, size_t offset) {
    NARE_OP_BOILERPLATE(io_uring_prep_write, fd, buffer, nbytes, offset);
}

int Nare_read(Nare* nare, NareCB cb, void* user_data, int fd, void* buffer, size_t nbytes, size_t offset) {
    NARE_OP_BOILERPLATE(io_uring_prep_read, fd, buffer, nbytes, offset);
}

int Nare_close(Nare* nare, NareCB cb, void* user_data, int fd) {
    NARE_OP_BOILERPLATE(io_uring_prep_close, fd);
}

int Nare_unlinkat(Nare* nare, NareCB cb, void* user_data, int dir_fd, const char* path, int flags) {
    NARE_OP_BOILERPLATE(io_uring_prep_unlinkat, dir_fd, path, flags);
}

/* NETWORK OPS */

int Nare_accept(Nare* nare, NareCB cb, void* user_data, int sockfd, struct sockaddr* client_addr, socklen_t* client_addr_len) {
    NARE_OP_BOILERPLATE(io_uring_prep_accept, sockfd, client_addr, client_addr_len, 0);
}

int Nare_recv(Nare* nare, NareCB cb, void* user_data, int sockfd, void* buffer, size_t nbytes, int flags) {
    NARE_OP_BOILERPLATE(io_uring_prep_recv, sockfd, buffer, nbytes, flags);
}

int Nare_send(Nare* nare, NareCB cb, void* user_data, int sockfd, const void* buffer, size_t nbytes, int flags) {
    NARE_OP_BOILERPLATE(io_uring_prep_send, sockfd, buffer, nbytes, flags);
}

int Nare_sendto(Nare* nare, NareCB cb, void* user_data, int sockfd, const void* buffer, size_t nbytes, int flags, const struct sockaddr* addr, socklen_t addrlen) {
    NARE_OP_BOILERPLATE(io_uring_prep_sendto, sockfd, buffer, nbytes, flags, addr, addrlen);
}

int Nare_socket(Nare* nare, NareCB cb, void* user_data, int domain, int type, int protocol, unsigned int flags) {
    NARE_OP_BOILERPLATE(io_uring_prep_socket, domain, type, protocol, flags);
}

int Nare_bind(Nare* nare, NareCB cb, void* user_data, int fd, struct sockaddr* addr, socklen_t addrlen) {
    NARE_OP_BOILERPLATE(io_uring_prep_bind, fd, addr, addrlen);
}

int Nare_listen(Nare* nare, NareCB cb, void* user_data, int fd, int backlog) {
    NARE_OP_BOILERPLATE(io_uring_prep_listen, fd, backlog);
}

int Nare_connect(Nare* nare, NareCB cb, void* user_data, int fd, const struct sockaddr* addr, socklen_t addrlen) {
    NARE_OP_BOILERPLATE(io_uring_prep_connect, fd, addr, addrlen);
}

/* MISC */

int Nare_timeout(Nare* nare, NareCB cb, void* user_data, struct timespec* ts) {
    NARE_OP_BOILERPLATE(io_uring_prep_timeout, (struct __kernel_timespec*)ts,0 ,0);
}