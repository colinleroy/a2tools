.struct SIMPLE_SERIAL_PARAMS
  DATA_BAUDRATE    .byte
  DATA_SLOT        .byte
  PRINTER_BAUDRATE .byte
  PRINTER_SLOT     .byte
.endstruct

.ifdef IIGS
PRINTER_SER_SLOT = 1
MODEM_SER_SLOT   = 0
.else
PRINTER_SER_SLOT = 1
MODEM_SER_SLOT   = 2
.endif
