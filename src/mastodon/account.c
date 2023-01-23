#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"

#define BUF_SIZE 255

account *account_new(void) {
  account *a = malloc(sizeof(account));
  if (a == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  memset(a, 0, sizeof(account));
  return a;
}

void account_free(account *a) {
  if (a == NULL)
    return;
  free(a->id);
  free(a->username);
  free(a->display_name);
  free(a);
}
