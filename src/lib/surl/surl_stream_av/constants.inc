MAX_LEVEL         = 31

.ifdef IIGS                   ; Todo
HAS_BYTE          = $01
.else
HAS_BYTE          = $08
.endif

MAX_OFFSET    = 126
N_BASES       = (8192/MAX_OFFSET)+1
N_TEXT_BASES  = 4
PAGE_TOGGLE   = $7F

.ifdef DOUBLE_BUFFER
PAGE0_HB      = $20
PAGE1_HB      = $40
.else
PAGE0_HB      = $20
PAGE1_HB      = PAGE0_HB
.endif

SPKR         := $C030
