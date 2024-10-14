.proc surl_stream_av_prepare_start
        lda       #SURL_CLIENT_READY
        jsr       _serial_putc_direct

        ; Check video port on proxy
        jsr       _simple_serial_getc
        cmp       #SURL_VIDEO_PORT_NOK
        bne       @video_ok
        ; Inform there's no video available
        jsr       _clrscr
        lda       #<no_video_str
        ldx       #>no_video_str
        jsr       _cputs
        lda       #5
        jsr       _sleep
@video_ok:
        lda       got_art
        bne       @hgr_is_set

        ; If we didn't get art, clear page
        lda       #<$2000
        ldx       #>$2000
        jsr       pushax
        jsr       _bzero

@hgr_is_set:
        .ifdef    __APPLE2ENH__
        lda       #VIDEOMODE_40COL
        jsr       _videomode
        .endif
        jsr       _hgr_mixoff
        lda       #$00
        jsr       pusha
        lda       _scrh
        jsr       _set_scrollwindow
        jsr       _clrscr
        .ifdef    __APPLE2ENH__
        jsr       _surl_start_stream_av
        lda       #VIDEOMODE_80COL
        jmp       _videomode
        .else
        jmp       _surl_start_stream_av
        .endif
.endproc
