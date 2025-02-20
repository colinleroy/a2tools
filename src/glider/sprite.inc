.struct SPRITE_DATA
  X_COORD         .byte
  PREV_X_COORD    .byte
  Y_COORD         .byte
  PREV_Y_COORD    .byte
  BYTES           .byte
  WIDTH           .byte
  BACKUP_BUFFER   .addr
  SPRITE          .addr
  SPRITE_MASK     .addr
.endstruct
