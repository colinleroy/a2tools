.proc surl_stream_av_send_request
        ; Backup url
        pha
        txa
        pha

        ; setup surl request headers
        jsr       push0       ; Push AX 00
        jsr       pusha       ; Push A 0

        sta       got_art     ; init got_art while we got a 0

        ; URL
        pla
        tax
        pla
        jsr       pushax

        ; Method
        lda       #SURL_METHOD_STREAM_AV

        ; Send request
        jsr       _surl_start_request

        lda       _translit_charset
        ldx       _translit_charset+1
        jsr       _simple_serial_puts_nl

        lda       #1          ; Monochrome
        jsr       _serial_putc_direct

        jsr       popptr1     ; Pop sub url to ptr1
        lda       _enable_subtitles
        .assert   SUBTITLES_NO = 0, error
        beq       @no_subs

        ; Do we have a subtitles URL?
        ldx       ptr1+1
        beq       @subs_no_url

        lda       #SUBTITLES_URL
        jsr       _serial_putc_direct
        lda       ptr1
        jsr       _simple_serial_puts_nl
        rts
@subs_no_url:
        lda       #SUBTITLES_AUTO
@no_subs:
        jmp       _serial_putc_direct ; A=0 if coming from _enable_subtitles=0,
                                      ; AUTO if no URL
.endproc
