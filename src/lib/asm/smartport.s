;
; Copyright (C) 2022-2026 Colin Leroy-Mira <colin@colino.net>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.
;

        .export         _is_slot_smartport, _smartport_get_status
        .export         _smartport_dev_size, _smartport_dev_name

        .import         return0, return1, popa
        .importzp       ptr1, sreg

UNIT_NUMBER_OFF=1
STATUS_CODE_OFF=4

STATUS_DIB_BLOCK_HI = 3
STATUS_DIB_NAME     = 5
STATUS_DIB_ENDNAME  = STATUS_DIB_NAME+16

; params: slot in A
.proc smartport_dispatch
        ora       #$C0
        sta       ptr1+1
        lda       #$03
        sta       ptr1
        ldy       #($C0FF-$C003)
        clc
        adc       (ptr1),y
        sta       ptr1

        jmp       (ptr1)
.endproc

; params: slot in A
; returns 1 if smartport, 0 otherwise
.proc _is_slot_smartport
        ora       #$C0
        sta       ptr1+1
        lda       #$00
        sta       ptr1

        ; Check zero bytes at offsets $03 and $07 while A == $00
        ldy       #$03
        cmp       (ptr1),y
        bne       no
        ldy       #$07
        cmp       (ptr1),y
        bne       no

        ; Should have $20 at $01
        ldy       #$01
        lda       #$20
        cmp       (ptr1),y
        bne       no

        ; Should have $03 at $05
        ldy       #$05
        lda       #$03
        cmp       (ptr1),y
        bne       no

        jmp       return1
no:     jmp       return0
.endproc

; params: normal/DIB in A
;         unit number
;         slot
; returns NULL on error, pointer to buffer on success
.proc _smartport_get_status
        sta       smartport_status_params+STATUS_CODE_OFF
        jsr       popa
        sta       smartport_status_params+UNIT_NUMBER_OFF
        jsr       popa
        jsr       smartport_dispatch
command:.byte     $00                   ; STATUS
        .word     smartport_status_params
        bcs       sp_error

        lda       #<sp_buffer
        ldx       #>sp_buffer
        rts
sp_error:
        jmp       return0
.endproc

.proc _smartport_dev_size
        sta       ptr1
        stx       ptr1+1

        ldy       #STATUS_DIB_BLOCK_HI
        lda       (ptr1),y
        sta       sreg+1
        dey
        lda       (ptr1),y
        sta       sreg
        dey
        lda       (ptr1),y
        asl
        tax
        rol       sreg
        rol       sreg+1
        lda       #$00
        rts
.endproc

.proc _smartport_dev_name
        sta       ptr1
        stx       ptr1+1

        ldy       #STATUS_DIB_ENDNAME
        lda       #$00
        sta       (ptr1),y

        ldx       ptr1+1
        lda       ptr1
        clc
        adc       #STATUS_DIB_NAME
        bcc       :+
        inx
:       rts
.endproc

.segment "DATA"
smartport_status_params:  .byte $03     ; CmdList Byte
                          .byte $01     ; UnitNumber
                          .addr sp_buffer
                          .byte $03     ; DIB

.segment "BSS"
sp_buffer: .res 512
