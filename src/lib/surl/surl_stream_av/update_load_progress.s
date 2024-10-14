.proc update_load_progress
        ; Go to correct place
        lda       #11
        jsr       pusha
        lda       #0
        jsr       _gotoxy

        ; Send ready
        lda       #SURL_CLIENT_READY
        jsr       _serial_putc_direct

        ; Get ETA
        jsr       _simple_serial_getc
        cmp       #$FF
        beq       @opening
        cmp       #$FE
        beq       @long
@secs:
        ; < 254 means the estimate is about A<<3 seconds
        pha
        lda       #<eta_secs_str
        ldx       #>eta_secs_str
        jsr       pushax
        pla
        ldx       #$00
        jsr       shlax3
        jsr       pushax
        ldy       #$04
        bne       @print
@long:
        ; 254 means more than 30 minutes estimated
        lda       #<eta_long_str
        ldx       #>eta_long_str
        bne       @print2
@opening:
        ; 255 means proxy is still opening the stream
        lda       #<eta_open_str
        ldx       #>eta_open_str
@print2:
        jsr       pushax
        ldy       #$02

@print:
        jmp       _cprintf
.endproc
