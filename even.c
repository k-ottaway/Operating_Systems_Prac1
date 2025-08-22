// even.c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Handler for SIGINT (Ctrl+C)
void sig_int(int sig) {
  write(STDOUT_FILENO, "Yeah!\n", 6);  // async-signal-safe
}

// Handler for SIGHUP
void sig_hup(int sig) {
  write(STDOUT_FILENO, "Ouch!\n", 6);  // async-signal-safe
}

int main(int argc, char *argv[]) {
  struct sigaction sa_int, sa_hup;

  // Set up SIGINT handler
  sa_int.sa_handler = sig_int;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags =
      SA_RESTART;  // restart syscalls like sleep/printf after signal
  sigaction(SIGINT, &sa_int, NULL);

  // Set up SIGHUP handler
  sa_hup.sa_handler = sig_hup;
  sigemptyset(&sa_hup.sa_mask);
  sa_hup.sa_flags = SA_RESTART;
  sigaction(SIGHUP, &sa_hup, NULL);

  // Check command line argument
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <positive number>\n", argv[0]);
    return 1;
  }

  int number = atoi(argv[1]);
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
