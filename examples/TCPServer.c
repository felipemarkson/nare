#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nare.h"
#include "nareTCPSvr.h"

#define PORT 5555
#define NENTRIES 4096

Nare* nare = NULL;
NareTCPSvr* svr = NULL;
void close_resources(int signal) {
    if (svr != NULL) NareTCPSvr_close(svr);
    if (nare != NULL) Nare_free(nare);
    exit(signal);
}

int main(int argc, char const* argv[]) {
    signal(SIGINT, close_resources);

    nare = Nare_alloc(NENTRIES * 4, NENTRIES);
    if (nare == NULL) {
        perror("Nare_alloc");
        return 1;
    }
    svr = NareTCPSvr_open(nare, PORT);
    if (svr == NULL) {
        perror("NareTCPSvr_open");
        return 1;
    }

    printf("Listening on port %d\n", PORT);
    int err = Nare_loop(nare);
    if (err < 0) {
        errno = -err;
        perror("Nare_loop");
        return 1;
    }

    return 0;
}
