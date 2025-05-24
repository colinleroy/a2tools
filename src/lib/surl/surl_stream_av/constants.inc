MAX_LEVEL         = 31

has_byte      = tmp3

zp_jmp        = $00 ; Unused

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

VIDEOMODE_40COL = $11
VIDEOMODE_80COL = $12
