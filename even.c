#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void sig_int(int int_signal) {
  printf("Yeah!\n");
  fflush(stdout);
}

void sig_hup(int hup_signal) {
  printf("Ouch!\n");
  fflush(stdout);
}

int main() {
  signal(SIGINT, sig_int);
  signal(SIGHUP, sig_hup);

  int number;
  printf("Enter a number: ");
  scanf("%d", &number);

  // make sure n is a positive number
  if (number < 0) {
    printf("please enter a positive number \n");
    return -1;
  }

  // prints the first n even numbers starting from 0
  for (int i = 0; i < 2 * number; i += 2) {
    printf("%d ", i);
    fflush(stdout);
    sleep(5);
  }
  printf("\n");
  return 0;
}