patch_vu_meters:
        lda     #<(vu_patches_a)
        sta     ptr1
        lda     #>(vu_patches_a)
        sta     ptr1+1
        jsr     patch_incr

        lda     #<(vu_patches_b)
        sta     ptr1
        lda     #>(vu_patches_b)
        sta     ptr1+1
        ; jsr   patch_incr  ; Fallthrough 
        ; rts

; --------------------------------------
patch_incr:                     ; Patch all registers in ptr1 array with vumeter_base + n
        ldy     #$00            ; Start at beginning
        lda    vumeter_base
        sta    tmp1
next_vu:
        clc
        lda     (ptr1),y
        adc     #1              ; Add one to patch after label
        sta     ptr2
        iny
        lda     (ptr1),y
        beq     vu_done         ; If high byte is 0, we're done
        adc     #0
        sta     ptr2+1
        iny

        tya
        pha
        ldy     #$00

        lda     tmp1            ; Patch low byte
        sta     (ptr2),y

        iny                     ; Patch high byte with base
        lda     vumeter_base+1
        sta     (ptr2),y

        inc     tmp1            ; Increment vumeter address (next char)

        pla
        tay                     ; Y > 0 there

        bne     next_vu
vu_done:
        rts
