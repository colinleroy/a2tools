; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _surl_read_image_to_screen
        .import         _surl_read_with_barrier, _write_hgr_to_aux

        .import         _can_dhgr

        .import         pushax, swapstk

        .ifdef          SURL_TO_LANGCARD
        .segment        "LOWCODE"
        .endif

        .include        "apple2.inc"

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

        ; Do we already know we can do DHGR?
        ldy     _can_dhgr
        beq     safe

        ; Do we have RD80COL?
        bit     RD80COL
        bpl     safe
        ; and HIRES?
        bit     $C01D
        bpl     safe

        ; so we can write to AUX directly using PAGE2
        bit     $C055
        jsr     _surl_read_with_barrier
        bit     $C054
        lda     #$FF                    ; Return value: is_dhgr true
        bne     continue

safe:   ; Not sure we can write to /RAM, so try
        jsr     _surl_read_with_barrier ; Read the first (aux) page
        jsr     _write_hgr_to_aux       ; Move it to AUX
continue:
        sta     is_dhgr                 ; Remember result

        lda     #<$2000                 ; 8kB remain, in MAIN
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
