.proc _surl_stream_av
        jsr       surl_stream_av_send_request
        jsr       surl_stream_av_setup_ui

        ; Ready, send last parameter
        lda       _video_size
        jsr       _serial_putc_direct

        ; Now wait
@wait_load:
        jsr       _simple_serial_getc
        cmp       #SURL_ANSWER_STREAM_LOAD
        beq       @handle_stream_load
        cmp       #SURL_ANSWER_STREAM_ART
        beq       @handle_stream_art
        cmp       #SURL_ANSWER_STREAM_START
        bne       @handle_error

@handle_stream_start:
        jsr       surl_stream_av_prepare_start
        jmp       @stream_url_done

@handle_stream_load:
        jsr       surl_stream_av_handle_preload
        jmp       @wait_load

@handle_stream_art:
        jsr       surl_stream_av_get_art
        jmp       @wait_load

@handle_error:
        jsr       _clrscr
        lda       #<playback_err_str
        ldx       #>playback_err_str
        jsr       _cputs
        lda       #1
        jsr       _sleep
        jmp       @stream_url_done

@stream_url_done:
        lda       #$00
        jsr       pusha
        lda       _scrh
        jsr       _set_scrollwindow
        lda       #$00
        tax
        rts
.endproc
