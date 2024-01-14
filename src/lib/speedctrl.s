;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _slowdown
        .export         _speedup

;void __fastcall__ platform_sleep(uint8 s)

_slowdown:
.ifdef IIGS
        lda     $C036
        sta     cyareg_orig
        and     #$79
        sta     $C036
.endif
        rts

_speedup:
.ifdef IIGS
        lda     cyareg_orig
        sta     $C036
.endif
        rts

.ifdef IIGS
        .bss
cyareg_orig:
        .res 1
.endif
