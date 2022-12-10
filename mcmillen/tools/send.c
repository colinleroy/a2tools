/* No more readability now */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

int main(int argc, char **argv) {
  FILE *fp, *outfp;
  char *filename;
  char *filetype;
  char c;
  int count = 0;
  char buf[128];
  struct stat statbuf;

  if (argc < 3) {
    printf("Usage: %s [file] [output tty]\n", argv[0]);
    exit(1);
  }

  if (stat(argv[1], &statbuf) < 0) {
    printf("Can't stat %s\n", argv[1]);
    exit(1);
  }
  
  fp = fopen(argv[1],"r");
  if (fp == NULL) {
    printf("Can't open %s\n", argv[1]);
    exit(1);
  }
  
  filename = basename(argv[1]);
  if (strchr(filename, '.') != NULL) {
    filetype = strchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }

  outfp = fopen(argv[2], "r+b");
  if (outfp == NULL) {
    printf("Can't open %s\n", argv[2]);
    exit(1);
  }

  /* Send filename */
  fprintf(outfp, "%s\n", filename);
  printf("Filename sent: %s\n", filename);
  sleep(1);

  /* Send filetype */
  fprintf(outfp, "%s\n", filetype);
  printf("Filetype sent: %s\n", filetype);
  sleep(1);

  if (!strcasecmp(filetype, "BIN")) {
    char buf[58];
    fread(buf, 1, 58, fp);
    if (buf[0] == 0x00 && buf[1] == 0x05
     && buf[2] == 0x16 && buf[3] == 0x00) {
      printf("AppleSingle file detected, skipping header\n");
    } else {
      rewind(fp);
    }
  }

  while(fread(&c, 1, 1, fp) > 0) {
    fwrite(&c, 1, 1, outfp);
    fflush(outfp);
    count++;
    if (count % 512 == 0) {
      printf("Wrote %d bytes...\n", count);
    }
    usleep(10*1000);
  }

  fclose(fp);
  fclose(outfp);
  printf("Wrote %d bytes.\n", count);
  exit(0);
}
