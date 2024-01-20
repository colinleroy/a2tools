;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _platform_msleep
        .import         _get_iigs_speed, _set_iigs_speed
        .importzp       tmp1

        .include        "accelerator.inc"

;void __fastcall__ platform_msleep(uint16 ms)

_platform_msleep:
        pha                     ; Backup AX
        txa
        pha
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
        bit     $C082

        pla                     ; Restore AX
        tax
        pla
        tay                     ; Move low byte to Y

:       lda     #$0F
        jsr     $FCA8
        lda     #$03
        jsr     $FCA8
        dey
        bne     :-
        dex
        bpl     :-
        bit     $C080
        lda     prevspd
        jmp     _set_iigs_speed

        .bss
prevspd:.res 1
