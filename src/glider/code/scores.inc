.define MAX_NAME_LEN 20
NUM_SCORES   = 10

.struct SCORE_LINE
  SCORE         .word
  NAME          .byte MAX_NAME_LEN
.endstruct

SCORE_TABLE_SIZE = NUM_SCORES*.sizeof(SCORE_LINE)
