#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

#define MAXL 20

int main(int argc, char **argv) {
  char name[MAXL + 2];
  int times;
  char *times_var = getenv("TIMES");
  if (times_var)
    times = atoi(times_var);
  else
    times = 1;

  while (fgets(name, MAXL + 2, stdin) != NULL)
    for (int i = 0; i < times; i++)
      printf("Hello %s", name);

  if (putenv("RESULT=Done") != 0) {
    ERR("putenv failed");
  }

  printf("%s\n", getenv("RESULT"));
  if (system("env|grep RESULT") != 0)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
