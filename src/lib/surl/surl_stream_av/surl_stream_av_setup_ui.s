.proc surl_stream_av_setup_ui
        ; Clear screen, setup HGR mix with scrollwindow
        jsr       _clrscr
        lda       #20
        jsr       pusha
        lda       #24
        jsr       _set_scrollwindow

        lda       #1
        jsr       _init_hgr
        jsr       _hgr_mixon
        jsr       _clrscr

        lda       #<$0800     ; Clear text page 2
        ldx       #>$0800
        jsr       pushax

        lda       #($20|$80)  ; with non-inverse spaces
        jsr       pusha0

        lda       #<$400      ; 1kB
        ldx       #>$400
        jsr       _memset

        ; Show controls
        lda       #<controls_40str
        ldx       #>controls_40str
        bit       _has_80cols
        bpl       :+

        lda       #<controls_80str1
        ldx       #>controls_80str1
        jsr       _cputs
        lda       #<controls_80str2
        ldx       #>controls_80str2
:       jmp       _cputs
.endproc
