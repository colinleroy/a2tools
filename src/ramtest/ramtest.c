#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <apple2.h>
#include "simple_em.h"

static char *test_str[5] = {
  "This is string 1.",
  "And now number 2",
  "we have five lines",
  "in the 80 columns card.",
  "finished, we're done."
};

int main(void) {
  int i, e;
  if ((e = em_init()) != 0) {
    printf("Error %d installing driver.\n", e);
    exit(1);
  }
  
  for (i = 0; i < 5; i++) {
    printf("> %d '%s'\n", i, test_str[i]);
    em_store_str(i, test_str[i]);
  }

  for (i = 0; i < 5; i++) {
    char *data = em_read(i, 256);
    
    printf("%d< '%s'\n", i, data);
    free(data);
  }
  exit (0);
}
