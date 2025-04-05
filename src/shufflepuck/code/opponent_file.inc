.struct OPPONENT
  SPRITE            .res 2+2+3  ; ldx, ldy, jmp
  NAME              .res 2+2+3  ; ldx, ldy, jmp
  LOSE_POINT        .res 2+2+3  ; ldx, ldy, jmp
  LOSE_POINT_SND    .res 2+3    ; ldy, jmp
  WIN_POINT         .res 2+2+3  ; ldx, ldy, jmp
  WIN_POINT_SND     .res 2+3    ; jmp
  THINK_CB          .addr       ; start of opponent's logic function
.endstruct

.import __OPPONENT_START__
