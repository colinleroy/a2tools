#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN18"

int main(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  read_file(fp);

  exit (0);
}

typedef struct _coords {
  char x;
  char y;
  char z;
} coords;

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  coords *coords = NULL;
  int num_coords = 0, i;
  int num_faces = 0, num_connected_faces = 0;
  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    char *s_x, *s_y, *s_z;
    coords = realloc(coords, (num_coords + 1) * sizeof(coords));
    s_x = buf;

    s_y = strchr(s_x, ',') + 1;
    s_z = strchr(s_y, ',') + 1;
    *strchr(s_x, ',') = '\0';
    *strchr(s_y, ',') = '\0';

    coords[num_coords].x = atoi(s_x);
    coords[num_coords].y = atoi(s_y);
    coords[num_coords].z = atoi(s_z);

    num_faces += 6;
    printf("Searching around %d,%d,%d...", 
           coords[num_coords].x,
           coords[num_coords].y,
           coords[num_coords].z);
    for (i = 0; i < num_coords; i++) {
      if (coords[i].x == coords[num_coords].x - 1 &&
          coords[i].y == coords[num_coords].y &&
          coords[i].z == coords[num_coords].z) {
            printf(".");
            num_connected_faces += 2;
      }
      if (coords[i].x == coords[num_coords].x + 1 &&
          coords[i].y == coords[num_coords].y &&
          coords[i].z == coords[num_coords].z) {
            printf(".");
            num_connected_faces += 2;
      }
      if (coords[i].x == coords[num_coords].x &&
          coords[i].y == coords[num_coords].y - 1 &&
          coords[i].z == coords[num_coords].z) {
            printf(".");
            num_connected_faces += 2;
      }
      if (coords[i].x == coords[num_coords].x &&
          coords[i].y == coords[num_coords].y + 1 &&
          coords[i].z == coords[num_coords].z) {
            printf(".");
            num_connected_faces += 2;
      }
      if (coords[i].x == coords[num_coords].x &&
          coords[i].y == coords[num_coords].y &&
          coords[i].z == coords[num_coords].z - 1) {
            printf(".");
            num_connected_faces += 2;
      }
      if (coords[i].x == coords[num_coords].x &&
          coords[i].y == coords[num_coords].y &&
          coords[i].z == coords[num_coords].z + 1) {
            printf(".");
            num_connected_faces += 2;
      }
    }
    printf("\n");
    num_coords++;
  };
  free(buf);
  fclose(fp);

  printf("%d cubes, %d faces,\n"
         "%d connected, %d unconnected\n",
         num_coords, num_faces,
         num_connected_faces, num_faces - num_connected_faces);

  free(coords);
}
