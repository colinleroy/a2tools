#include <stdio.h>
#include <string.h>
#include "hgr.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

void backup_restore_logo(char *op) {
#ifdef __APPLE2ENH__
  FILE *fp = fopen("/RAM/WOZAMP.HGR", op);
  if (!fp) {
    return;
  }
  if (op[0] == 'w') {
    fwrite((char *)HGR_PAGE, 1, HGR_LEN, fp);
  } else {
    fread((char *)HGR_PAGE, 1, HGR_LEN, fp);
  }
  fclose(fp);
#else
  if (op[0] == 'r') {
    bzero((char *)HGR_PAGE, HGR_LEN);
  }
#endif
}
