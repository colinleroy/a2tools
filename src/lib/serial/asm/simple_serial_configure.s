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

        .export         _simple_serial_configure

        .import         _ser_params
        .import         _simple_serial_settings_io
        .import         _simple_serial_close

        .import         simple_serial_ram_settings
        .import         simple_serial_disk_settings
        .import         read_mode_str, write_mode_str

        .import         _set_scrollwindow
        .import         _clrscr, pusha, pushax, booleq
        .import         _cputs, _cgetc, _revers, _gotoxy

        .include        "../../simple_serial.inc"
        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"
        .include        "accelerator.inc"

        .segment "RT_ONCE"

.ifdef IIGS
baud_str_2400:   .asciiz "2400   "
baud_str_4800:   .asciiz "4800   "
baud_str_9600:   .asciiz "9600   "
baud_str_19200:  .asciiz "19200  "
baud_str_57600:  .asciiz "57600  "
baud_str_115200: .asciiz "115200 "
slot_str_0:      .asciiz "MODEM  "
slot_str_1:      .asciiz "PRINTER"

baud_rates:      .byte SER_BAUD_2400
                 .byte SER_BAUD_4800
                 .byte SER_BAUD_9600
                 .byte SER_BAUD_19200
                 .byte SER_BAUD_57600
                 .byte SER_BAUD_115200

baud_strs_low:   .byte <baud_str_2400
                 .byte <baud_str_4800
                 .byte <baud_str_9600
                 .byte <baud_str_19200
                 .byte <baud_str_57600
                 .byte <baud_str_115200

slots_strs_low:  .byte <slot_str_0
                 .byte <slot_str_1

baud_strs_high:  .byte >baud_str_2400
                 .byte >baud_str_4800
                 .byte >baud_str_9600
                 .byte >baud_str_19200
                 .byte >baud_str_57600
                 .byte >baud_str_115200

slots_strs_high: .byte >slot_str_0
                 .byte >slot_str_1

MAX_SLOT_IDX     := 1
MAX_SPEED_IDX    := 5

.else

baud_str_2400:   .asciiz "2400  "
baud_str_4800:   .asciiz "4800  "
baud_str_9600:   .asciiz "9600  "
baud_str_19200:  .asciiz "19200 "
baud_str_115200: .asciiz "115200"
slot_str_0:      .asciiz "0"
slot_str_1:      .asciiz "1"
slot_str_2:      .asciiz "2"
slot_str_3:      .asciiz "3"
slot_str_4:      .asciiz "4"
slot_str_5:      .asciiz "5"
slot_str_6:      .asciiz "6"
slot_str_7:      .asciiz "7"

baud_rates:      .byte SER_BAUD_2400
                 .byte SER_BAUD_4800
                 .byte SER_BAUD_9600
                 .byte SER_BAUD_19200
                 .byte SER_BAUD_115200

baud_strs_low:   .byte <baud_str_2400
                 .byte <baud_str_4800
                 .byte <baud_str_9600
                 .byte <baud_str_19200
                 .byte <baud_str_115200

slots_strs_low:  .byte <slot_str_0
                 .byte <slot_str_1
                 .byte <slot_str_2
                 .byte <slot_str_3
                 .byte <slot_str_4
                 .byte <slot_str_5
                 .byte <slot_str_6
                 .byte <slot_str_7

baud_strs_high:  .byte >baud_str_2400
                 .byte >baud_str_4800
                 .byte >baud_str_9600
                 .byte >baud_str_19200
                 .byte >baud_str_115200

slots_strs_high: .byte >slot_str_0
                 .byte >slot_str_1
                 .byte >slot_str_2
                 .byte >slot_str_3
                 .byte >slot_str_4
                 .byte >slot_str_5
                 .byte >slot_str_6
                 .byte >slot_str_7

MAX_SLOT_IDX     := 7
MAX_SPEED_IDX    := 4

.endif

slot_idx:         .byte 0
speed_idx:        .byte 0
printer_slot_idx: .byte 0
printer_speed_idx:.byte 0
setting_offset:   .byte 0
cur_setting:      .byte 0
modified:         .byte 0

ui_base:          .byte "Serial connection",$0D,$0A,$0D,$0A
                  .byte "Data slot:      ",$0D,$0A
                  .byte "Baud rate:      ",$0D,$0A
                  .byte $0D,$0A
                  .byte "Printer slot:   ",$0D,$0A
                  .byte "Baud rate:      ",$0D,$0A
                  .byte $0D,$0A
                  .ifdef __APPLE2ENH__
                  .byte "Up/down/left/right to configure,",$0D,$0A
                  .else
                  .byte "U/J/left/right to configure,",$0D,$0A
                  .endif
                  .asciiz "Enter to validate."

;Fixme put that where it belongs
;void _switch_text(void)
.proc _switch_text: near
        bit     $C054
        bit     $C051
        bit     $C056
        rts
.endproc

;void simple_serial_configure(void);
.proc _simple_serial_configure: near
        jsr     _switch_text
        lda     #00
        jsr     pusha
        lda     #24
        jsr     _set_scrollwindow

        ; Figure out current slots
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_SLOT
        sta     slot_idx
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_SLOT
        sta     printer_slot_idx


        ; Figure out current speeds
        ldx     #0
@next_x:
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_BAUDRATE
        cmp     baud_rates,x
        bne     :+
        stx     speed_idx
:
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_BAUDRATE
        cmp     baud_rates,x
        bne     :+
        stx     printer_speed_idx
:
        inx
        cpx     #(MAX_SPEED_IDX+1)
        bne     @next_x

        ; Make sure we don't have a port opened
        jsr     _simple_serial_close

        ; Print instructions
        jsr     _clrscr
        lda     #<ui_base
        ldx     #>ui_base
        jsr     _cputs

@update_ui:
        ; Print settings - data slot
        lda     #15
        jsr     pusha
        lda     #2
        jsr     _gotoxy
        lda     cur_setting
        cmp     #0
        jsr     booleq
        jsr     _revers
        ldy     slot_idx
        lda     slots_strs_high,y
        tax
        lda     slots_strs_low,y
        jsr     _cputs

        ; Print settings - data baudrate
        lda     #15
        jsr     pusha
        lda     #3
        jsr     _gotoxy
        lda     cur_setting
        cmp     #1
        jsr     booleq
        jsr     _revers
        ldy     speed_idx
        lda     baud_strs_high,y
        tax
        lda     baud_strs_low,y
        jsr     _cputs

        ; Print settings - printer slot
        lda     #15
        jsr     pusha
        lda     #5
        jsr     _gotoxy
        lda     cur_setting
        cmp     #2
        jsr     booleq
        jsr     _revers
        ldy     printer_slot_idx
        lda     slots_strs_high,y
        tax
        lda     slots_strs_low,y
        jsr     _cputs

        ; Print settings - printer baudrate
        lda     #15
        jsr     pusha
        lda     #6
        jsr     _gotoxy
        lda     cur_setting
        cmp     #3
        jsr     booleq
        jsr     _revers
        ldy     printer_speed_idx
        lda     baud_strs_high,y
        tax
        lda     baud_strs_low,y
        jsr     _cputs

        lda     #0
        sta     setting_offset
        jsr     _revers

@ssc_getchar:
        jsr     _cgetc
        cmp     #$0B          ; CH_CURS_UP
        beq     @curs_up
        cmp     #'U'
        beq     @curs_up
        cmp     #$0A          ; CH_CURS_DOWN
        beq     @curs_down
        cmp     #'J'
        beq     @curs_down
        cmp     #$08
        beq     @curs_left
        cmp     #$15
        beq     @curs_right
        cmp     #$0D
        beq     @curs_done
        bne     @ssc_getchar

@curs_up:
        dec     cur_setting
        bpl     :+
        lda     #3
        sta     cur_setting
:       jmp     @update_ui
@curs_down:
        inc     cur_setting
        lda     cur_setting
        cmp     #4
        bmi     :+
        lda     #0
        sta     cur_setting
:       jmp     @update_ui
@curs_left:
        lda     #<(-1)
        sta     modified
        sta     setting_offset
        bne     @update_setting
@curs_right:
        lda     #<(+1)
        sta     modified
        sta     setting_offset
        bne     @update_setting
@curs_done:
        lda     modified
        beq     @ssc_return

        ; Store settings
        lda     slot_idx
        sta     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_SLOT
        ldy     speed_idx
        lda     baud_rates,y
        sta     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_BAUDRATE

        lda     printer_slot_idx
        sta     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_SLOT
        ldy     printer_speed_idx
        lda     baud_rates,y
        sta     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_BAUDRATE

        ; Save settings to disk
        lda     #<simple_serial_disk_settings
        ldx     #>simple_serial_disk_settings
        jsr     pushax
        lda     #<write_mode_str
        ldx     #>write_mode_str
        jsr     _simple_serial_settings_io

        ; And in RAM
        lda     #<simple_serial_ram_settings
        ldx     #>simple_serial_ram_settings
        jsr     pushax
        lda     #<write_mode_str
        ldx     #>write_mode_str
        jsr     _simple_serial_settings_io

@ssc_return:
        rts

@update_setting:
        lda     cur_setting
        cmp     #0
        beq     @update_data_slot
        cmp     #1
        beq     @update_data_baudrate
        cmp     #2
        beq     @update_printer_slot
        cmp     #3
        beq     @update_printer_baudrate
        ; That can't be
        brk

@update_data_slot:
        lda     slot_idx
        clc
        adc     setting_offset
        bmi     :+
        cmp     #(MAX_SLOT_IDX+1)
        bmi     :++
        lda     #0
        beq     :++
:       lda     #MAX_SLOT_IDX
:       sta     slot_idx
        jmp     @update_ui

@update_data_baudrate:
        lda     speed_idx
        clc
        adc     setting_offset
        bmi     :+
        cmp     #(MAX_SPEED_IDX+1)
        bmi     :++
        lda     #0
        beq     :++
:       lda     #MAX_SPEED_IDX
:       sta     speed_idx
        jmp     @update_ui

@update_printer_slot:
        lda     printer_slot_idx
        clc
        adc     setting_offset
        bmi     :+
        cmp     #(MAX_SLOT_IDX+1)
        bmi     :++
        lda     #0
        beq     :++
:       lda     #MAX_SLOT_IDX
:       sta     printer_slot_idx
        jmp     @update_ui

@update_printer_baudrate:
        lda     printer_speed_idx
        clc
        adc     setting_offset
        bmi     :+
        cmp     #(MAX_SPEED_IDX+1)
        bmi     :++
        lda     #0
        beq     :++
:       lda     #MAX_SPEED_IDX
:       sta     printer_speed_idx
        jmp     @update_ui
.endproc
