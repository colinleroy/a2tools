MAX_LEVEL         = 31

.ifdef IIGS
HAS_BYTE          = $01
.else
HAS_BYTE          = $08
.endif

INV_SPC      := ' '
SPC          := ' '|$80
SPKR         := $C030

ser_data     := $FFFF           ; Placeholders for legibility, going to be patched
ser_status   := $FFFF
txt_level    := $FFFF
