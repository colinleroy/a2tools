         .export _balloon
         .export _balloon_mask
         .include "balloon.inc"

         .rodata
balloon_x0:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $08, $00, $00
         .byte $00, $1C, $00, $00
         .byte $00, $3E, $00, $00
         .byte $00, $7F, $00, $00
         .byte $40, $7F, $01, $00
         .byte $40, $7F, $01, $00
         .byte $60, $7F, $03, $00
         .byte $60, $7F, $03, $00
         .byte $70, $7F, $06, $00
         .byte $70, $7F, $05, $00
         .byte $78, $7F, $0D, $00
         .byte $78, $7F, $0D, $00
         .byte $78, $7F, $0F, $00
         .byte $70, $7F, $07, $00
         .byte $70, $7F, $07, $00
         .byte $60, $7F, $03, $00
         .byte $00, $7F, $00, $00
         .byte $00, $1C, $00, $00
         .byte $00, $00, $00, $00
balloon_mask_x0:
         .byte $7F, $6B, $7F, $7F
         .byte $7F, $77, $7F, $7F
         .byte $7F, $63, $7F, $7F
         .byte $7F, $41, $7F, $7F
         .byte $7F, $00, $7F, $7F
         .byte $3F, $00, $7E, $7F
         .byte $1F, $00, $7C, $7F
         .byte $1F, $00, $7C, $7F
         .byte $0F, $00, $78, $7F
         .byte $0F, $00, $78, $7F
         .byte $07, $00, $70, $7F
         .byte $07, $00, $70, $7F
         .byte $03, $00, $60, $7F
         .byte $03, $00, $60, $7F
         .byte $03, $00, $60, $7F
         .byte $07, $00, $70, $7F
         .byte $07, $00, $70, $7F
         .byte $0F, $00, $78, $7F
         .byte $1F, $00, $7C, $7F
         .byte $7F, $00, $7F, $7F
         .byte $7F, $63, $7F, $7F
balloon_x1:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $10, $00, $00
         .byte $00, $38, $00, $00
         .byte $00, $7C, $00, $00
         .byte $00, $7E, $01, $00
         .byte $00, $7F, $03, $00
         .byte $00, $7F, $03, $00
         .byte $40, $7F, $07, $00
         .byte $40, $7F, $07, $00
         .byte $60, $7F, $0D, $00
         .byte $60, $7F, $0B, $00
         .byte $70, $7F, $1B, $00
         .byte $70, $7F, $1B, $00
         .byte $70, $7F, $1F, $00
         .byte $60, $7F, $0F, $00
         .byte $60, $7F, $0F, $00
         .byte $40, $7F, $07, $00
         .byte $00, $7E, $01, $00
         .byte $00, $38, $00, $00
         .byte $00, $00, $00, $00
balloon_mask_x1:
         .byte $7F, $57, $7F, $7F
         .byte $7F, $6F, $7F, $7F
         .byte $7F, $47, $7F, $7F
         .byte $7F, $03, $7F, $7F
         .byte $7F, $01, $7E, $7F
         .byte $7F, $00, $7C, $7F
         .byte $3F, $00, $78, $7F
         .byte $3F, $00, $78, $7F
         .byte $1F, $00, $70, $7F
         .byte $1F, $00, $70, $7F
         .byte $0F, $00, $60, $7F
         .byte $0F, $00, $60, $7F
         .byte $07, $00, $40, $7F
         .byte $07, $00, $40, $7F
         .byte $07, $00, $40, $7F
         .byte $0F, $00, $60, $7F
         .byte $0F, $00, $60, $7F
         .byte $1F, $00, $70, $7F
         .byte $3F, $00, $78, $7F
         .byte $7F, $01, $7E, $7F
         .byte $7F, $47, $7F, $7F
balloon_x2:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $20, $00, $00
         .byte $00, $70, $00, $00
         .byte $00, $78, $01, $00
         .byte $00, $7C, $03, $00
         .byte $00, $7E, $07, $00
         .byte $00, $7E, $07, $00
         .byte $00, $7F, $0F, $00
         .byte $00, $7F, $0F, $00
         .byte $40, $7F, $1B, $00
         .byte $40, $7F, $17, $00
         .byte $60, $7F, $37, $00
         .byte $60, $7F, $37, $00
         .byte $60, $7F, $3F, $00
         .byte $40, $7F, $1F, $00
         .byte $40, $7F, $1F, $00
         .byte $00, $7F, $0F, $00
         .byte $00, $7C, $03, $00
         .byte $00, $70, $00, $00
         .byte $00, $00, $00, $00
balloon_mask_x2:
         .byte $7F, $2F, $7F, $7F
         .byte $7F, $5F, $7F, $7F
         .byte $7F, $0F, $7F, $7F
         .byte $7F, $07, $7E, $7F
         .byte $7F, $03, $7C, $7F
         .byte $7F, $01, $78, $7F
         .byte $7F, $00, $70, $7F
         .byte $7F, $00, $70, $7F
         .byte $3F, $00, $60, $7F
         .byte $3F, $00, $60, $7F
         .byte $1F, $00, $40, $7F
         .byte $1F, $00, $40, $7F
         .byte $0F, $00, $00, $7F
         .byte $0F, $00, $00, $7F
         .byte $0F, $00, $00, $7F
         .byte $1F, $00, $40, $7F
         .byte $1F, $00, $40, $7F
         .byte $3F, $00, $60, $7F
         .byte $7F, $00, $70, $7F
         .byte $7F, $03, $7C, $7F
         .byte $7F, $0F, $7F, $7F
balloon_x3:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $40, $00, $00
         .byte $00, $60, $01, $00
         .byte $00, $70, $03, $00
         .byte $00, $78, $07, $00
         .byte $00, $7C, $0F, $00
         .byte $00, $7C, $0F, $00
         .byte $00, $7E, $1F, $00
         .byte $00, $7E, $1F, $00
         .byte $00, $7F, $37, $00
         .byte $00, $7F, $2F, $00
         .byte $40, $7F, $6F, $00
         .byte $40, $7F, $6F, $00
         .byte $40, $7F, $7F, $00
         .byte $00, $7F, $3F, $00
         .byte $00, $7F, $3F, $00
         .byte $00, $7E, $1F, $00
         .byte $00, $78, $07, $00
         .byte $00, $60, $01, $00
         .byte $00, $00, $00, $00
balloon_mask_x3:
         .byte $7F, $5F, $7E, $7F
         .byte $7F, $3F, $7F, $7F
         .byte $7F, $1F, $7E, $7F
         .byte $7F, $0F, $7C, $7F
         .byte $7F, $07, $78, $7F
         .byte $7F, $03, $70, $7F
         .byte $7F, $01, $60, $7F
         .byte $7F, $01, $60, $7F
         .byte $7F, $00, $40, $7F
         .byte $7F, $00, $40, $7F
         .byte $3F, $00, $00, $7F
         .byte $3F, $00, $00, $7F
         .byte $1F, $00, $00, $7E
         .byte $1F, $00, $00, $7E
         .byte $1F, $00, $00, $7E
         .byte $3F, $00, $00, $7F
         .byte $3F, $00, $00, $7F
         .byte $7F, $00, $40, $7F
         .byte $7F, $01, $60, $7F
         .byte $7F, $07, $78, $7F
         .byte $7F, $1F, $7E, $7F
balloon_x4:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $00, $01, $00
         .byte $00, $40, $03, $00
         .byte $00, $60, $07, $00
         .byte $00, $70, $0F, $00
         .byte $00, $78, $1F, $00
         .byte $00, $78, $1F, $00
         .byte $00, $7C, $3F, $00
         .byte $00, $7C, $3F, $00
         .byte $00, $7E, $6F, $00
         .byte $00, $7E, $5F, $00
         .byte $00, $7F, $5F, $01
         .byte $00, $7F, $5F, $01
         .byte $00, $7F, $7F, $01
         .byte $00, $7E, $7F, $00
         .byte $00, $7E, $7F, $00
         .byte $00, $7C, $3F, $00
         .byte $00, $70, $0F, $00
         .byte $00, $40, $03, $00
         .byte $00, $00, $00, $00
balloon_mask_x4:
         .byte $7F, $3F, $7D, $7F
         .byte $7F, $7F, $7E, $7F
         .byte $7F, $3F, $7C, $7F
         .byte $7F, $1F, $78, $7F
         .byte $7F, $0F, $70, $7F
         .byte $7F, $07, $60, $7F
         .byte $7F, $03, $40, $7F
         .byte $7F, $03, $40, $7F
         .byte $7F, $01, $00, $7F
         .byte $7F, $01, $00, $7F
         .byte $7F, $00, $00, $7E
         .byte $7F, $00, $00, $7E
         .byte $3F, $00, $00, $7C
         .byte $3F, $00, $00, $7C
         .byte $3F, $00, $00, $7C
         .byte $7F, $00, $00, $7E
         .byte $7F, $00, $00, $7E
         .byte $7F, $01, $00, $7F
         .byte $7F, $03, $40, $7F
         .byte $7F, $0F, $70, $7F
         .byte $7F, $3F, $7C, $7F
balloon_x5:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $00, $02, $00
         .byte $00, $00, $07, $00
         .byte $00, $40, $0F, $00
         .byte $00, $60, $1F, $00
         .byte $00, $70, $3F, $00
         .byte $00, $70, $3F, $00
         .byte $00, $78, $7F, $00
         .byte $00, $78, $7F, $00
         .byte $00, $7C, $5F, $01
         .byte $00, $7C, $3F, $01
         .byte $00, $7E, $3F, $03
         .byte $00, $7E, $3F, $03
         .byte $00, $7E, $7F, $03
         .byte $00, $7C, $7F, $01
         .byte $00, $7C, $7F, $01
         .byte $00, $78, $7F, $00
         .byte $00, $60, $1F, $00
         .byte $00, $00, $07, $00
         .byte $00, $00, $00, $00
balloon_mask_x5:
         .byte $7F, $7F, $7A, $7F
         .byte $7F, $7F, $7D, $7F
         .byte $7F, $7F, $78, $7F
         .byte $7F, $3F, $70, $7F
         .byte $7F, $1F, $60, $7F
         .byte $7F, $0F, $40, $7F
         .byte $7F, $07, $00, $7F
         .byte $7F, $07, $00, $7F
         .byte $7F, $03, $00, $7E
         .byte $7F, $03, $00, $7E
         .byte $7F, $01, $00, $7C
         .byte $7F, $01, $00, $7C
         .byte $7F, $00, $00, $78
         .byte $7F, $00, $00, $78
         .byte $7F, $00, $00, $78
         .byte $7F, $01, $00, $7C
         .byte $7F, $01, $00, $7C
         .byte $7F, $03, $00, $7E
         .byte $7F, $07, $00, $7F
         .byte $7F, $1F, $60, $7F
         .byte $7F, $7F, $78, $7F
balloon_x6:
         .byte $00, $00, $00, $00
         .byte $00, $00, $00, $00
         .byte $00, $00, $04, $00
         .byte $00, $00, $0E, $00
         .byte $00, $00, $1F, $00
         .byte $00, $40, $3F, $00
         .byte $00, $60, $7F, $00
         .byte $00, $60, $7F, $00
         .byte $00, $70, $7F, $01
         .byte $00, $70, $7F, $01
         .byte $00, $78, $3F, $03
         .byte $00, $78, $7F, $02
         .byte $00, $7C, $7F, $06
         .byte $00, $7C, $7F, $06
         .byte $00, $7C, $7F, $07
         .byte $00, $78, $7F, $03
         .byte $00, $78, $7F, $03
         .byte $00, $70, $7F, $01
         .byte $00, $40, $3F, $00
         .byte $00, $00, $0E, $00
         .byte $00, $00, $00, $00
balloon_mask_x6:
         .byte $7F, $7F, $75, $7F
         .byte $7F, $7F, $7B, $7F
         .byte $7F, $7F, $71, $7F
         .byte $7F, $7F, $60, $7F
         .byte $7F, $3F, $40, $7F
         .byte $7F, $1F, $00, $7F
         .byte $7F, $0F, $00, $7E
         .byte $7F, $0F, $00, $7E
         .byte $7F, $07, $00, $7C
         .byte $7F, $07, $00, $7C
         .byte $7F, $03, $00, $78
         .byte $7F, $03, $00, $78
         .byte $7F, $01, $00, $70
         .byte $7F, $01, $00, $70
         .byte $7F, $01, $00, $70
         .byte $7F, $03, $00, $78
         .byte $7F, $03, $00, $78
         .byte $7F, $07, $00, $7C
         .byte $7F, $0F, $00, $7E
         .byte $7F, $3F, $40, $7F
         .byte $7F, $7F, $71, $7F
_balloon:
         .addr balloon_x0
         .addr balloon_x1
         .addr balloon_x2
         .addr balloon_x3
         .addr balloon_x4
         .addr balloon_x5
         .addr balloon_x6
_balloon_mask:
         .addr balloon_mask_x0
         .addr balloon_mask_x1
         .addr balloon_mask_x2
         .addr balloon_mask_x3
         .addr balloon_mask_x4
         .addr balloon_mask_x5
         .addr balloon_mask_x6
