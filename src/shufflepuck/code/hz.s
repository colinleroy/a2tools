        .export     _calibrate_hz, hz
        
        .import     _set_iigs_speed, _get_iigs_speed

        .import     vbl_ready, ostype

        .import     pushax, _bzero
        .importzp   tmp1, ptr1

        .include    "apple2.inc"
        .include    "accelerator.inc"

.code

MAGIC_VAL = 21
.proc waste_189
                                  ; +6 for jsr
        lda     #MAGIC_VAL        ; +2
        sta     tmp1              ; +3
:       dec     tmp1              ;  16 | 8 cycles * 21
        bne     :-                ;  19 |
        inc     tmp1              ; +4 (5-1 for untaken bne)
        rts                       ; +6
.endproc

.proc _calibrate_hz
        lda     ostype
        cmp     #$20
        bcc     iip
        cmp     #$40
        bcc     iie
        cmp     #$50
        bcc     iic
        bcs     iigs

iic:    jmp     calibrate_iic
iie:    jmp     calibrate_iie
iigs:   jmp     calibrate_iigs
iip:    jmp     calibrate_iip
.endproc

.proc calibrate_iic
        php
        sei

        sta     IOUDISOFF
        lda     RDVBLMSK
        bit     ENVBL
        bit     PTRIG           ; Reset VBL interrupt flag
        :       bit     RDVBLBAR        ; Wait for one VBL
        bpl     :-
        bit     PTRIG           ; Reset VBL interrupt flag

        ldx     #0

        ; Wait for second VBL
        :       nop                       ; 2
        inx                       ; 4
        jsr     waste_189         ; 193
        bit     RDVBLBAR          ; 197 - Wait for second VBL
        bpl     :-                ; 200

        ; Cleanup
        asl
        bcs     :+              ; VBL interrupts were already enabled
        bit     DISVBL
:       sta     IOUDISON        ; IIc Tech Ref Man: The firmware normally leaves IOUDIS on.

        plp
        jmp     calibrate_done
.endproc

.proc calibrate_iie
        ; Wait for bit 7 to be off (VBL start)
:       bit     RDVBLBAR
        bmi     :-
        ; Wait for bit 7 to be on (VBL end)
:       bit     RDVBLBAR
        bpl     :-

        ldx     #0

:       ; now wait for bit 7 to be off again
        nop                       ; 2
        inx                       ; 4
        jsr     waste_189         ; 193
        bit     RDVBLBAR          ; 197
        bmi     :-                ; 200
:       ; now wait for bit 7 to be on again (end of next VBL)
        nop                       ; 2
        inx                       ; 4
        jsr     waste_189         ; 193
        bit     RDVBLBAR          ; 197
        bpl     :-                ; 200
        jmp     calibrate_done
.endproc

.proc calibrate_iigs
        ; The same as IIe, but reverted, because fuck you that's why
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed

        ldx     #0

        ; Wait for bit 7 to be on (VBL start)
:       bit     RDVBLBAR
        bpl     :-
        ; Wait for bit 7 to be off (VBL end)
:       bit     RDVBLBAR
        bmi     :-

:       ; now wait for bit 7 to be on again
        nop                       ; 2
        inx                       ; 4
        jsr     waste_189         ; 193
        bit     RDVBLBAR          ; 197
        bpl     :-                ; 200
:       ; now wait for bit 7 to be off again (end of next VBL)
        nop                       ; 2
        inx                       ; 4
        jsr     waste_189         ; 193
        bit     RDVBLBAR          ; 197
        bmi     :-                ; 200

        jsr     calibrate_done

        lda     prevspd
        jmp     _set_iigs_speed
.endproc

.proc calibrate_iip
        bit     $C082
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
        lda     KBD
        bpl     read_kbd
        bit     KBDSTRB
        ldx     #60

        and     #$7F
        cmp     #'1'
        beq     set60
        cmp     #'2'
        bne     read_kbd
        ldx     #50
set60:  stx     hz
        bit     $C080
        rts
.endproc

.proc calibrate_done
        lda     #60               ; Consider we're at 60Hz
        sta     hz
        ; we'll now have X ~= 86 at 60Hz, ~= 102 at 50Hz:
        ; 86*200 = 17200, 102*200 = 20400.
        ; Consider any X >= 92 means 50Hz
        cpx     #92
        bcc     :+
        lda     #50               ; we're at 50Hz
        sta     hz
:       rts
.endproc

.data
iip_str:        .asciiz "HIT 1 FOR US (60HZ), 2 FOR EURO (50HZ)"

.bss

prevspd:        .res 1
hz:             .res 1
