#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "extended_conio.h"
#include "get_filedetails.h"
#include "progress_bar.h"

#define TRANSFER_BUF_SIZE 2048

static char dest_file[64];
static char transfer_buf[TRANSFER_BUF_SIZE];
unsigned char scrw, scrh;

int __fastcall__ copy_file(char *source_file, const char *dest_dir) {
  char *filename = NULL;
  unsigned long size;
  unsigned char type;
  unsigned auxtype;
  size_t r, total;
  int result = -1;
  FILE *srcfp = NULL;
  FILE *dstfp = NULL;
  char x, y;

  screensize(&scrw, &scrh);

  if (get_filedetails(source_file, &filename, &size, &type, &auxtype) < 0) {
    goto err_out;
  }
  
  snprintf(dest_file, sizeof(dest_file), "%s/%s", dest_dir, filename);
#ifdef __APPLE2__
  _filetype = type;
  _auxtype = auxtype;
#endif
  srcfp = fopen(source_file, "r");
  if (!srcfp) {
    goto err_out;
  }

  dstfp = fopen(dest_file, "w");
  if (!dstfp) {
    goto err_out;
  }

  x = wherex();
  y = wherey();
  progress_bar(x, y, scrw - x, 0, size);
  while ((r = fread(transfer_buf, 1, TRANSFER_BUF_SIZE, srcfp)) > 0) {
    if (fwrite(transfer_buf, 1, r, dstfp) < r) {
      goto err_out;
    }
    total += r;
    progress_bar(x, y, scrw - x, total, size);
  }

  result = 0;

err_out:

  if (srcfp)
    fclose(srcfp);
  if (dstfp)
    fclose(dstfp);

  return result;
}
