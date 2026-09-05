; Our own serial parameters
        .export         _ser_params, sser_c
        .export         _simple_serial_read_config
        .export         _simple_serial_write_config
        .export         _simple_serial_settings_io
        .export         simple_serial_ram_settings
        .export         simple_serial_disk_settings

        .import         _reopen_start_device
        .import         _register_start_device

        .import         __filetype, __auxtype
        .import         _open, _read, _write, _close, _unlink
        .import         pusha, pusha0, pushax, return0, returnFFFF
        .import         ostype

        .constructor    setup_serial_defaults
        .destructor     unlink_tmpfile

        .include        "../../simple_serial.inc"
        .include        "apple2.inc"
        .include        "fcntl.inc"
        .include        "ser-kernel.inc"

        .segment "BSS"

settings_fd:        .res 1
sser_c:             .res 2

        .segment "DATA"

_ser_params:    .byte SER_BAUD_115200   ; Data speed
                .byte 2                 ; Data slot
                .byte SER_BAUD_9600     ; Printer speed
                .byte 1                 ; Printer slot
.ifdef EXTRA_SERIAL_CONFIG
                .byte $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
.endif

        .segment "RODATA"

simple_serial_ram_settings:   .asciiz "/RAM/serialcfg"
simple_serial_disk_settings:= simple_serial_ram_settings+5 ; "serialcfg"
read_mode_str:                .asciiz "r"
write_mode_str:               .asciiz "w"

        .segment "ONCE"

; Used to fix default serial port (2) to 0 (modem) on IIgs
setup_serial_defaults:
        bit     ostype
        bpl     :+
        lda     #0
        sta     _ser_params+SIMPLE_SERIAL_PARAMS::DATA_SLOT
        ; IIgs printer port: same as default 8-bits printer slot
        ; lda     #1
        ; sta     _ser_params+SIMPLE_SERIAL_PARAMS::PRINTER_SLOT
:       rts

        .segment "RT_ONCE"

;char __fastcall__ simple_serial_settings_io(const char *path, int flags);
.proc _simple_serial_settings_io: near
        ; Store mode temporarily in sser_c
        sta     sser_c

        ; Set filetype
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda     sser_c
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
        ldy     sser_c
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

.proc simple_serial_read_from_AX: near
        jsr     pushax
        lda     #<(O_RDONLY)
        ldx     #>(O_RDONLY)
        jmp     _simple_serial_settings_io
.endproc

.proc _simple_serial_read_config: near
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

.proc do_write_config
        jsr     pushax
        lda     #<(O_WRONLY|O_CREAT)
        ldx     #>(O_WRONLY|O_CREAT)
        jmp     _simple_serial_settings_io
.endproc

.proc _simple_serial_write_config
        ; Save settings to disk
        lda     #<simple_serial_disk_settings
        ldx     #>simple_serial_disk_settings
        jsr     do_write_config

        ; And in RAM
        lda     #<simple_serial_ram_settings
        ldx     #>simple_serial_ram_settings
        jmp     do_write_config
.endproc

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

.proc unlink_tmpfile
        lda     #<simple_serial_ram_settings
        ldx     #>simple_serial_ram_settings
        jmp     _unlink
.endproc
