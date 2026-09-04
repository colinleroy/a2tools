#include <unistd.h>
#include <string.h>

char *zxloader_name = NULL;

void zexec(char *args) {
#ifdef __CC65__
  exec(zxloader_name, args);
#endif
}
