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

        .export         _getfirstsmartportslot, _getnextsmartportslot
        .export         _smartportgetunitcount, _smartport_get_status
        .export         _smartport_dev_size, _smartport_dev_name

        .import         return0, return1, pusha, popa
        .importzp       ptr1, sreg, tmp1

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

.proc _getfirstsmartportslot
        lda       #$08
        ; Fallthrough
.endproc

; params: Last slot number in A
.proc _getnextsmartportslot
        pha                   ; Push for symmetry
next:   pla                   ; Pull last slot ID,
        sec                   ; decrement it,
        sbc       #$01
        pha                   ; and save it for next slot
        bmi       done        ; < 0, we're done

        ora       #$C0        ; Point to slot ROM
        sta       ptr1+1
        lda       #$00
        sta       ptr1

        ldx       #$03        ; Compare 4 magic bytes
        ldy       #$07        ; at offsets 7, 5, 3, 1
:       lda       (ptr1),y
        cmp       smartport_id_bytes,x
        bne       next        ; No match!
        dey
        dey
        dex
        bpl       :-

        ; If we arrive here, we have a match!

done:   pla                   ; Get saved slot ID (or $FF for INVALID_DEVICE)
        ldx       #>$0000
        rts
.endproc

; params: Slot number in A
.proc _smartportgetunitcount
        jsr       pusha       ; Push slot number
        lda       #$00
        jsr       pusha       ; Unit number
        jsr       _smartport_get_status
        bcs       done

        lda       sp_buffer+0 ; Get number of units
        ldx       #>$0000
done:   rts
.endproc

; params: normal/DIB in A
;         unit number in TOS
;         slot after it.
; returns NULL on error, pointer to buffer on success
.proc _smartport_get_status
        sta       smartport_status_params+STATUS_CODE_OFF
        jsr       popa
        sta       smartport_status_params+UNIT_NUMBER_OFF
        jsr       popa
        jsr       smartport_dispatch
command:.byte     $00
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

.segment "RODATA"
smartport_id_bytes:       .byte $20, $00, $03, $00

.segment "DATA"
smartport_status_params:  .byte $03     ; CmdList Byte
                          .byte $01     ; UnitNumber
                          .addr sp_buffer
                          .byte $03     ; DIB

.segment "BSS"
sp_buffer: .res 512
