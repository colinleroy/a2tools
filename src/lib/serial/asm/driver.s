; Copyright (C) 2025 Colin Leroy-Mira <colin@colino.net>
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

; ----------
; Serial

; Core functions
        .export     _serial_open, _serial_close
        .export     _serial_putc_direct, _serial_read_byte_direct
        .export     _serial_read_byte_no_irq, serial_read_byte_no_irq_timeout

        .import     _acia_open, _acia_close
        .import     _acia_read_byte_sync, _acia_put
        .import     _z8530_open, _z8530_close
        .import     _z8530_read_byte_sync, _z8530_put

; IRQ functions
.ifdef SERIAL_ENABLE_IRQ
        .export     _serial_get_async
        .export     _simple_serial_set_irq
        .export     _ser_irq_buf, _ser_irq_widx, _ser_irq_ridx

        .import     _acia_irq, _acia_set_irq
        .import     _z8530_irq, _z8530_set_irq

        .interruptor serial_irq, 29
.endif

        .export     _ser_data_reg, _ser_status_reg

; Low-level control
.ifdef SERIAL_LOW_LEVEL_CONTROL
        .export     _simple_serial_set_speed
        .export     _simple_serial_slot_dtr_onoff
        .export     _simple_serial_dtr_onoff
        .export     _simple_serial_set_parity

        .import     _acia_set_speed
        .import     _acia_slot_dtr_onoff
        .import     _acia_set_parity

        .import     _z8530_set_speed
        .import     _z8530_slot_dtr_onoff
        .import     _z8530_set_parity
.endif

        .import     ostype, _open_slot, pusha

        .constructor setup_defaults
        .destructor  _serial_close

        .include    "apple2.inc"
        .include    "ser-kernel.inc"
        .include    "ser-error.inc"

.segment "ONCE"

.proc setup_defaults
        bit     ostype
        bpl     :+

        ; Patch defaults and callbacks for
        ; IIgs z8530 integrated serial ports
        lda     #0
        sta     serial_slot

        lda     #<_z8530_open
        ldx     #>_z8530_open
        sta     _serial_open+1
        stx     _serial_open+2

        lda     #<_z8530_close
        ldx     #>_z8530_close
        sta     _serial_close+1
        stx     _serial_close+2

        lda     #<_z8530_put
        ldx     #>_z8530_put
        sta     _serial_putc_direct+1
        stx     _serial_putc_direct+2

        lda     #<_z8530_read_byte_sync
        ldx     #>_z8530_read_byte_sync
        sta     _serial_read_byte_direct+1
        stx     _serial_read_byte_direct+2

.ifdef SERIAL_ENABLE_IRQ
        lda     #<_z8530_irq
        ldx     #>_z8530_irq
        sta     serial_handle_irq+1
        stx     serial_handle_irq+2

        lda     #<_z8530_set_irq
        ldx     #>_z8530_set_irq
        sta     _simple_serial_set_irq+1
        stx     _simple_serial_set_irq+2
.endif

.ifdef SERIAL_LOW_LEVEL_CONTROL
        lda     #<_z8530_slot_dtr_onoff
        ldx     #>_z8530_slot_dtr_onoff
        sta     _simple_serial_slot_dtr_onoff+1
        stx     _simple_serial_slot_dtr_onoff+2

        lda     #<_z8530_set_speed
        ldx     #>_z8530_set_speed
        sta     _simple_serial_set_speed+1
        stx     _simple_serial_set_speed+2

        lda     #<_z8530_set_parity
        ldx     #>_z8530_set_parity
        sta     _simple_serial_set_parity+1
        stx     _simple_serial_set_parity+2
.endif

:       rts
.endproc

.segment "CODE"

.proc _serial_open
        jsr     _acia_open
        cmp     #SER_ERR_OK
        bne     :+
        ldy     #$01
        sty     connected
:       rts
.endproc

.proc _serial_close
        jmp     _acia_close
.endproc

; Send character in A over serial. Destroys X.
; Does not touch Y.
.proc _serial_putc_direct
        jmp     _acia_put
.endproc

; Returns with char in A and carry clear if
; character available. Destroys X.
; Does not touch Y.
.proc _serial_read_byte_direct
        jmp     _acia_read_byte_sync
.endproc


.ifdef SERIAL_ENABLE_IRQ

.proc serial_handle_irq
        jmp     _acia_irq
.endproc

.proc _simple_serial_set_irq
        jmp     _acia_set_irq
.endproc

.proc serial_irq
        lda     connected
        beq     :+

        jmp     serial_handle_irq
:       clc
        rts
.endproc

.proc _serial_get_async
        ldx     _ser_irq_ridx
        cpx     _ser_irq_widx
        beq     no
        lda     _ser_irq_buf,x
        inc     _ser_irq_ridx
        clc
        rts

no:     sec
        rts
.endproc
.endif ;.ifdef SERIAL_ENABLE_IRQ

_serial_read_byte_no_irq:
:       jsr     _serial_read_byte_direct
        bcs     :-
        rts

serial_read_byte_no_irq_timeout:
        lda     #$00
        sta     timeout_cnt
        sta     timeout_cnt+1
        jsr     _serial_read_byte_direct
        bcc     out

        inc     timeout_cnt
        bne     :-
        inc     timeout_cnt+1
        bne     :-

out:    rts

.ifdef SERIAL_LOW_LEVEL_CONTROL
_simple_serial_dtr_onoff:
        tay
        lda     _open_slot
        jsr     pusha
        tya
_simple_serial_slot_dtr_onoff:
        jmp     _acia_slot_dtr_onoff

_simple_serial_set_speed:
        jmp     _acia_set_speed

_simple_serial_set_parity:
        jmp     _acia_set_parity
.endif ;.ifdef SERIAL_LOW_LEVEL_CONTROL

timeout_cnt:      .byte   2
serial_slot:      .byte   2
connected:        .byte   0

.bss

.ifdef SERIAL_ENABLE_IRQ
_ser_irq_buf:     .res    256
_ser_irq_widx:    .byte   0
_ser_irq_ridx:    .byte   0
.endif

_ser_data_reg:    .word   0
_ser_status_reg:  .word   0
