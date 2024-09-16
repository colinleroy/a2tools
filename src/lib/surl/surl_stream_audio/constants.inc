MAX_LEVEL         = 31

.ifdef IIGS
serial_status_reg = zilog_status_reg_r
serial_data_reg   = zilog_data_reg_r
HAS_BYTE          = $01
.else
serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
HAS_BYTE          = $08
.endif

INV_SPC      := ' '
SPC          := ' '|$80
SPKR         := $C030

ser_data     := $FFFF           ; Placeholders for legibility, going to be patched
ser_status   := $FFFF
txt_level    := $FFFF
