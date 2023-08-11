#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#ifndef __CC65__
#include <libgen.h>
#endif

static char file_parts[FILENAME_MAX];

int get_filedetails(char *path, char **filename, unsigned long *size, unsigned char *type, unsigned *auxtype) {

  struct dirent *d;
  DIR *dir;
  char *dirname;

  *size = 0;
  *type = 0;
  *auxtype = 0;
  strncpy(file_parts, path, FILENAME_MAX - 1);
  if (strrchr(file_parts + 1, '/')) {
    dirname = file_parts;
    *filename = strrchr(file_parts + 1, '/') + 1;
    *strrchr(file_parts + 1, '/') = '\0';
  } else {
    dirname = ".";
    *filename = path;
  }

  dir = opendir(dirname);
  if (dir == NULL) {
    return -1;
  }
  while (d = readdir(dir)) {
    if (!strcasecmp(*filename, d->d_name)) {
      *type = d->d_type;
#ifdef __APPLE2__
      *size = d->d_size;
      *auxtype = d->d_auxtype;
#endif
      break;
    }
  }

  closedir(dir);

#ifndef __CC65__
  struct stat statbuf;

  if (stat(path, &statbuf) < 0) {
    return -1;
  }

  if (strrchr(path, '/'))
    *filename = strrchr(path, '/') + 1;
  else
    *filename = path;

  *size = statbuf.st_size;
#endif

  if (*size == 0) {
    return -1;
  }
  return 0;
}
