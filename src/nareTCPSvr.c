/* Copyright 2024 Felipe Markson dos Santos Monteiro <fmarkson@outlook.com> */
/*
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "nareTCPSvr.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NARE_NCLIENTS (10 * 1024)
#define NARE_BUFFER_SIZE (8 * 1024)

struct NareTCPSvr {
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    struct timespec time;
    size_t nclients;
    NareTCPSvrClient* clients;
};

struct NareTCPSvrClient {
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[NARE_BUFFER_SIZE];
    NareTCPSvr* svr;
};

static NareTCPSvrClient* NareTCPSvr_init_client(NareTCPSvr* server) {
    for (size_t i = 0; i < server->nclients; i++)
        if (server->clients[i].fd == 0) {
            server->clients[i].svr = server;
            return server->clients + i;
        }
    return NULL;
}

static void NareTCPSvr_deinit_client(NareTCPSvrClient* client) {
    client->fd = 0;
    memset(client->buffer, '\0', NARE_BUFFER_SIZE);
}

void NareTCPSvr_close(NareTCPSvr* server) {
    close(server->fd);
    free(server);
    return;
}

static void NareTCPSvr_closeclient(Nare* nare, NareTCPSvrClient* client) {
    int err = Nare_close(nare, NULL, NULL, client->fd);
    if (err < 0) {
        close(client->fd);  // Force.
        fprintf(stderr, "ERROR: Nare_close TOO MUCH SUBMITTED. Forcing close\n");
    }
    NareTCPSvr_deinit_client(client);
}

static void NareTCPSvr_wrote(Nare* nare, ssize_t result, void* user_data) {
    (void)result;
    NareTCPSvrClient* client = user_data;
    NareTCPSvr_closeclient(nare, client);
}

static void NareTCPSvr_read(Nare* nare, ssize_t result, void* user_data) {
    int err = 0;
    NareTCPSvrClient* client = user_data;
    if (result < 0) {
        // ERROR just close.
        fprintf(stderr, "ERROR! NareTCPSvr_read: %s", strerror(-result));
        NareTCPSvr_closeclient(nare, client);
        return;
    }

    // TODO CONTROLLER;
    printf("Readed %ld bytes\n", result);
    printf("%.*s", (int)result, client->buffer);
    printf("END\n");
    strcpy(client->buffer, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    err = Nare_send(nare, &NareTCPSvr_wrote, client, client->fd, client->buffer, strlen(client->buffer), 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Nare_send TOO MUCH SUBMITTED\n");
        NareTCPSvr_closeclient(nare, client);
    }
    return;
}

static void NareTCPSvr_accept(Nare* nare, ssize_t result, void* user_data);

static void NareTCPSvr_accepted(Nare* nare, ssize_t result, void* user_data) {
    int err = 0;
    NareTCPSvrClient* client = user_data;

    if (result > 0) {
        client->fd = result;
        err = Nare_recv(nare, &NareTCPSvr_read, client, client->fd, client->buffer, NARE_BUFFER_SIZE, 0);
        if (err < 0) {
            close(client->fd);
            fprintf(stderr, "ERROR: Nare_recv TOO MUCH SUBMITTED\n");
        }
    }

    NareTCPSvr_accept(nare, 0, client->svr);
    return;
}

static void NareTCPSvr_accept(Nare* nare, ssize_t result, void* user_data) {
    (void)result;
    int err = 0;
    NareTCPSvr* svr = user_data;
    NareTCPSvrClient* new_client = NareTCPSvr_init_client(svr);
    if (new_client == NULL) {
        fprintf(stderr, "NareTCPSvr_init_client: NO MORE CLIENTS AVAILABLE!\n");
        goto error;
    }

    err = Nare_accept(nare, &NareTCPSvr_accepted, new_client, svr->fd, (struct sockaddr*)&new_client->addr, &new_client->addr_len);
    if (err < 0) {
        fprintf(stderr, "FATAL: Nare_accept TOO MUCH SUBMITTED\n");
        goto error;
    }

    return;

error:
    svr->time.tv_nsec = 0;
    svr->time.tv_sec = 5;
    err = Nare_timeout(nare, &NareTCPSvr_accept, svr, &svr->time, 1);
    if (err < 0) {
        NareTCPSvr_close(svr);
        fprintf(stderr, "FATAL: Nare_timeout TOO MUCH SUBMITTED\n");
        exit(EXIT_FAILURE);
    }
}

NareTCPSvr* NareTCPSvr_open(Nare* nare, int port) {
    int err = -1;
    int reuse_addr = 1;
    NareTCPSvr* svr = calloc(1, sizeof(*svr) + NARE_NCLIENTS * (sizeof(*svr->clients)));
    if (svr == NULL) goto error;
    svr->nclients = NARE_NCLIENTS;
    svr->clients = (NareTCPSvrClient*)((unsigned char*)svr + sizeof(*svr));

    if (port > 0) {
        svr->addr.sin_port = htons(port);
    } else {
        svr->addr.sin_port = htons(80);
        port = 80;
    }

    svr->addr.sin_family = AF_INET;
    svr->addr_len = sizeof(svr->addr);

    err = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (err < 0) goto error;
    svr->fd = err;

    err = setsockopt(svr->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (err < 0) goto error;

    err = bind(svr->fd, (struct sockaddr*)&svr->addr, sizeof(svr->addr));
    if (err < 0) goto error;

    err = listen(svr->fd, SOMAXCONN);
    if (err < 0) goto error;

    NareTCPSvr_accept(nare, 0, svr);
    return svr;
error:
    if (svr != NULL) {
        close(svr->fd);
        free(svr);
    }
    return NULL;
}