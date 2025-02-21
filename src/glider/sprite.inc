.struct SPRITE_DATA
  X_COORD         .byte
  WIDTH           .byte
  Y_COORD         .byte
  HEIGHT          .byte
  PREV_X_COORD    .byte
  PREV_Y_COORD    .byte
  BYTES           .byte
  BYTES_WIDTH     .byte
  BACKUP_BUFFER   .addr
  SPRITE          .addr
  SPRITE_MASK     .addr
.endstruct
