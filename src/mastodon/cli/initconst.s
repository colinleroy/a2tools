        .export         _LEFT_COL_WIDTH
        .export         _RIGHT_COL_START
        .export         _NUMCOLS
        .export         _STATE_FILE
        .export         _SCROLL_KEYS
        .export         _TRANSLITCMD
        .export         _TL_SPOILER_TEXT_BUF
        .export         _MEDIA_STR, _IMAGE_STR, _VIDEO_STR, _AUDIO_STR, _GIF_STR

        .import         _has_80cols, _has_128k, _is_iie

        .import         _try_videomode
        
        .import         pushax, _strcpy

        .constructor    initconst, 6

        .segment "DATA"

_LEFT_COL_WIDTH:      .byte 19
_RIGHT_COL_START:     .byte 20
_NUMCOLS:             .byte 80
_TL_SPOILER_TEXT_BUF: .byte 54

_STATE_FILE:          .asciiz "/RAM/mastostate"
_SCROLL_KEYS:         .asciiz "Up/Dn"
_TRANSLITCMD:         .byte "TRANSLIT", $00, $00, $00

_MEDIA_STR:           .asciiz "media"
_IMAGE_STR:           .asciiz "image"
_VIDEO_STR:           .asciiz "video"
_AUDIO_STR:           .asciiz "audio"
_GIF_STR:             .asciiz "gif"

        .segment "ONCE"

; Strings that will get discarded if unused
DISK_STATE_FILE  = _STATE_FILE+5
UJ_STR:             .asciiz "U/J  "
SHORT_MEDIA_STR:    .asciiz "img"
SHORT_IMAGE_STR:    .asciiz "img"
SHORT_VIDEO_STR:    .asciiz "vid"
SHORT_AUDIO_STR:    .asciiz "snd"

.macro COPY_STR dest, src
        lda     #<dest
        ldx     #>dest
        jsr     pushax
        lda     #<src
        ldx     #>src
        jsr     _strcpy
.endmacro

.proc initconst
        lda     _has_128k         ; Do we have 128k
        bne     :+

        lda     #<_STATE_FILE     ; Yes, save state to /RAM
        ldx     #>_STATE_FILE
        jsr     pushax
        lda     #<DISK_STATE_FILE
        ldx     #>DISK_STATE_FILE
        jsr     _strcpy

:       lda     _is_iie           ; Do we have a IIe?
        bne     :+

        ; No, set scroll keys
        COPY_STR _SCROLL_KEYS, UJ_STR

        lda     #'L'              ; Fix Translit command for
        sta     _TRANSLITCMD+8    ; auto lower-casing
        lda     #'C'
        sta     _TRANSLITCMD+9

:       lda     #$00              ; VIDEOMODE_80COL
        jsr     _try_videomode

        lda     _has_80cols       ; Did it work?
        beq     set40
        rts

set40:
        lda     #40
        sta     _NUMCOLS

        lda     #39
        sta     _LEFT_COL_WIDTH

        lda     #0
        sta     _RIGHT_COL_START

        lda     #34
        sta     _TL_SPOILER_TEXT_BUF

        COPY_STR _MEDIA_STR, SHORT_MEDIA_STR
        COPY_STR _IMAGE_STR, SHORT_IMAGE_STR
        COPY_STR _VIDEO_STR, SHORT_VIDEO_STR
        COPY_STR _AUDIO_STR, SHORT_AUDIO_STR
        rts
.endproc
