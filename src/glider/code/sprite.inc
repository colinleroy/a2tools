MAX_SPRITES = 10

.struct SPRITE_DATA
  ACTIVE          .byte
  DEADLY          .byte
  DESTROYABLE     .byte
  STATIC          .byte
  X_COORD         .byte
  WIDTH           .byte
  Y_COORD         .byte
  HEIGHT          .byte
  PREV_X_COORD    .byte
  PREV_Y_COORD    .byte
  BYTES           .byte
  BYTES_WIDTH     .byte
  SPRITE          .addr
  DEACTIVATE_DATA .byte
  DEACTIVATE_FUNC .addr
  STATE_BACKUP    .word
  BG_BACKUP       .addr
  NEED_CLEAR      .byte

.endstruct
