#include <stdio.h>
#include <stdlib.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

#define NARGS 2

void usage(char *pname) {
  fprintf(stderr, "USAGE:%s name times>0\n", pname);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  if (argc != NARGS + 1)
    usage(argv[0]);
  char *name = argv[1];
  int n_times = atoi(argv[2]);
  if (n_times <= 0)
    ERR("times should be positive");

  for (int i = 0; i < n_times; i++) {
    printf("Hello %s\n", name);
  }
  exit(EXIT_SUCCESS);
}
