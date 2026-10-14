#include <stdio.h>
#include <stdlib.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

int main(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    printf("param [%d] %s\n", i, argv[i]);
  }

  exit(EXIT_SUCCESS);
}
