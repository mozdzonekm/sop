#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

#define FILL_PERCENTAGE 10

struct Params {
  char *name;
  mode_t perms;
  ssize_t size;
};

void usage(char *pname) {
  fprintf(stderr, "USAGE:%s -n Name -p OCTAL -s SIZE\n", pname);
  exit(EXIT_FAILURE);
}

void parse_params(int argc, char **argv, struct Params *params) {
  int c;
  while ((c = getopt(argc, argv, "p:n:s:")) != -1) {
    switch (c) {
    case 'p':
      params->perms = strtol(optarg, (char **)NULL, 8);
      break;
    case 's':
      params->size = strtol(optarg, (char **)NULL, 10);
      break;
    case 'n':
      params->name = optarg;
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }
  if ((NULL == params->name) || ((mode_t)-1 == params->perms) ||
      ((ssize_t)-1 == params->size) || (params->size <= 0)) {
    usage(argv[0]);
  }
}

void delete_file(const struct Params *params) {
  if (unlink(params->name) && errno != ENOENT)
    ERR("unlink");
}

void make_file(const struct Params *params) {
  FILE *fp;
  umask(~params->perms & 0777);
  if ((fp = fopen(params->name, "w+")) == NULL)
    ERR("fopen");
  for (int i = 0; i < (params->size * FILL_PERCENTAGE) / 100; i++) {
    if (fseek(fp, rand() % params->size, SEEK_SET))
      ERR("fseek");
    fprintf(fp, "%c", 'A' + (i % ('Z' - 'A' + 1)));
  }
  if (fclose(fp))
    ERR("fclose");
}

int main(int argc, char **argv) {
  struct Params params = {.name = NULL, .perms = (mode_t)-1, .size = -1};
  srand(time(NULL));

  parse_params(argc, argv, &params);
  delete_file(&params);
  make_file(&params);
  return EXIT_SUCCESS;
}
