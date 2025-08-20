.proc surl_stream_av_get_art
        lda       #'H'
        jsr       _serial_putc_direct
        lda       #<$2000     ; Read to HGR page
        ldx       #>$2000
        jsr       pushax
        ; AX already the correct length
        jsr       _surl_read_with_barrier
        lda       #SURL_CLIENT_READY
        sta       got_art
        jmp       _serial_putc_direct
.endproc
