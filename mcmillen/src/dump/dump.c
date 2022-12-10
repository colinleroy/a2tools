#include <stdio.h>
#include <stdlib.h>
#include "extended_conio.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2.h>

#define BUF_SIZE 255
#define DATA_SIZE 16385

int main(void) {
  int l = 0, exit_code = 0;
  char *filename = malloc(BUF_SIZE);
  char *buf = malloc(DATA_SIZE);
  size_t b = 0;
  FILE *infp = NULL;

  if (buf == NULL) {
    printf("Couldn't allocate data buffer.\n");
    exit (1);
  }

  printf("Enter filename: ");
  cgets(buf, BUF_SIZE);

  _filetype = PRODOS_T_TXT;

  infp = fopen(filename,"r");
  if (infp == NULL) {
    printf("Open error %d: %s\n", errno, strerror(errno));
    exit_code = 1;
    goto err_out;
  }
  
  while (fgets(buf, DATA_SIZE, infp) != NULL) {
    l++;
    printf("%s", buf);
    b += strlen(buf);

    if (l == 23) {
      printf("(Hit a key to continue)\n");
      while(!kbhit());
      cgetc();
      gotoxy(0,24);
      l = 0;
    }
  }
  if (feof(infp)) {
    printf("\n(End of file. %d bytes read)\n", b);
  } else if (ferror(infp)) {
    printf("\nError %d: %s\n", errno, strerror(errno));
    
  }
  
  if (fclose(infp) != 0) {
    printf("Close error %d: %s\n", errno, strerror(errno));
  }

err_out:
  free(filename);
  free(buf);
  exit(0);
}
