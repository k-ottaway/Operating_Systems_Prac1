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
  signal(SIGINT, handle_signal);
  signal(SIGHUP, handle_signal);

  // prints the first n even numbers starting from 0
  for (int i = 0; i < 2 * n; i += 2) {
    printf("%d ", i);
    fflush(stdout);
    sleep(5);
  }
  return 0;
}