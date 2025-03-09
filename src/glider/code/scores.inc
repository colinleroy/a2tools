MAX_NAME_LEN = 14

.struct SCORE_LINE
  SCORE         .word
  NAME          .byte 14
.endstruct

SCORE_TABLE_SIZE = 10*.sizeof(SCORE_LINE)
