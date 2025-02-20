         .export _clock
         .export _clock_mask
         .include "clock.inc"

         .rodata
clock_x0:
         .byte $00, $00, $00
         .byte $60, $03, $00
         .byte $78, $0F, $00
         .byte $7C, $1F, $00
         .byte $7C, $1F, $00
         .byte $7E, $3F, $00
         .byte $7E, $3F, $00
         .byte $3E, $3E, $00
         .byte $5E, $3D, $00
         .byte $6C, $1B, $00
         .byte $7C, $17, $00
         .byte $78, $0F, $00
         .byte $60, $03, $00
         .byte $00, $00, $00
clock_mask_x0:
         .byte $1D, $5C, $7F
         .byte $03, $60, $7F
         .byte $03, $60, $7F
         .byte $01, $40, $7F
         .byte $01, $40, $7F
         .byte $00, $00, $7F
         .byte $00, $00, $7F
         .byte $00, $00, $7F
         .byte $00, $00, $7F
         .byte $01, $40, $7F
         .byte $01, $40, $7F
         .byte $03, $60, $7F
         .byte $04, $10, $7F
         .byte $1D, $5C, $7F
clock_x1:
         .byte $00, $00, $00
         .byte $40, $07, $00
         .byte $70, $1F, $00
         .byte $78, $3F, $00
         .byte $78, $3F, $00
         .byte $7C, $7F, $00
         .byte $7C, $7F, $00
         .byte $7C, $7C, $00
         .byte $3C, $7B, $00
         .byte $58, $37, $00
         .byte $78, $2F, $00
         .byte $70, $1F, $00
         .byte $40, $07, $00
         .byte $00, $00, $00
clock_mask_x1:
         .byte $3B, $38, $7F
         .byte $07, $40, $7F
         .byte $07, $40, $7F
         .byte $03, $00, $7F
         .byte $03, $00, $7F
         .byte $01, $00, $7E
         .byte $01, $00, $7E
         .byte $01, $00, $7E
         .byte $01, $00, $7E
         .byte $03, $00, $7F
         .byte $03, $00, $7F
         .byte $07, $40, $7F
         .byte $09, $20, $7E
         .byte $3B, $38, $7F
clock_x2:
         .byte $00, $00, $00
         .byte $00, $0F, $00
         .byte $60, $3F, $00
         .byte $70, $7F, $00
         .byte $70, $7F, $00
         .byte $78, $7F, $01
         .byte $78, $7F, $01
         .byte $78, $79, $01
         .byte $78, $76, $01
         .byte $30, $6F, $00
         .byte $70, $5F, $00
         .byte $60, $3F, $00
         .byte $00, $0F, $00
         .byte $00, $00, $00
clock_mask_x2:
         .byte $77, $70, $7E
         .byte $0F, $00, $7F
         .byte $0F, $00, $7F
         .byte $07, $00, $7E
         .byte $07, $00, $7E
         .byte $03, $00, $7C
         .byte $03, $00, $7C
         .byte $03, $00, $7C
         .byte $03, $00, $7C
         .byte $07, $00, $7E
         .byte $07, $00, $7E
         .byte $0F, $00, $7F
         .byte $13, $40, $7C
         .byte $77, $70, $7E
clock_x3:
         .byte $00, $00, $00
         .byte $00, $1E, $00
         .byte $40, $7F, $00
         .byte $60, $7F, $01
         .byte $60, $7F, $01
         .byte $70, $7F, $03
         .byte $70, $7F, $03
         .byte $70, $73, $03
         .byte $70, $6D, $03
         .byte $60, $5E, $01
         .byte $60, $3F, $01
         .byte $40, $7F, $00
         .byte $00, $1E, $00
         .byte $00, $00, $00
clock_mask_x3:
         .byte $6F, $61, $7D
         .byte $1F, $00, $7E
         .byte $1F, $00, $7E
         .byte $0F, $00, $7C
         .byte $0F, $00, $7C
         .byte $07, $00, $78
         .byte $07, $00, $78
         .byte $07, $00, $78
         .byte $07, $00, $78
         .byte $0F, $00, $7C
         .byte $0F, $00, $7C
         .byte $1F, $00, $7E
         .byte $27, $00, $79
         .byte $6F, $61, $7D
clock_x4:
         .byte $00, $00, $00
         .byte $00, $3C, $00
         .byte $00, $7F, $01
         .byte $40, $7F, $03
         .byte $40, $7F, $03
         .byte $60, $7F, $07
         .byte $60, $7F, $07
         .byte $60, $67, $07
         .byte $60, $5B, $07
         .byte $40, $3D, $03
         .byte $40, $7F, $02
         .byte $00, $7F, $01
         .byte $00, $3C, $00
         .byte $00, $00, $00
clock_mask_x4:
         .byte $5F, $43, $7B
         .byte $3F, $00, $7C
         .byte $3F, $00, $7C
         .byte $1F, $00, $78
         .byte $1F, $00, $78
         .byte $0F, $00, $70
         .byte $0F, $00, $70
         .byte $0F, $00, $70
         .byte $0F, $00, $70
         .byte $1F, $00, $78
         .byte $1F, $00, $78
         .byte $3F, $00, $7C
         .byte $4F, $00, $72
         .byte $5F, $43, $7B
clock_x5:
         .byte $00, $00, $00
         .byte $00, $78, $00
         .byte $00, $7E, $03
         .byte $00, $7F, $07
         .byte $00, $7F, $07
         .byte $40, $7F, $0F
         .byte $40, $7F, $0F
         .byte $40, $4F, $0F
         .byte $40, $37, $0F
         .byte $00, $7B, $06
         .byte $00, $7F, $05
         .byte $00, $7E, $03
         .byte $00, $78, $00
         .byte $00, $00, $00
clock_mask_x5:
         .byte $3F, $07, $77
         .byte $7F, $00, $78
         .byte $7F, $00, $78
         .byte $3F, $00, $70
         .byte $3F, $00, $70
         .byte $1F, $00, $60
         .byte $1F, $00, $60
         .byte $1F, $00, $60
         .byte $1F, $00, $60
         .byte $3F, $00, $70
         .byte $3F, $00, $70
         .byte $7F, $00, $78
         .byte $1F, $01, $64
         .byte $3F, $07, $77
clock_x6:
         .byte $00, $00, $00
         .byte $00, $70, $01
         .byte $00, $7C, $07
         .byte $00, $7E, $0F
         .byte $00, $7E, $0F
         .byte $00, $7F, $1F
         .byte $00, $7F, $1F
         .byte $00, $1F, $1F
         .byte $00, $6F, $1E
         .byte $00, $76, $0D
         .byte $00, $7E, $0B
         .byte $00, $7C, $07
         .byte $00, $70, $01
         .byte $00, $00, $00
clock_mask_x6:
         .byte $7F, $0E, $6E
         .byte $7F, $01, $70
         .byte $7F, $01, $70
         .byte $7F, $00, $60
         .byte $7F, $00, $60
         .byte $3F, $00, $40
         .byte $3F, $00, $40
         .byte $3F, $00, $40
         .byte $3F, $00, $40
         .byte $7F, $00, $60
         .byte $7F, $00, $60
         .byte $7F, $01, $70
         .byte $3F, $02, $48
         .byte $7F, $0E, $6E
_clock:
         .addr clock_x0
         .addr clock_x1
         .addr clock_x2
         .addr clock_x3
         .addr clock_x4
         .addr clock_x5
         .addr clock_x6
_clock_mask:
         .addr clock_mask_x0
         .addr clock_mask_x1
         .addr clock_mask_x2
         .addr clock_mask_x3
         .addr clock_mask_x4
         .addr clock_mask_x5
         .addr clock_mask_x6
