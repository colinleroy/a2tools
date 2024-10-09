;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _platform_msleep
        .export         _platform_interruptible_msleep
        .import         _get_iigs_speed, _set_iigs_speed
        .importzp       tmp1

        .include        "accelerator.inc"

;void __fastcall__ platform_msleep(uint16 ms)

KBD          := $C000   ; Read keyboard

        .segment "LOWCODE"

_platform_interruptible_msleep:
        ldy     #$80
        bne     start_sleep
_platform_msleep:
        ldy     #$00
start_sleep:
        sty     @int_check+1
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
        lda     #$04
        jsr     $FCA8
        lda     KBD
@int_check:
        and     #$80
        bne     @interrupted
        dey
        bne     :-
        dex
        bpl     :-
@interrupted:
        bit     $C080
        lda     prevspd
        jmp     _set_iigs_speed

        .bss
prevspd:.res 1
