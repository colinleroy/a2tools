#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#ifndef __CC65__
#include <libgen.h>
#endif

int get_filedetails(char *filename, unsigned long *size, unsigned char *type, unsigned *auxtype) {

  struct dirent *d;
  DIR *dir;
  *size = 0;
  *type = 0;
  *auxtype = 0;

  dir = opendir(".");
  while (d = readdir(dir)) {
    if (!strcasecmp(filename, d->d_name)) {
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

  if (stat(filename, &statbuf) < 0) {
    return -1;
  }
  
  *size = statbuf.st_size;
#endif

  if (*size == 0) {
    return -1;
  }
  return 0;
}
