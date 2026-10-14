#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

#define BUFF_SIZE 1024
#define MAX_LIST 1024

// -------------------------
// Dict

struct KV {
  char *key;
  char *value;
};

typedef struct {
  struct KV *elements[MAX_LIST];
  int count;
} Dict;

Dict *dict_init(void) {
  Dict *d = (Dict *)malloc(sizeof(Dict));
  if (!d)
    ERR("malloc metadata");
  d->count = 0;
  return d;
}

void dict_add(Dict *d, const char *key, const char *value) {
  if (d->count >= MAX_LIST)
    ERR("dict capacity");

  struct KV *elem = (struct KV *)malloc(sizeof(struct KV));
  if (!elem)
    ERR("malloc elem");

  elem->key = strdup(key);
  elem->value = strdup(value);

  if (!elem->key || !elem->value)
    ERR("strdup");

  d->elements[d->count++] = elem;
}

const char *dict_get(Dict *d, const char *key) {
  for (int i = 0; i < d->count; i++) {
    struct KV *elem = d->elements[i];
    if (strcmp(elem->key, key) == 0) {
      return elem->value;
    }
  }
  return NULL;
}

void dict_free(Dict *d) {
  if (!d)
    return;
  for (int i = 0; i < d->count; i++) {
    struct KV *elem = d->elements[i];
    free(elem->key);
    free(elem->value);
    free(elem);
  }
  free(d);
}

// END: Dict
// -------------------------
// PART 1

void usage(char *prog) {
  fprintf(stderr, "USAGE:%s\n", prog);
  exit(EXIT_FAILURE);
}

void parse_line(char *buff, Dict *metadata) {
  buff[strcspn(buff, "\r\n")] = 0;

  if (strlen(buff) == 0)
    return;

  char *key = strtok(buff, ":");
  if (key == NULL)
    return;

  char *value = strtok(NULL, ":");
  if (value == NULL)
    return;

  dict_add(metadata, key, value);
}

Dict *parse_metadata(char *file_path) {
  char buff[BUFF_SIZE];
  Dict *metadata = dict_init();

  FILE *fp = fopen(file_path, "r");
  if (fp == NULL)
    ERR("fopen");

  while (fgets(buff, BUFF_SIZE, fp) != NULL) {
    parse_line(buff, metadata);
  }

  if (fclose(fp) != 0)
    ERR("fclose");

  return metadata;
}

void print_fields(Dict *metadata) {
  char *search_keys[] = {"name", "type", "rarity"};
  for (int j = 0; j < 3; j++) {
    char *search = search_keys[j];
    const char *value = dict_get(metadata, search);
    if (value) {
      printf("%s: %s\n", search, value);

    } else
      printf("%s missing!\n", search);
  }
}
// END: PART 1
// -------------------------

// Etap 2 (by-name): płaskie dowiązanie nazwane wartością pola.
// Etap 3 (by-rarity / by-type): podkatalog by-<by>/<value> z dowiązaniem
// nazwanym polem name (przyciętym do 64 bajtów); plik bez pola <by> pomijany.
void process_warehouse_file(char *index_dir, char *by, char *file_path) {
  Dict *metadata = parse_metadata(file_path);
  if (metadata == NULL)
    return;

  const char *name = dict_get(metadata, "name");
  if (name == NULL) {
    dict_free(metadata);
    return;
  }
  // przycięcie do 64 BAJTÓW (wg formatu z Etapu 3/4), nie znaków
  char truncated_name[65];
  size_t name_len = strlen(name);
  if (name_len > sizeof truncated_name - 1)
    name_len = sizeof truncated_name - 1;
  memcpy(truncated_name, name, name_len);
  truncated_name[name_len] = 0;

  char ln_target[PATH_MAX];
  char ln_source[PATH_MAX];

  if (strcmp(by, "name") == 0) {
    snprintf(ln_target, sizeof(ln_target), "%s/by-%s/%s", index_dir, by,
             truncated_name);
    snprintf(ln_source, sizeof(ln_source), "../../%s", file_path);
  } else {
    const char *value = dict_get(metadata, by);
    if (value == NULL) {
      // brak pola rarity/type: plik pomijamy
      dict_free(metadata);
      return;
    }
    char ln_dir[PATH_MAX - 128];
    snprintf(ln_dir, sizeof(ln_dir), "%s/by-%s/%s", index_dir, by, value);
    if (mkdir(ln_dir, (mode_t)0777) != 0 && errno != EEXIST)
      ERR("mkdir subfolder");

    snprintf(ln_target, sizeof(ln_target), "%s/%s", ln_dir, truncated_name);
    snprintf(ln_source, sizeof(ln_source), "../../../%s", file_path);
  }

  if (symlink(ln_source, ln_target) < 0) {
    if (errno == EEXIST)
      ERR("symlink exists");
    ERR("symlink failed");
  }
  dict_free(metadata);
}
void process_warehouse_dir(char *current_dir, char *index_dir, char *by) {
  DIR *dirp;
  struct dirent *dirent;

  if ((dirp = opendir(current_dir)) == NULL)
    ERR("opendir failed");

  do {
    errno = 0;
    if ((dirent = readdir(dirp)) != NULL) {
      if (strcmp(".", dirent->d_name) == 0 || strcmp("..", dirent->d_name) == 0)
        continue;

      char file_path[PATH_MAX];
      snprintf(file_path, sizeof(file_path), "%s/%s", current_dir,
               dirent->d_name);

      struct stat st;
      if (lstat(file_path, &st) < 0)
        ERR("lstat failed");

      if (S_ISDIR(st.st_mode))
        process_warehouse_dir(file_path, index_dir, by);
      else if (S_ISREG(st.st_mode))
        process_warehouse_file(index_dir, by, file_path);
    }
  } while (dirent != NULL);

  if (errno != 0)
    ERR("readdir failed");

  if (closedir(dirp))
    ERR("closedir");
}

void create_sub_index(char *warehouse_dir, char *index_dir, char *by) {
  char sub_index_path[PATH_MAX];
  snprintf(sub_index_path, sizeof(sub_index_path), "%s/by-%s", index_dir, by);
  if (mkdir(sub_index_path, (mode_t)0777) != 0)
    ERR("subindex dir exists");

  process_warehouse_dir(warehouse_dir, index_dir, by);
}

void create_index(char *warehouse_dir, char *index_dir) {
  if (mkdir(index_dir, (mode_t)0777) != 0)
    ERR("index dir  exists");

  create_sub_index(warehouse_dir, index_dir, "name");
  create_sub_index(warehouse_dir, index_dir, "rarity");
  create_sub_index(warehouse_dir, index_dir, "type");
}

int main(int argc, char **argv) {
  if (argc > 1)
    usage(argv[0]);
  create_index("warehouse", "alchemist_index");
  return EXIT_SUCCESS;
}
