#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

int main(int argc, char *argv[]) {
  FILE *fp;
  char *filename;
  int c;

  if (argc < 2) {
    printf("Usage: %s [input.raw]\n", argv[0]);
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    printf("Can not open %s\n", argv[1]);
    exit(1);
  }
  filename = argv[1];
  if (strchr(filename, '.')) {
    *strchr(filename, '.') = 0;
  }
  if (strchr(filename, '/')) {
    filename = strrchr(filename, '/')+1;
  }

  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf(".import sound_level_%d\n", c);
  }
  printf(".export _play_%s\n", filename);
  printf("_play_%s:\n"
         "php\n"
         "sei\n"
         "ldx #$32\n", filename);

  while ((c = fgetc(fp)) != EOF) {
    unsigned char r = (c*(NUM_LEVELS-1))/255;
    r = (r/STEP)*STEP;
    printf("jsr sound_level_%d\n", r);
  }
  printf("plp\n"
         "rts\n");

  fclose(fp);
}
