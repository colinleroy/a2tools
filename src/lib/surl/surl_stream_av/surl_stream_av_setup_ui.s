.proc surl_stream_av_setup_ui
        ; Clear screen, setup HGR mix with scrollwindow
        jsr       _clrscr
        lda       #20
        jsr       pusha
        lda       _scrh
        jsr       _set_scrollwindow

        lda       #1
        jsr       _init_hgr
        jsr       _hgr_mixon
        jsr       _clrscr

        ; Clear text page 2
        lda       #<$0800
        ldx       #>$0800
        jsr       pushax

        lda       #($20|$80)  ; with non-inverse spaces
        jsr       pusha0

        ; 1kB
        lda       #<$400
        ldx       #>$400
        jsr       _memset

        ; Show controls
        lda       #<controls_str
        ldx       #>controls_str
        jmp       _cputs
.endproc
