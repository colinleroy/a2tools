; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _surl_read_image_to_screen
        .import         _surl_read_with_barrier, _write_hgr_to_aux

        .import         pushax, swapstk

        .ifdef          SURL_TO_LANGCARD
        .segment        "LOWCODE"
        .endif

.proc _surl_read_image_to_screen: near
        ldy     #0
        sty     is_dhgr
        cmp     #<$4000
        bne     no_dhgr
        cpx     #>$4000
        bne     no_dhgr

        lda     #<$2000
        ldx     #>$2000
        jsr     pushax
        jsr     _surl_read_with_barrier ; Read the first (aux) page
        jsr     _write_hgr_to_aux       ; Move it to AUX
        sta     is_dhgr                 ; Remember result

        lda     #<$2000                 ; 8kB remain
        ldx     #>$2000
no_dhgr:
        jsr     pushax                  ; Remember len
        lda     #<$2000
        ldx     #>$2000
        jsr     swapstk
        jsr     _surl_read_with_barrier ; Read the MAIN page

is_dhgr = *+1
        lda     #$FF
        ldx     #$00
        rts
.endproc
