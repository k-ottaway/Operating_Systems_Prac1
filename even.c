// even.c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Handle SIGINT (Ctrl+C)
void sig_int(int int_signal) {
  printf("Yeah!\n");
  fflush(stdout);
}

// Handle SIGHUP (hangup)
void sig_hup(int hup_signal) {
  printf("Ouch!\n");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  // Install signal handlers
  signal(SIGINT, sig_int);
  signal(SIGHUP, sig_hup);

  // Check command line argument
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <positive number>\n", argv[0]);
    return 1;
  }

  int number = atoi(argv[1]);  // convert command line argument to int
  if (number < 0) {
    fprintf(stderr, "Please enter a positive number\n");
    return 1;
  }

  // Print the first n even numbers
  for (int i = 0; i < 2 * number; i += 2) {
    printf("%d ", i);
    fflush(stdout);
    sleep(5);  // delay to allow time for signals
  }
  printf("\n");
  return 0;
}
