MAX_SPRITES = 10

.struct SPRITE_DATA
  X_COORD         .byte
  WIDTH           .byte
  Y_COORD         .byte
  HEIGHT          .byte
  PREV_X_COORD    .byte
  PREV_Y_COORD    .byte
  BYTES           .byte
  BYTES_WIDTH     .byte
  SPRITE          .addr
  BG_BACKUP       .addr
  NEED_CLEAR      .byte

.endstruct
