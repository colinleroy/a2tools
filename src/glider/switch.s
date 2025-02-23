         .export _switch
         .export _switch_mask
         .include "switch.inc"

         .rodata
switch_x0:
         .byte $00, $00
         .byte $3E, $00
         .byte $3E, $00
         .byte $3E, $00
         .byte $3E, $00
         .byte $36, $00
         .byte $36, $00
         .byte $36, $00
         .byte $3E, $00
         .byte $00, $00
switch_mask_x0:
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
         .byte $00, $7F
switch_x1:
         .byte $00, $00
         .byte $7C, $00
         .byte $7C, $00
         .byte $7C, $00
         .byte $7C, $00
         .byte $6C, $00
         .byte $6C, $00
         .byte $6C, $00
         .byte $7C, $00
         .byte $00, $00
switch_mask_x1:
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
         .byte $01, $7E
switch_x2:
         .byte $00, $00
         .byte $78, $01
         .byte $78, $01
         .byte $78, $01
         .byte $78, $01
         .byte $58, $01
         .byte $58, $01
         .byte $58, $01
         .byte $78, $01
         .byte $00, $00
switch_mask_x2:
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
         .byte $03, $7C
switch_x3:
         .byte $00, $00
         .byte $70, $03
         .byte $70, $03
         .byte $70, $03
         .byte $70, $03
         .byte $30, $03
         .byte $30, $03
         .byte $30, $03
         .byte $70, $03
         .byte $00, $00
switch_mask_x3:
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
         .byte $07, $78
switch_x4:
         .byte $00, $00
         .byte $60, $07
         .byte $60, $07
         .byte $60, $07
         .byte $60, $07
         .byte $60, $06
         .byte $60, $06
         .byte $60, $06
         .byte $60, $07
         .byte $00, $00
switch_mask_x4:
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
         .byte $0F, $70
switch_x5:
         .byte $00, $00
         .byte $40, $0F
         .byte $40, $0F
         .byte $40, $0F
         .byte $40, $0F
         .byte $40, $0D
         .byte $40, $0D
         .byte $40, $0D
         .byte $40, $0F
         .byte $00, $00
switch_mask_x5:
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
         .byte $1F, $60
switch_x6:
         .byte $00, $00
         .byte $00, $1F
         .byte $00, $1F
         .byte $00, $1F
         .byte $00, $1F
         .byte $00, $1B
         .byte $00, $1B
         .byte $00, $1B
         .byte $00, $1F
         .byte $00, $00
switch_mask_x6:
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
         .byte $3F, $40
_switch:
         .addr switch_x0
         .addr switch_x1
         .addr switch_x2
         .addr switch_x3
         .addr switch_x4
         .addr switch_x5
         .addr switch_x6
_switch_mask:
         .addr switch_mask_x0
         .addr switch_mask_x1
         .addr switch_mask_x2
         .addr switch_mask_x3
         .addr switch_mask_x4
         .addr switch_mask_x5
         .addr switch_mask_x6
