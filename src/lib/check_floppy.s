; Figure if file is on a floppy, and if so compute where to poke to keep the
; floppy motor running.
; Ref: https://downloads.reactivemicro.com/Apple%20II%20Items/Documentation/Books/Beneath%20Apple%20ProDOS%20v1.2%20&%201.3%20-%20Supplement.PDF
; Ref: https://bitsavers.org/pdf/apple/disk/sony/Software_control_of_IWM.pdf

        .export               _check_floppy
        .export               floppy_motor_on

        .importzp             tmp1, ptr1

_check_floppy:
        stz     floppy_motor_on

        lda     $BF30           ; Get last accessed device
        and     #$7F            ; Clear drive number and keep slot
        sta     tmp1
        lsr                     ; Shift to low nibble
        lsr
        lsr
        lsr
        clc

        adc     #$C0            ; Build slot ROM pointer
        sta     ptr1+1
        stz     ptr1

        ldy     #$01            ; Check for DiskII/IWM in slot
        lda     (ptr1),y
        cmp     #$20
        bne     :+

        ldy     #$03
        lda     (ptr1),y
        bne     :+

        ldy     #$05
        lda     (ptr1),y
        cmp     #$03
        bne     :+

        ldy     #$FF
        lda     (ptr1),y
        bne     :+

        lda     tmp1            ; Restore DEVNUM
        ora     #$80            ; clear drive number and set high bit (1SSS000)
        clc
        adc     #$9             ; Add 9 to point to DiskII/IWM Turn Motor On softswitch */
        sta     floppy_motor_on

:       rts

        .bss
floppy_motor_on: .res 1
