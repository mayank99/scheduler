#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void damn(char *s) {
  printf("%s\n", s);
  exit(1);
}

int random_between(int from, int to) {
  return (rand() % (to - from + 1)) + from;
}

// int main() {
//   for (int i = 0; i < 100; i++) {
//     printf("%d\n", random_between(0, 99));
//   }
// }