#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN16E"

int stack_size_test(void *ptr, int x, int y, int z, int d) {
  char result = 0;
  printf("%d.", d);
  return result || stack_size_test(ptr, x, y, z, d + 1);
}

int main(void) {
  char *buf;
  int i, j;
  char ***cubes = NULL;
  int num_cubes = 0;
  int surface_area = 0;
  int exterior_surface_area = 0;
  char *s_x, *s_y, *s_z;
  int x, y, z;
  
  cubes = malloc(16000);
  return stack_size_test(cubes, 1, 2, 3, 0);
}
