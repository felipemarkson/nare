# Nare

A low-level asynchronous I/O library for Linux based on [io_uring](https://github.com/axboe/liburing).

## Example as documentation

``` C
/* gcc -I includes/ src/nare.c examples/open.c -luring && a.out */
#include <fcntl.h>  /* openat constants */
#include <stdio.h>  /* printf, fprintf */
#include <stdlib.h> /* exit */
#include <string.h> /* strerror */

#include "nare.h"   /* Nare functions and types */

#define COMPLETED_NENTRIES 16 /* Can handle until 16 completed operations */
#define SUBMITTED_NENTRIES 8  /* Can handle until 8  submitted operations */

void free_resources(void);
void callback(Nare* nare, ssize_t result, void* user_data);

Nare* nare = NULL;
int main() {
    char* path = "examples/open.c";
    char* msg  = "Hi from Nare!";
    int err = 0;

    nare = Nare_alloc(COMPLETED_NENTRIES, SUBMITTED_NENTRIES); /* Alloc resources */
    if (nare == NULL) {
        perror("Nare_alloc");
        exit(EXIT_FAILURE);
    }

    err = Nare_openat(nare, callback, msg, AT_FDCWD, path, O_RDWR, 0); /* Submit openat. See man openat(2) */
    if (err < 0) {
        fprintf(stderr, "Error: Event loop full.\n"); /* Operations can check if it was submitted */
        exit(EXIT_FAILURE);
    }
    
    err = Nare_loop(nare); /* Start event loop */
    if (err < 0) {
        fprintf(stderr, "Error: Fail to start evento loop: %s\n", strerror(-err)); /* Event loop can fail due system errors */
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void callback(Nare* nare, ssize_t result, void* user_data) {
    char* msg = user_data;
    int fd = result;
    if (fd < 0) {
        fprintf(stderr, "Error: %s\n", strerror(-fd)); /* IO operations always set result to -errno on error */
        exit(EXIT_FAILURE);
    }
    printf("%s\n", msg);
    Nare_close(nare, NULL, NULL, fd); /* Submit close without callback */
    free_resources(); /* Gracefully exit */
}

void free_resources(void) {
    Nare_free(nare); /* Free resources */
    exit(EXIT_SUCCESS);
}
```

## Licence

Copyright 2024 Felipe Markson dos Santos Monteiro <fmarkson@outlook.com>

All files under `src/` and `includes/` folders are subject to the terms of the Mozilla Public License, v. 2.0 as detailed in the headers of each file.
See `LICENSE` for more details.

Other files are under Public Domain (CC0 1.0 Universal) License.
See `LICENSE-CC0` for more details.
