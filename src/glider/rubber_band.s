         .export _rubber_band
         .export _rubber_band_mask
         .include "rubber_band.inc"

         .rodata
rubber_band_x0:
         .byte $00, $00
         .byte $0E, $00
         .byte $38, $00
         .byte $00, $00
rubber_band_mask_x0:
         .byte $71, $7F
         .byte $40, $7F
         .byte $01, $7F
         .byte $47, $7F
rubber_band_x1:
         .byte $00, $00
         .byte $1C, $00
         .byte $70, $00
         .byte $00, $00
rubber_band_mask_x1:
         .byte $63, $7F
         .byte $01, $7F
         .byte $03, $7E
         .byte $0F, $7F
rubber_band_x2:
         .byte $00, $00
         .byte $38, $00
         .byte $60, $01
         .byte $00, $00
rubber_band_mask_x2:
         .byte $47, $7F
         .byte $03, $7E
         .byte $07, $7C
         .byte $1F, $7E
rubber_band_x3:
         .byte $00, $00
         .byte $70, $00
         .byte $40, $03
         .byte $00, $00
rubber_band_mask_x3:
         .byte $0F, $7F
         .byte $07, $7C
         .byte $0F, $78
         .byte $3F, $7C
rubber_band_x4:
         .byte $00, $00
         .byte $60, $01
         .byte $00, $07
         .byte $00, $00
rubber_band_mask_x4:
         .byte $1F, $7E
         .byte $0F, $78
         .byte $1F, $70
         .byte $7F, $78
rubber_band_x5:
         .byte $00, $00
         .byte $40, $03
         .byte $00, $0E
         .byte $00, $00
rubber_band_mask_x5:
         .byte $3F, $7C
         .byte $1F, $70
         .byte $3F, $60
         .byte $7F, $71
rubber_band_x6:
         .byte $00, $00
         .byte $00, $07
         .byte $00, $1C
         .byte $00, $00
rubber_band_mask_x6:
         .byte $7F, $78
         .byte $3F, $60
         .byte $7F, $40
         .byte $7F, $63
_rubber_band:
         .addr rubber_band_x0
         .addr rubber_band_x1
         .addr rubber_band_x2
         .addr rubber_band_x3
         .addr rubber_band_x4
         .addr rubber_band_x5
         .addr rubber_band_x6
_rubber_band_mask:
         .addr rubber_band_mask_x0
         .addr rubber_band_mask_x1
         .addr rubber_band_mask_x2
         .addr rubber_band_mask_x3
         .addr rubber_band_mask_x4
         .addr rubber_band_mask_x5
         .addr rubber_band_mask_x6
