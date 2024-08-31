#include <stdio.h>
#include "hgr.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#ifdef __APPLE2ENH__
void backup_restore_logo(char *op) {
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
}
#endif
