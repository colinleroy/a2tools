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
        .export         _ser_params

        .export         _simple_serial_open
        .export         _simple_serial_open_printer
        .export         _simple_serial_close
        .export         _simple_serial_settings_io
        .export         _simple_serial_getc_with_timeout
        .export         _simple_serial_getc

        .export         simple_serial_ram_settings
        .export         simple_serial_disk_settings
        .export         read_mode_str, write_mode_str

        .import         __filetype, __auxtype
        .import         _open, _read, _write, _close
        .import         pusha, pusha0, pushax, return0, returnFFFF
        
        .import         _reopen_start_device
        .import         _register_start_device

        .importzp       ptr1

        .ifdef IIGS
        .import         _a2e_gs_ser
        .import         _get_iigs_speed
        .import         _set_iigs_speed
        .else
        .ifdef __APPLE2ENH__
        .import         _a2e_ssc_ser
        .else
        .import         _a2_ssc_ser
        .endif
        .endif

        .include        "../../simple_serial.inc"
        .include        "fcntl.inc"
        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"
        .include        "accelerator.inc"

        .data

_baudrate:      .byte $00
_flow_control:  .byte SER_HS_HW
_open_slot:     .byte 0

; Our own serial parameters
_ser_params:    .byte SER_BAUD_115200
                .byte MODEM_SER_SLOT
                .byte SER_BAUD_9600
                .byte PRINTER_SER_SLOT

; CC65's serial parameters
default_params: .byte SER_BAUD_115200
                .byte SER_BITS_8
                .byte SER_STOP_1
                .byte SER_PAR_NONE
                .byte SER_HS_HW

        .rodata

simple_serial_ram_settings:   .asciiz "/RAM/serialcfg"
simple_serial_disk_settings:= simple_serial_ram_settings+5 ; "serialcfg"
; Fixme incorrect place
read_mode_str:  .asciiz "r"
write_mode_str: .asciiz "w"

        .segment "RT_ONCE"

;char __fastcall__ simple_serial_open_printer(void);
.proc _simple_serial_open_printer: near
        jsr     simple_serial_read_opts
        ldy     #SIMPLE_SERIAL_PARAMS::PRINTER_BAUDRATE
        jmp     simple_serial_open_direct
.endproc

;char __fastcall__ simple_serial_open(void);
.proc _simple_serial_open: near
        ; Get options
        jsr     simple_serial_read_opts
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

        ; get driver address
        .ifdef IIGS
        lda     #<_a2e_gs_ser
        ldx     #>_a2e_gs_ser
        .else
        .ifdef __APPLE2ENH__
        lda     #<_a2e_ssc_ser
        ldx     #>_a2e_ssc_ser
        .else
        lda     #<_a2_ssc_ser
        ldx     #>_a2_ssc_ser
        .endif
        .endif

        ; install driver
        jsr     _ser_install
        cmp     #$00
        bne     @simple_serial_open_slot_err

        ; call _ser_apple2_slot(n), which is
        ; _ser_ioctl(0, (void *)n)
        jsr     pusha
        tax
        lda     _open_slot
        jsr     _ser_ioctl
        cmp     #$00
        bne     @simple_serial_open_slot_err

        ; set params
        lda     _baudrate
        sta     default_params + SER_PARAMS::BAUDRATE
        lda     _flow_control
        sta     default_params + SER_PARAMS::HANDSHAKE

        lda     #<default_params
        ldx     #>default_params

        ; open port
        jsr     _ser_open
@simple_serial_open_slot_err:
        rts
.endproc

.proc simple_serial_read_from_AX: near
        jsr     pushax
        lda     #<(O_RDONLY)
        ldx     #>(O_RDONLY)
        jmp     _simple_serial_settings_io
.endproc

.proc simple_serial_read_opts: near
        jsr     _register_start_device

        lda     #<simple_serial_ram_settings
        ldx     #>simple_serial_ram_settings
        jsr     simple_serial_read_from_AX

        cmp     #$00
        beq     :+

        jsr     _reopen_start_device
        lda     #<simple_serial_disk_settings
        ldx     #>simple_serial_disk_settings
        jsr     simple_serial_read_from_AX

:       jmp     _reopen_start_device
.endproc

;char __fastcall__ simple_serial_close(void);
.proc _simple_serial_close: near
        lda     #$00
        sta     _baudrate
        sta     _open_slot
        jsr     _ser_close
        jmp     _ser_uninstall
.endproc

;char __fastcall__ simple_serial_settings_io(const char *path, int flags);
.proc _simple_serial_settings_io: near
        ; Store mode temporarily in c
        sta     c

        ; Set filetype
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda     c
        jsr     pushax
        ldy     #$04          ; _open is variadic

        ; Open file (path, flags already set)
        jsr     _open
        cmp     #$FF
        bne     @sss_open_ok
        cpx     #$FF
        beq     @sss_err_open

@sss_open_ok:
        sta     settings_fd

        ; Prepare read/write call

        ldx     #$00
        jsr     pushax

        lda     #<_ser_params
        ldx     #>_ser_params
        jsr     pushax

        lda     #<.sizeof (SIMPLE_SERIAL_PARAMS)
        ldx     #>.sizeof (SIMPLE_SERIAL_PARAMS)

        ; Call correct function
        ldy     c
        cpy     #(O_RDONLY)
        beq     @sss_read

        jsr     _write
        jmp     @sss_close
@sss_read:
        jsr     _read
@sss_close:
        lda     settings_fd
        ldx     #$00
        jsr     _close
        jmp     return0

@sss_err_open:
        jmp     returnFFFF
.endproc

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

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

        ; Init c
        lda     #$00
        sta     c
        sta     c+1

        .ifdef IIGS
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     orig_speed_reg
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
        .endif

@getc_try:
        ; Try to get char (into c)
        lda     #<c
        ldx     #>c
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        bne     @getc_out

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
        sta     c
        sta     c+1

        ; Done
@getc_out:
        .ifdef IIGS
        lda     orig_speed_reg
        jsr     _set_iigs_speed
        .endif

        lda     c
        ldx     c+1
        rts
.endproc

        .bss

ser_timeout_cycles: .res 2
c:                  .res 2
settings_fd:        .res 1
.ifdef IIGS
orig_speed_reg:     .res 1
.endif
