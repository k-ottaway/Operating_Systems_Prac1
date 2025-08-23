#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_signal(int signal) {
  if (signal == SIGHUP) {
    printf("Ouch!");
    fflush(stdout);
  } else if (signal == SIGINT) {
    printf("Yeah!");
    fflush(stdout);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s n\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);
  if (n < 0) {
    fprintf(stderr, "n must be non-negative\n");
    return 1;
  }

  // Use sigaction (preferred over signal)
  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGHUP, &sa, NULL) == -1) {
    perror("sigaction SIGHUP");
    return 1;
  }
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction SIGINT");
    return 1;
  }

  for (int i = 0; i < n; i++) {
    printf("%d\n", 2 * i);
    fflush(stdout);
    sleep(5);
  }

  return 0;
}