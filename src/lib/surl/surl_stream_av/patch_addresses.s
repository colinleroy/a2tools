patch_addresses:                ; Patch all registers in ptr1 array with A
        ldy     #$00            ; Start at beginning
        sta     tmp1            ; Save value
next_addr:
        clc
        lda     (ptr1),y
        adc     #1              ; Add one to patch after label
        sta     ptr2
        iny
        lda     (ptr1),y
        beq     done            ; If high byte is 0, we're done
        adc     #0
        sta     ptr2+1
        iny

        .ifdef STREAM_CHECK_ADDR_PATCH
        lda     (ptr2)          ; Debug to be sure
        cmp     #$FF
        beq     :+
        brk
:
        .endif

        tya
        pha
        ldy     #$00

        lda     tmp1            ; Patch low byte
        sta     (ptr2),y

        iny                     ; Patch high byte with base
        txa
        sta     (ptr2),y

        pla
        tay                     ; Y > 0 there

        bne     next_addr
done:
        rts
