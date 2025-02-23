         .export _socket
         .export _socket_mask
         .include "socket.inc"

         .rodata
socket_x0:
         .byte $3E, $00
         .byte $41, $00
         .byte $55, $00
         .byte $41, $00
         .byte $3E, $00
         .byte $7F, $00
         .byte $3E, $00
         .byte $41, $00
         .byte $55, $00
         .byte $41, $00
         .byte $3E, $00
socket_mask_x0:
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
         .byte $00, $7F
socket_x1:
         .byte $7C, $00
         .byte $02, $01
         .byte $2A, $01
         .byte $02, $01
         .byte $7C, $00
         .byte $7E, $01
         .byte $7C, $00
         .byte $02, $01
         .byte $2A, $01
         .byte $02, $01
         .byte $7C, $00
socket_mask_x1:
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
         .byte $01, $7E
socket_x2:
         .byte $78, $01
         .byte $04, $02
         .byte $54, $02
         .byte $04, $02
         .byte $78, $01
         .byte $7C, $03
         .byte $78, $01
         .byte $04, $02
         .byte $54, $02
         .byte $04, $02
         .byte $78, $01
socket_mask_x2:
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
         .byte $03, $7C
socket_x3:
         .byte $70, $03
         .byte $08, $04
         .byte $28, $05
         .byte $08, $04
         .byte $70, $03
         .byte $78, $07
         .byte $70, $03
         .byte $08, $04
         .byte $28, $05
         .byte $08, $04
         .byte $70, $03
socket_mask_x3:
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
         .byte $07, $78
socket_x4:
         .byte $60, $07
         .byte $10, $08
         .byte $50, $0A
         .byte $10, $08
         .byte $60, $07
         .byte $70, $0F
         .byte $60, $07
         .byte $10, $08
         .byte $50, $0A
         .byte $10, $08
         .byte $60, $07
socket_mask_x4:
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
         .byte $0F, $70
socket_x5:
         .byte $40, $0F
         .byte $20, $10
         .byte $20, $15
         .byte $20, $10
         .byte $40, $0F
         .byte $60, $1F
         .byte $40, $0F
         .byte $20, $10
         .byte $20, $15
         .byte $20, $10
         .byte $40, $0F
socket_mask_x5:
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
         .byte $1F, $60
socket_x6:
         .byte $00, $1F
         .byte $40, $20
         .byte $40, $2A
         .byte $40, $20
         .byte $00, $1F
         .byte $40, $3F
         .byte $00, $1F
         .byte $40, $20
         .byte $40, $2A
         .byte $40, $20
         .byte $00, $1F
socket_mask_x6:
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
         .byte $3F, $40
_socket:
         .addr socket_x0
         .addr socket_x1
         .addr socket_x2
         .addr socket_x3
         .addr socket_x4
         .addr socket_x5
         .addr socket_x6
_socket_mask:
         .addr socket_mask_x0
         .addr socket_mask_x1
         .addr socket_mask_x2
         .addr socket_mask_x3
         .addr socket_mask_x4
         .addr socket_mask_x5
         .addr socket_mask_x6
