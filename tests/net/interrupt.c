#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// A thread interrupting another one a few times with SIGUSR1 while it blocks, then unblocking it
typedef struct interrupter_t {
    pthread_t self;
    pthread_t target;
    int fd;          // a byte is written to it once the interruptions are over, -1 for none
    uint16_t port;   // connected to on the loopback once the interruptions are over, 0 for none
    int connected;   // the socket of that connection, closed by the join
} interrupter_t;

static void noop_handler (int sig) {
    (void) sig;
}

static void * interrupter_main (void * arg) {
    interrupter_t * it = arg;
    for (int i = 0; i < 5; i++) {
        usleep (20000);
        pthread_kill (it-> target, SIGUSR1);
    }

    usleep (20000);
    if (it-> fd >= 0) {
        uint8_t byte = 42;
        (void) write (it-> fd, &byte, 1);
    }

    if (it-> port != 0) {
        struct sockaddr_in addr;
        memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons (it-> port);
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

        it-> connected = socket (AF_INET, SOCK_STREAM, 0);
        (void) connect (it-> connected, (struct sockaddr *) &addr, sizeof (addr));
    }

    return NULL;
}

// Without SA_RESTART, a blocking call of the interrupted thread fails with EINTR instead of resuming
static interrupter_t * start (int fd, uint16_t port) {
    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &noop_handler;
    sigemptyset (&sa.sa_mask);
    sigaction (SIGUSR1, &sa, NULL);

    interrupter_t * it = calloc (1, sizeof (interrupter_t));
    it-> target = pthread_self ();
    it-> fd = fd;
    it-> port = port;
    it-> connected = -1;
    pthread_create (&it-> self, NULL, &interrupter_main, it);

    return it;
}

void * _yrt_test_interrupt_then_write (int fd) {
    return start (fd, 0);
}

void * _yrt_test_interrupt_then_connect (uint16_t port) {
    return start (-1, port);
}

void _yrt_test_interrupter_join (void * handle) {
    interrupter_t * it = handle;
    pthread_join (it-> self, NULL);
    if (it-> connected >= 0) close (it-> connected);
    free (it);
}
