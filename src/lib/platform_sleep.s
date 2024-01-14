;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _platform_sleep
        .export         _platform_msleep
        .import         _slowdown
        .import         _speedup
        .importzp       tmp1

;void __fastcall__ platform_sleep(uint16 s)

_platform_sleep:
        stx     tmp1
        tay
        ora     tmp1
        beq     out
        jsr     _slowdown; Down to 1MHz for consistency around MONWAIT
        bit     $C082
sleep_1s:
        ldx     #10             ; Loop 10 times
sleep_100ms:
        lda     #199            ; Sleep about 99ms
        jsr     $FCA8
        lda     #12             ; About 1ms
        jsr     $FCA8
        dex
        bne     sleep_100ms
        dey
        bne     sleep_1s
        dec     tmp1
        bmi     done
        ldy     #$FF
        bne     sleep_1s
done:
        bit     $C080
        jsr     _speedup
out:
        rts

;void __fastcall__ platform_msleep(uint16 ms)

_platform_msleep:
        tay                     ; low byte in Y, high byte in X
        jsr     _slowdown
        bit     $C082

:       lda     #$11
        jsr     $FCA8
        lda     #$2
        jsr     $FCA8
        dey
        bne     :-
        dex
        bpl     :-
        bit     $C080
        jsr     _speedup
        rts
