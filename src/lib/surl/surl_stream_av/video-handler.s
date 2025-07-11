; -----------------------------------------------------------------
; VIDEO HANDLER
video_spkr_sub:                 ; Alternate entry point for duty cycle 30
        ____SPKR_DUTY____4      ; 38
        WASTE_4                 ; 42

; Video handler expects the video byte in Y register, and the N flag set by
; its loading.
; Video handler must take 26 cycles on every code path.

video_sub:
        bcc     @maybe_control          ; 2/3   Is it a data byte?

@set_pixel:                             ;       It is a data byte
        beq     @toggle_page            ; 4/5   Result of cpy #$7F
        ldy     next_offset             ; 7     Load the offset to the start of the base
        sta     (cur_base),y            ; 13    Store data byte
        inc     next_offset             ; 18    and increment offset.
        clv                             ; 20    Reset the offset-received flag.
        JUMP_NEXT_6                     ; 26    Done, go to next duty cycle

@maybe_control:
        bvc     @set_offset             ; 5/6   If V flag is set, this one is a base byte

@set_base:                              ;       This is a base byte
        lda     pages_addrs_arr_low,y   ; 9     Load base pointer low byte from base array
        sta     cur_base                ; 12    Store it to destination pointer low byte
        lda     (page_ptr_high),y       ; 17    Load base pointer high byte from base array
        sta     cur_base+1              ; 20    Store it to destination pointer high byte
        JUMP_NEXT_6                     ; 26    Done, go to next duty cycle

@set_offset:                            ;       This is an offset byte
        sty     next_offset             ; 9     Store offset
        lda     page_ptr_high+1         ; 12    Update the page flag here, where we have time
        sbc     #(PAGE_ARR_TO_SWITCH-1) ; 14    Carry clear so substract one less. Use the fact that:
                                                .assert >PAGE0_ARRAY - PAGE_ARR_TO_SWITCH = $54, error
                                                .assert >PAGE1_ARRAY - PAGE_ARR_TO_SWITCH = $55, error
        .ifdef DOUBLE_BUFFER
        sta     @toggle_page+1          ; 18
        .else
        WASTE_4                         ; 18
        .endif
        adc     #$30                    ; 20    $54/$55 + $30 => sets V flag
        JUMP_NEXT_6                     ; 26    Done, go to next duty cycle

@toggle_page:                           ;       Page toggling
        .ifdef DOUBLE_BUFFER
        lda     $C054                   ; 9     Activate page (patched by set_offset)
        lda     page_ptr_high+1         ; 12    Toggle written page
        eor     #(>PAGE0_ARRAY ^ >PAGE1_ARRAY)  
                                        ; 14    The other array is right in the next/prev page
        sta     page_ptr_high+1         ; 17    No time to update page flag,
        WASTE_3                         ; 20
        JUMP_NEXT_6                     ; 26    We'll do it in @set_offset
        .else
        WASTE_15                        ; 20
        JUMP_NEXT_6                     ; 26
        .endif
