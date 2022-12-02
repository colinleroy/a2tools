/* No more readability now */
#include <stdio.h>
#include <stdlib.h>

#define LINE_FORMAT "A X\n"

/* transform A to 1, B to 2, C to 3 */
int score_by_shape(char shape) {
  return (shape - 'A' + 1);
}

/* transform X to 0, Y to 3, Z to 6 */
int score_by_outcome(char outcome) {
  return (outcome - 'X') * 3;
}

/* shift/modulo opponent_move according to outcome:
 * opponent_move + 1 modulo 3 for winning,
 * opponent_move + 2 modulo 3 for losing
 * opponent_move + 0 modulo 3 for draw :
 * A/X -> C, A/Y -> A, A/Z -> B, 
 * B/X -> A, B/Y -> B, B/Z -> C
 * C/X -> B, C/Y -> C, C/Z -> A
 */
char calculate_my_move(char opponent_move, char expected_outcome) {
  int shift = (expected_outcome - 'X' + 2) % 3;
  return ((opponent_move - 'A' + shift) % 3) + 'A';
}

int main(int argc, char **argv) {
  char line[sizeof(LINE_FORMAT) + 1];
  int score = 0;

  while(NULL != fgets(line, sizeof(line), stdin)) {
    score += score_by_outcome(line[2])
             + score_by_shape(calculate_my_move(line[0], line[2]));
  }

  printf("Score: %d\n", score);

  exit(0);
}
