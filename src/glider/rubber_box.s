         .export _rubber_box
         .export _rubber_box_mask
         .include "rubber_box.inc"

         .rodata
rubber_box_x0:
         .byte $00, $00, $00
         .byte $7E, $1F, $00
         .byte $2A, $1A, $00
         .byte $32, $12, $00
         .byte $2A, $1F, $00
         .byte $72, $1F, $00
         .byte $1E, $1E, $00
         .byte $46, $18, $00
         .byte $2A, $15, $00
         .byte $54, $0A, $00
         .byte $08, $0C, $00
         .byte $70, $03, $00
         .byte $00, $00, $00
rubber_box_mask_x0:
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $00, $40, $7F
         .byte $01, $60, $7F
         .byte $03, $70, $7F
         .byte $07, $78, $7F
rubber_box_x1:
         .byte $00, $00, $00
         .byte $7C, $3F, $00
         .byte $54, $34, $00
         .byte $64, $24, $00
         .byte $54, $3E, $00
         .byte $64, $3F, $00
         .byte $3C, $3C, $00
         .byte $0C, $31, $00
         .byte $54, $2A, $00
         .byte $28, $15, $00
         .byte $10, $18, $00
         .byte $60, $07, $00
         .byte $00, $00, $00
rubber_box_mask_x1:
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $01, $00, $7F
         .byte $03, $40, $7F
         .byte $07, $60, $7F
         .byte $0F, $70, $7F
rubber_box_x2:
         .byte $00, $00, $00
         .byte $78, $7F, $00
         .byte $28, $69, $00
         .byte $48, $49, $00
         .byte $28, $7D, $00
         .byte $48, $7F, $00
         .byte $78, $78, $00
         .byte $18, $62, $00
         .byte $28, $55, $00
         .byte $50, $2A, $00
         .byte $20, $30, $00
         .byte $40, $0F, $00
         .byte $00, $00, $00
rubber_box_mask_x2:
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $03, $00, $7E
         .byte $07, $00, $7F
         .byte $0F, $40, $7F
         .byte $1F, $60, $7F
rubber_box_x3:
         .byte $00, $00, $00
         .byte $70, $7F, $01
         .byte $50, $52, $01
         .byte $10, $13, $01
         .byte $50, $7A, $01
         .byte $10, $7F, $01
         .byte $70, $71, $01
         .byte $30, $44, $01
         .byte $50, $2A, $01
         .byte $20, $55, $00
         .byte $40, $60, $00
         .byte $00, $1F, $00
         .byte $00, $00, $00
rubber_box_mask_x3:
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $07, $00, $7C
         .byte $0F, $00, $7E
         .byte $1F, $00, $7F
         .byte $3F, $40, $7F
rubber_box_x4:
         .byte $00, $00, $00
         .byte $60, $7F, $03
         .byte $20, $25, $03
         .byte $20, $26, $02
         .byte $20, $75, $03
         .byte $20, $7E, $03
         .byte $60, $63, $03
         .byte $60, $08, $03
         .byte $20, $55, $02
         .byte $40, $2A, $01
         .byte $00, $41, $01
         .byte $00, $3E, $00
         .byte $00, $00, $00
rubber_box_mask_x4:
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $0F, $00, $78
         .byte $1F, $00, $7C
         .byte $3F, $00, $7E
         .byte $7F, $00, $7F
rubber_box_x5:
         .byte $00, $00, $00
         .byte $40, $7F, $07
         .byte $40, $4A, $06
         .byte $40, $4C, $04
         .byte $40, $6A, $07
         .byte $40, $7C, $07
         .byte $40, $47, $07
         .byte $40, $11, $06
         .byte $40, $2A, $05
         .byte $00, $55, $02
         .byte $00, $02, $03
         .byte $00, $7C, $00
         .byte $00, $00, $00
rubber_box_mask_x5:
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $1F, $00, $70
         .byte $3F, $00, $78
         .byte $7F, $00, $7C
         .byte $7F, $01, $7E
rubber_box_x6:
         .byte $00, $00, $00
         .byte $00, $7F, $0F
         .byte $00, $15, $0D
         .byte $00, $19, $09
         .byte $00, $55, $0F
         .byte $00, $79, $0F
         .byte $00, $0F, $0F
         .byte $00, $23, $0C
         .byte $00, $55, $0A
         .byte $00, $2A, $05
         .byte $00, $04, $06
         .byte $00, $78, $01
         .byte $00, $00, $00
rubber_box_mask_x6:
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $3F, $00, $60
         .byte $7F, $00, $70
         .byte $7F, $01, $78
         .byte $7F, $03, $7C
_rubber_box:
         .addr rubber_box_x0
         .addr rubber_box_x1
         .addr rubber_box_x2
         .addr rubber_box_x3
         .addr rubber_box_x4
         .addr rubber_box_x5
         .addr rubber_box_x6
_rubber_box_mask:
         .addr rubber_box_mask_x0
         .addr rubber_box_mask_x1
         .addr rubber_box_mask_x2
         .addr rubber_box_mask_x3
         .addr rubber_box_mask_x4
         .addr rubber_box_mask_x5
         .addr rubber_box_mask_x6
