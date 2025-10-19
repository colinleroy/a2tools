        .export     _freq
        .import     _get_tv, _read_key

        .importzp   ptr1

        .constructor calibrate_hz, 1

        .include    "apple2.inc"
        .include    "freq.inc"

.segment "ONCE"

.proc calibrate_hz
        jsr     _get_tv
        cmp     #TV_OTHER
        beq     :+
        sta     _freq
        rts

        ; We don't know. Let's ask.
:       bit     $C082
        jsr     $FC58
        lda     #<iip_str
        ldx     #>iip_str
        sta     ptr1
        stx     ptr1+1

        ldy     #$00
:       lda     (ptr1),y
        beq     read_kbd
        ora     #$80
        jsr     $FDED
        iny
        bne     :-

read_kbd:
        jsr     _read_key
        sec
        sbc     #'1'          ; '1'-'1'=0, TV_NTSC
        cmp     #2            ; 0/1 accepted
        bcs     read_kbd
        sta     _freq
        bit     $C080
        rts
.endproc

iip_str:        .asciiz "HIT 1 FOR US (60HZ), 2 FOR EURO (50HZ)"

.segment "DATA"
; The only thing remaining from that code after init
_freq:          .res 1
