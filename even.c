#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_signals(int signal) {
  // and prints a message accordingly
  if (signal == SIGHUP)
    printf("Ouch!\n");
  else if (signal == SIGINT)
    printf("Yeah!\n");
  fflush(stdout);  // immediately prints output
}

int main(int arg_command, char *arg_vector[]) {
  int n = atoi(arg_vector[1]);  // convert the string to an integer

  signal(SIGHUP, handle_signals);  // call signals handler
  signal(SIGINT, handle_signals);

  for (int i = 0; i < n; i++) {
    printf(" %d\n", i * 2);  // print the first n even numbers
    fflush(stdout);
    sleep(5);
  }
  return 0;
}
