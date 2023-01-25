#include <stdio.h>
int main(int argc, char *argv[]) {
  int i = 0;
  int n_posts = 6;
  int N_STATUS_TO_LOAD = 6;
  int root = 1;
  char keeping_root = 0;
  int loaded = atoi(argv[1]);
  
  for (i = 0; i < loaded + keeping_root; i++) {
    if (i != root)
      printf("free %d\n", i);
    else {
      printf("keeping %d at 0\n", i);
      keeping_root = 1;
    }
  }

  for (i = 0 + keeping_root; i < n_posts - loaded ; i++) {
    char offset = i + loaded;
    printf("move %d to %d\n", offset, i);

  }
  for (i = n_posts - loaded; i < n_posts; i++) {
    char offset = i - (n_posts - loaded);
    printf("new id from newids %d to %d\n", offset, i);
  }

  printf("first -= %d\n", loaded);
  
}



/* 2 3 4 5 */
