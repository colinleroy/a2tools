.proc surl_stream_av_handle_preload
        jsr       update_load_progress
        lda       KBD
        bmi       @no_escape

        sta       KBDSTRB
        and       #$7F
        cmp       #$1B        ; Escape?
        bne       @no_escape
        lda       #SURL_METHOD_ABORT
        bne       @send_pong_char
@no_escape:
        lda       #SURL_CLIENT_READY
@send_pong_char:
        jmp       _serial_putc_direct
.endproc
