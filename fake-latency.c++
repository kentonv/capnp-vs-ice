// This is a really crappy program that introduces artificial latency into a connection.
// I looked for something like this on the internet but remarkably could not find it.  I
// probably did not look hard enough.
//
// Anyway, the program listens for connections on port 9999 and forwards them to port 10000.
// The client -> server link is delayed by 100ms; the server -> client link passes through
// normally.  This is equivalent to delaying both directions by half the amount.  The program
// prints a notification for each round trip completed.

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#define DO_SYSCALL(code) \
({ \
  auto result = code; \
  if (result < 0) { \
    perror(#code); \
    exit(1); \
  } \
  result; \
})

struct Pump {
  int in;
  int out;

  void pump() {
    char buffer[8192];

    bool dataSent = false;

    for (;;) {
      ssize_t n = read(in, buffer, sizeof(buffer));

      if (n < 0) {
        int error = errno;
        if (error == EBADF || error == EINTR) {
          // We closed the FD in doit().  We're done.
          break;
        } else if (error == EAGAIN) {
          // The FD is nonblocking, which means its the one where we want to insert delays.

          if (dataSent) {
            // Since the client has sent data and didn't follow it up with at EOF, we indicate
            // a round trip occurred.
            printf("(round trip)\n");
            dataSent = false;
          }

          // Wait for data.
          struct pollfd pfd;
          pfd.fd = in;
          pfd.events = POLLIN;
          pfd.revents = 0;
          DO_SYSCALL(poll(&pfd, 1, -1));

          // Sleep for 100ms to introduce latency.
          usleep(100000);
        } else {
          perror("read");
          exit(1);
        }
      } else if (n == 0) {
        // EOF
        break;
      } else {
        dataSent = true;
        char* pos = buffer;
        while (n > 0) {
          size_t n2 = DO_SYSCALL(write(out, pos, n));
          n -= n2;
          pos += n2;
        }
      }
    }

    shutdown(out, SHUT_WR);
  }
};

void noop(int) {}

void* threadFunc(void* param) {
  signal(SIGUSR1, &noop);
  ((Pump*)param)->pump();
  return nullptr;
}

int main(int argc, char* argv[]) {
  // build listen addr
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9999);
  addr.sin_addr.s_addr = 0;

  // listen
  int listener = DO_SYSCALL(socket(AF_INET, SOCK_STREAM, 0));
  int optval = 1;
  DO_SYSCALL(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));
  DO_SYSCALL(bind(listener, (struct sockaddr*)&addr, sizeof(addr)));
  DO_SYSCALL(listen(listener, 5));

  for (;;) {
    // accept
    struct sockaddr_in peer;
    socklen_t peerSize = sizeof(peer);
    int in = DO_SYSCALL(accept(listener, (struct sockaddr*)&peer, &peerSize));
    DO_SYSCALL(setsockopt(in, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)));

    // build connect addr
    struct sockaddr_in outAddr;
    outAddr.sin_family = AF_INET;
    outAddr.sin_port = htons(10000);
    outAddr.sin_addr.s_addr = htonl(0x7f000001);  // 127.0.0.1

    // connect
    int out = socket(AF_INET, SOCK_STREAM, 0);
    DO_SYSCALL(setsockopt(out, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)));
    DO_SYSCALL(connect(out, (struct sockaddr*)&outAddr, sizeof(outAddr)));

    // We'll set `in` to nonblocking which will cause the pumping code to insert a delay whenever
    // read() blocks.
    DO_SYSCALL(fcntl(in, F_SETFL, O_NONBLOCK));

    // Pump data in both directions.
    Pump a = { in, out };
    Pump b = { out, in };
    pthread_t thread;
    pthread_create(&thread, nullptr, &threadFunc, &b);
    a.pump();

    // The server doesn't always necessarily close the connection in the other direction.  Force
    // shutdown really crappily here.
    usleep(10000);  // Give time for server to respond.
    close(out);

    // Wake up any syscall that might be running in the other thread, so that it can discover
    // that the FD is no longer valid.  (A syscall on an FD holds a reference to the FD, so closing
    // it doesn't cancel the syscall.  On Linux, at least.)
    pthread_kill(thread, SIGUSR1);

    pthread_join(thread, nullptr);
    close(in);
  }
}
