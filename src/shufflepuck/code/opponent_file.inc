.struct OPPONENT
  SPRITE            .res 3  ; jmp
  NAME              .res 3  ; jmp
  LOSE_POINT        .res 3  ; jmp
  LOSE_POINT_SND    .res 3  ; jmp
  WIN_POINT         .res 3  ; jmp
  WIN_POINT_SND     .res 3  ; jmp
  END_GAME          .res 3  ; jmp
  HIT_CB            .res 3  ; jmp - inform opponent about hit (serial)
  THINK_CB          .addr   ; start of opponent's logic function
.endstruct

.import __OPPONENT_START__
