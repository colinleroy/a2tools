; Figure if file is on a floppy, and if so compute where to poke to keep the
; floppy motor running.
; Ref: https://downloads.reactivemicro.com/Apple%20II%20Items/Documentation/Books/Beneath%20Apple%20ProDOS%20v1.2%20&%201.3%20-%20Supplement.PDF
; Ref: https://bitsavers.org/pdf/apple/disk/sony/Software_control_of_IWM.pdf

        .export               _check_floppy
        .export               floppy_motor_on

_check_floppy:
        stz     floppy_motor_on

        lda     $FED1           ; Check jmp vector at $FED1/FED2
        bne     :+
        lda     $FED2
        cmp     #$D0
        bne     :+
                                ; jmp vector points to DiskII driver at $D000
        lda     $43             ; load unit/drive number (DSSS0000)
        ora     #$80            ; clear drive number and set high bit (1SSS000)
        clc
        adc     #$9             ; Add 9 to point to DiskII/IWM Turn Motor On softswitch */
        sta     floppy_motor_on

:       rts

        .bss
floppy_motor_on: .res 1
