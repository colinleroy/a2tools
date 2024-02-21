.if (.cpu .bitand CPU_ISET_65C02)
ACIA           := $C088
.else
Offset          = $8F           ; Move 6502 false read out of I/O to page $BF
ACIA           := $C088-Offset
.endif

ACIA_DATA      := ACIA+0        ; Data register
ACIA_STATUS    := ACIA+1        ; Status register
ACIA_CMD       := ACIA+2        ; Command register
ACIA_CTRL      := ACIA+3        ; Control register
