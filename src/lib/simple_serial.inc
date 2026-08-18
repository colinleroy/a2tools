.struct SIMPLE_SERIAL_PARAMS
  DATA_BAUDRATE    .byte
  DATA_SLOT        .byte
  PRINTER_BAUDRATE .byte
  PRINTER_SLOT     .byte
  .ifdef EXTRA_SERIAL_CONFIG
  EXTRA_PARAMETERS .byte 16
  .endif
.endstruct
