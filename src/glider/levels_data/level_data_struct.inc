LEVEL_DATA_START = $4000

.struct LEVEL_DATA
  SPRITES_DATA  .addr
  VENTS_DATA    .addr
  BLOCKERS_DATA .addr
  EXITS_DATA    .addr
  LOGIC_CB      .addr
.endstruct
