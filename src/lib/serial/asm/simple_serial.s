;
; Copyright (C) 2022-2024 Colin Leroy-Mira <colin@colino.net>
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

        .export         _baudrate, _flow_control, _open_slot

        .export         _simple_serial_open
        .export         _simple_serial_open_printer
        .export         _simple_serial_close

.ifdef SERIAL_ENABLE_IRQ
        .export         _simple_serial_getc_with_timeout
        .export         _simple_serial_getc
.endif

        .import         sser_c

        .import         _ser_params
        .import         _simple_serial_read_config
        .import         _simple_serial_setup_no_irq_regs

        .import         __filetype, __auxtype
        .import         _open, _read, _write, _close, _unlink
        .import         pusha, pusha0, pushax, return0, returnFFFF
        
        .import         _serial_open, _serial_close
        .import         _serial_get_async

        .importzp       ptr1

        .import         _get_iigs_speed
        .import         _set_iigs_speed

        .include        "../../simple_serial.inc"
        .include        "fcntl.inc"
        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"
        .include        "accelerator.inc"

        .segment "DATA"

_baudrate:      .byte $00
_flow_control:  .byte SER_HS_HW
_open_slot:     .byte 0

        .segment "RT_ONCE"

;char __fastcall__ simple_serial_open_printer(void);
.proc _simple_serial_open_printer: near
        jsr     _simple_serial_read_config
        ldy     #SIMPLE_SERIAL_PARAMS::PRINTER_BAUDRATE
        jmp     simple_serial_open_direct
.endproc

;char __fastcall__ simple_serial_open(void);
.proc _simple_serial_open: near
        ; Get options
        jsr     _simple_serial_read_config
        ; Get speed
        ldy     #SIMPLE_SERIAL_PARAMS::DATA_BAUDRATE
        jmp     simple_serial_open_direct
.endproc

.proc simple_serial_open_direct: near
        ; Store speed
        lda     _baudrate
        bne     :+
        lda     _ser_params,y
        sta     _baudrate
:
        ; get and store slot
        iny
        lda     _ser_params,y
        sta     _open_slot

        ldx     _baudrate

        ; open port
        jsr     _serial_open
        cmp     #$00
        bne     @simple_serial_open_slot_err

        jmp     return0
@simple_serial_open_slot_err:
        rts
.endproc

;char __fastcall__ simple_serial_close(void);
.proc _simple_serial_close: near
        lda     #$00
        sta     _baudrate
        sta     _open_slot
        jmp     _serial_close
.endproc

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

.ifdef SERIAL_ENABLE_IRQ

;char __fastcall__ simple_serial_getc(void) {
.proc _simple_serial_getc: near
        jsr     _simple_serial_getc_with_timeout
        cpx     #$FF
        beq     _simple_serial_getc
        rts
.endproc

;char __fastcall__ simple_serial_getc_with_timeout(void) {
.proc _simple_serial_getc_with_timeout: near
        ; Init cycle counter
        lda     #<10000
        sta     ser_timeout_cycles
        ldx     #>10000
        stx     ser_timeout_cycles+1

        ; Init sser_c
        lda     #$00
        sta     sser_c
        sta     sser_c+1

        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     orig_speed_reg
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed

@getc_try:
        lda     #$00
        sta     sser_c+1
        ; Try to get char (into sser_c)
        jsr     _serial_get_async
        bcc     @getc_out

        ; Decrement timeout counter
        lda     ser_timeout_cycles
        bne     :+
        dec     ser_timeout_cycles+1
:       dec     ser_timeout_cycles
        bne     @getc_try
        lda     ser_timeout_cycles+1
        bne     @getc_try

        ; We got no data
        lda     #$FF
        sta     sser_c+1

        ; Done
@getc_out:
        sta     sser_c
        lda     orig_speed_reg
        jsr     _set_iigs_speed

        lda     sser_c
        ldx     sser_c+1
        rts
.endproc

.endif

        .bss

ser_timeout_cycles: .res 2
orig_speed_reg:     .res 1
