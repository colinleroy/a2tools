        .export     hz
        .import     _get_tv, _read_key

        .importzp   ptr1

        .constructor calibrate_hz, 1

        .include    "apple2.inc"

.segment "ONCE"

hz_vals:
        .byte   60                ; TV_NTSC
        .byte   50                ; TV_PAL
        .byte   0                 ; TV_OTHER

.proc calibrate_hz
        jsr     _get_tv
        tax
        lda     hz_vals,x
        sta     hz
        beq     :+
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
        ldx     #60

        cmp     #'1'
        beq     set60
        cmp     #'2'
        bne     read_kbd
        ldx     #50
set60:  stx     hz
        bit     $C080
        rts
.endproc

iip_str:        .asciiz "HIT 1 FOR US (60HZ), 2 FOR EURO (50HZ)"

.segment "DATA"
; The only thing remaining from that code after init
hz:             .res 1
