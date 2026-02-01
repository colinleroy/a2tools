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
        .export         _smartport_unit_size, _smartport_unit_name

        .import         return0, return1, pusha, popa
        .importzp       ptr1, sreg, tmp1


PARAM_COUNT =0
UNIT_NUMBER =1
PARAM_BUFFER=2
STATUS_CODE =4

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
        jsr       pusha       ; StatusCode
        lda       #<ptr1      ; Use ptr1-4 as 8 bytes buffer
        ldx       #>ptr1
        jsr       _smartport_get_status
        bcc       ok
error:  lda       #$00
        beq       done

ok:     lda       ptr1        ; Get number of units
done:   ldx       #>$0000
        rts
.endproc

; params: response_buffer in AX,
;         normal/DIB in TOS
;         unit number after,
;         slot after it
; returns smartport error code (0 on success)
.proc _smartport_get_status
        sta       smartport_params+PARAM_BUFFER
        stx       smartport_params+PARAM_BUFFER+1
        lda       #$03
        sta       smartport_params+PARAM_COUNT
        jsr       popa
        sta       smartport_params+STATUS_CODE
        jsr       popa
        sta       smartport_params+UNIT_NUMBER
        jsr       popa
        jsr       smartport_dispatch
command:.byte     $00
        .word     smartport_params

        ldx       #>$0000
        rts
.endproc

.proc _smartport_unit_size
        sta       ptr1        ; Store buffer
        stx       ptr1+1

        ; The response gives the unit's number of blocks, we'll
        ; return bytes (bytes = blocks<<9). Get the three bytes
        ; into sreg+1/sreg/x so that we can shift one instead of nine.
        ldy       #STATUS_DIB_BLOCK_HI
        lda       (ptr1),y
        sta       sreg+1
        dey
        lda       (ptr1),y
        sta       sreg
        dey
        lda       (ptr1),y
        asl                   ; Shift low byte,
        tax                   ; move to X
        rol       sreg        ; shift higher bytes,
        rol       sreg+1
        lda       #$00        ; and zero A
        rts
.endproc

.proc _smartport_unit_name
        sta       ptr1        ; Store buffer
        stx       ptr1+1

        ; Shortcut: overwrite the 16th character of the unit name
        ; with terminator, in order to avoid having to copy the data.
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

.segment "BSS"
smartport_params:         .res 9
smartport_buffer:         .res 9
