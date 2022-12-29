#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __CC65__
#include <apple2enh.h>
#include <conio.h>
#endif

char line[255];

int main(void) {
  int xreg = 1;
  int ncycles = 0, cycles_to_do = 0;
  int to_add = 0;
  int can_read = 1;
  int instruction = 0;
  int sigstrength = 0;
  int sum = 0;
  int col, row = -1;
  FILE *fp;

#ifdef __CC65__
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen("IN10", "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }
  
#ifdef __CC65__
  clrscr();
#endif

  do {
    if (instruction == 0) {
      if (fgets(line, sizeof(line), fp) == NULL) {
        break;
      } else {
        if (line[0] == 'n') {
          instruction = 1;
        } else {
          instruction = 2;
          to_add = atoi(line+5);
        }
      }
    }
    ncycles++;

    /* during cycle */
    sigstrength = ncycles*xreg;
    col = (ncycles - 1) % 40;
    if (col + 1== 20) {
      /* part 1 */
      sum += sigstrength;
    }
    if (col == 0) {
      row++;
      printf("\n");
    }
    printf((col >= xreg - 1 && col <= xreg + 1) ? "#":" ");
    /* end during cycle */

    instruction--;
    if (instruction == 0) {
      xreg += to_add;
      to_add = 0;
    }
  } while (1);

  /* part 1 */
  printf("\n\nDone. Signal strength sum: %d.\n", sum);
  fclose(fp);
  exit(0);
}
