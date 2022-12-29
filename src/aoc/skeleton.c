#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN16E"

int main(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  read_file(fp);

  exit (0);
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);

  while (fgets(buf, BUFSIZE-1, fp) != NULL) {


  };

  printf("\n");

  free(buf);
  fclose(fp);
}
