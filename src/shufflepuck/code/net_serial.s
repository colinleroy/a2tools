        .export     _configure_serial, _teardown_serial
        .export     _send_byte_serial, _get_byte_serial

        .import     _conf_help_str

        .import     _home, _gotox, _gotoy, _gotoxy
        .import     _platform_msleep
        .import     _read_key, _last_key, _cout, _numout, _strout

        .import     game_cancelled

        .import     pushax, popax

        .include    "../code/constants.inc"

; Aliases for naming convention
_send_byte_serial = _serial_read_byte_direct
_get_byte_serial  = _serial_putc_direct
_teardown_serial  = _serial_close

; Configuration code

.proc _configure_serial
        bit     ostype
        bpl     ask_slot

        ; Patch defaults and callbacks for
        ; IIgs z8530 integrated serial ports
        lda     #0
        sta     serial_slot

ask_slot:
        lda     #0
        sta     _last_key         ; Reset last key to avoid pausing on config exit

        lda     #0
        jsr     pusha
        lda     #23
        jsr     _gotoxy
        lda     #<_conf_help_str
        ldx     #>_conf_help_str
        jsr     _strout

        lda     #20
        jsr     _gotoy

        bit     ostype
        bmi     iigs
        lda     #1+1
        ldx     #7
        jmp     :+
iigs:
        lda     #0+1
        ldx     #1
:       jsr     pushax
        lda     #<serial_slot
        ldx     #>serial_slot
        jsr     pushax
        lda     #<slot_str
        ldx     #>slot_str
        jsr     configure_slot

        lda     game_cancelled
        bne     cancel

        lda     serial_slot
        ldx     #SER_BAUD_115200
        jsr     _serial_open
        cmp     #SER_ERR_OK
        bne     open_error
        clc
        rts

open_error:
        lda     game_cancelled    ; Did we cancel at some point?
        bne     cancel

        lda     #12
        jsr     pusha
        lda     #22
        jsr     _gotoxy
        lda     #<open_error_str
        ldx     #>open_error_str
        jsr     _strout
        lda     #<1000
        ldx     #>1000
        jsr     _platform_msleep
        jsr     _home
        jmp     ask_slot

cancel:
        sec
        rts
.endproc

.proc configure_slot
        sta     str_low+1
        stx     str_high+1
        ; parameter
        jsr     popax
        sta     get_parameter+1
        sta     update_parameter+1
        stx     get_parameter+2
        stx     update_parameter+2
        ; lower bound
        jsr     popax
        sta     dec_param+1
        stx     inc_param+1

print:
        lda     #0
        jsr     _gotox

str_low:
        lda     #$FF
str_high:
        ldx     #$FF
        jsr     _strout

get_parameter:
        lda     $FFFF
        sta     tmp_param
        ldx     #0

        bit     ostype
        bmi     print_port_name
print_slot_number:
        jsr     _numout

        lda     #' '
        jsr     _cout
        jmp     update

print_port_name:
        lda     #<modem_str
        ldx     #>modem_str
        ldy     tmp_param
        beq     :+
        lda     #<printer_str
        ldx     #>printer_str
:       jsr     _strout

update:
        ldx     tmp_param
        jsr     _read_key
        cmp     #$08
        beq     dec_param
        cmp     #$15
        beq     inc_param
        cmp     #$0D              ; Enter
        beq     done
        cmp     #CH_ESC
        beq     cancel
        bne     print
dec_param:
        cpx     #$00
        bcc     print
        dex
        jmp     update_parameter
inc_param:
        cpx     #$FF
        bcs     print
        inx
update_parameter:
        stx     $FFFF
        jmp     print

cancel:
        inc     game_cancelled
done:
        rts
.endproc

; This goes in the n(etwork) segment
.segment "n"

; And include the low-level serial code
.segment "n"
.include "../lib/serial/asm/driver.s"


.segment "RODATA"

slot_str:         .asciiz "SERIAL SLOT: "
open_error_str:   .asciiz "SERIAL OPEN ERROR"
modem_str:        .asciiz "MODEM  "
printer_str:      .asciiz "PRINTER"

tmp_param:        .byte   0
