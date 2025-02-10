// cc simple.c -lnare -luring
#include <nare.h>
#define STDOUT_FD 1
#define NULL 0

/* callbacks have access to:
    - The Nare pointer;
    - The result of the operation (Typically -errno on error);
    - The pointer passed at the 'user_data' argument of the operation.
   Note that the pointer passed in the operation's 'user_data' argument should
   be valid when the callback is called */
void callback(Nare* nare, ssize_t result, void* user_data);

int main(void) {
    int data = 0;
    /* Allocate memory resources to deal '2' completed operation and '4'
       submitted operation */
    Nare* nare = Nare_alloc(2, 4);
    // Prepare async timeouts and immediately return. They are not submitted.
    Nare_timeout(nare, &callback, &data, &(struct timespec){.tv_sec = 1});
    Nare_timeout(nare, &callback, &data, &(struct timespec){.tv_sec = 2});
    Nare_timeout(nare, &callback, &data, &(struct timespec){.tv_sec = 3});
    // Submit and blocks until the first timeout
    Nare_step(nare);
    // Prepare new operation write. Users can safetly pass NULL at the cb
    const char async_msg[] = "Async write worked!\n";
    Nare_write(nare, NULL, NULL, STDOUT_FD, async_msg, sizeof(async_msg), NULL);
    // Users can check if there are operations not processed
    while (Nare_nactive(nare) > 0) {
        Nare_step(nare); // Submit and blocks until the next event
    }
    /* Prepare new write operation, but it will not be performed because it was
       not submitted and waited */
    Nare_write(nare, NULL, NULL, STDOUT_FD, async_msg, sizeof(async_msg), 0);
    // Free the memory
    Nare_free(nare);
    return 0;
}

#include <stdio.h>
void callback(Nare* nare, ssize_t result, void* user_data) {
    printf("Timeout %d\n", *(int*)user_data);
    /* It is safe to acess the user_data if it is used only in 'user_data'
       operations */
    *(int*)user_data += 1;
}


