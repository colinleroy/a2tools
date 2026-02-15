        .export       _flip_page_after_vbl
        .interruptor  irq_handler, 15

irq_handler:
        lda       #$01
        sta       irq_fired
        clc                    ; Let mouse driver handle it
        rts

waste_65:                      ; 6
        nop                    ; 2
        ldy       #10          ; 2
:       dey                    ; 5*(Y-1) + 4
        bne       :-           ;
        rts                    ; 6

_flip_page_after_vbl:
        lda       #$00
        sta       irq_fired

:       lda       irq_fired
        beq       :-

        ldx       #111        ; Skip ~120 lines, so we see the band on both PAL and NTSC
:       jsr       waste_65    ; 65
        dex                   ; 67
        bne       :-          ; 70

        bit       $C055       ; Toggle page 2
        ldx       #3
:       jsr       waste_65    ; Keep on page 2 for three lines
        dex                   ; 67
        bne       :-
        bit       $C054       ; Toggle page 1

        rts

.bss

irq_fired:       .res  1
