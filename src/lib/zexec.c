#include <unistd.h>
#include <string.h>

char *zxloader_name = NULL;

void zexec(char *args) {
  exec(zxloader_name, args);
}
