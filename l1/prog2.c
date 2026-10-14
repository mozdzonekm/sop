#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))

#define MAX_PATH 101

void scan_dir() {
  DIR *dirp;
  struct dirent *dirent;
  struct stat filestat;

  int dirs = 0, files = 0, links = 0, others = 0;

  if ((dirp = opendir(".")) == NULL)
    ERR("opendir failed");

  do {
    errno = 0;
    if ((dirent = readdir(dirp)) != NULL) {
      if (lstat(dirent->d_name, &filestat))
        ERR("lstat");

      if (S_ISREG(filestat.st_mode))
        files++;
      else if (S_ISDIR(filestat.st_mode))
        dirs++;
      else if (S_ISLNK(filestat.st_mode))
        links++;
      else
        others++;
    }
  } while (dirent != NULL);
  if (errno != 0)
    ERR("readdir failed");

  if (closedir(dirp))
    ERR("closedir");

  printf("Files: %d, Dirs: %d, Links: %d, Others: %d\n", files, dirs, links,
         others);
}

int main(int argc, char **argv) {
  char cwd[MAX_PATH];
  if (getcwd(cwd, MAX_PATH) == NULL)
    ERR("getcwd");
  for (int i = 1; i < argc; i++) {
    char *dir = argv[i];
    if (chdir(dir) != 0)
      ERR("chdir");
    scan_dir();

    if (chdir(cwd) != 0)
      ERR("chdir");
  }
  return EXIT_SUCCESS;
}
