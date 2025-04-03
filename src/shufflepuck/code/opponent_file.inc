.struct OPPONENT
  SPRITE            .res 2+2+3
  NAME              .res 2+2+3
  LOSE_POINT        .res 2+2+3
  WIN_POINT         .res 2+2+3
  THINK_CB          .addr
.endstruct

.import __OPPONENT_START__
