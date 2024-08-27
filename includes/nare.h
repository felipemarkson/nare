/* Copyright 2024 Felipe Markson dos Santos Monteiro <fmarkson@outlook.com> */
/*
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef NARE_H
#define NARE_H
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

typedef struct Nare Nare;

Nare* Nare_alloc(unsigned int cq_entries, unsigned int sq_entries);
void Nare_free(Nare* nare);
int Nare_loop(Nare* nare);

typedef void (*NareCB)(Nare* nare, ssize_t result, void* user_data);

/* BASIC IO OPS*/

int Nare_openat(Nare* nare, NareCB cb, void* user_data, int dir_fd, const char* path, int flags, mode_t mode);
int Nare_write(Nare* nare, NareCB cb, void* user_data, int fd, const void* buffer, size_t nbytes, size_t offset);
int Nare_read(Nare* nare, NareCB cb, void* user_data, int fd, void* buffer, size_t nbytes, size_t offset);
int Nare_close(Nare* nare, NareCB cb, void* user_data, int fd);

/* NETWORK OPS */

int Nare_accept(Nare* nare, NareCB cb, void* user_data, int sockfd, struct sockaddr* client_addr, socklen_t* client_addr_len);
int Nare_recv(Nare* nare, NareCB cb, void* user_data, int sockfd, void* buffer, size_t nbytes, int flags);
int Nare_send(Nare* nare, NareCB cb, void* user_data, int sockfd, const void* buffer, size_t nbytes, int flags);
int Nare_sendto(Nare* nare, NareCB cb, void* user_data, int sockfd, const void* buffer, size_t nbytes, int flags, const struct sockaddr* addr, socklen_t addrlen);
int Nare_socket(Nare* nare, NareCB cb, void* user_data, int domain, int type, int protocol, unsigned int flags);
int Nare_bind(Nare* nare, NareCB cb, void* user_data, int fd, struct sockaddr* addr, socklen_t addrlen);
int Nare_listen(Nare* nare, NareCB cb, void* user_data, int fd, int backlog);
int Nare_connect(Nare* nare, NareCB cb, void* user_data, int fd, const struct sockaddr* addr, socklen_t addrlen);

/* MISC */

int Nare_timeout(Nare* nare, NareCB cb, void* user_data, struct timespec* ts, unsigned int count);


#endif
