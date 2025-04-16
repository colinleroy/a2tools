.define LEN 12
.struct SCORE_LINE
  SCORE         .word
  NAME          .byte LEN
.endstruct

SCORE_TABLE_SIZE = MAX_WINNERS*.sizeof(SCORE_LINE)
