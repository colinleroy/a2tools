MAX_LEVEL         = 31

has_byte      = tmp3

zp_jmp        = _zp10

INV_SPC      := ' '
SPC          := ' '|$80
SPKR         := $C030

ser_data     := $FFFF           ; Placeholders for legibility, going to be patched
ser_status   := $FFFF
txt_level    := $FFFF
