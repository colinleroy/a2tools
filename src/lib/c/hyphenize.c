#include <string.h>

void hyphenize(char *str, int len) {
  if (strlen(str) < len) {
    return;
  }
  str[len-3] = str[len-2] = str[len-1] = '.';
  str[len] = '\0';
}
