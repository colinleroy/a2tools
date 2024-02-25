
        .importzp             ptr1

        .import               _serial_read_byte_no_irq
        .import               _simple_serial_setup_no_irq_regs
        .import               _simple_serial_set_irq
        .import               _serial_putc_direct

        .export               _surl_stream

MAX_OFFSET =126
NUM_BASES  =(8192/MAX_OFFSET)

calc_bases:
        lda     #<(base_addr)
        sta     ptr1
        lda     #>(base_addr)
        sta     ptr1+1

        ldy     #0
        lda     #$00
        ldx     #$20

        clc

next_base:
        sta     (ptr1),y
        iny
        pha
        txa
        sta     (ptr1),y
        pla
        iny

        adc     #(MAX_OFFSET)
        bcc     :+
        inx
        clc
:       cpy     #(NUM_BASES*2)
        bcc     next_base
        rts


_surl_stream:
        jsr     calc_bases

        jsr     _simple_serial_setup_no_irq_regs
        lda     #$00
        jsr     _simple_serial_set_irq

        ldx     #$00            ; Offset
        lda     #($00|$80)      ; Base

set_base:
        cmp     #$FF            ; Is it a rep in reality ?
        beq     do_rep
        and     #%01111111      ; Get rid of the sign
        asl     a               ; shift for array index
        tay
        lda     base_addr,y
        sta     store_dest+1
        iny
        lda     base_addr,y
        sta     store_dest+2

loop:
        ldy     #$01
        jsr     _serial_read_byte_no_irq
        bpl     store_dest
        cmp     #$FF            ; Is it a rep?
        bne     set_offset
do_rep:
        ; handle repetitions
        jsr     _serial_read_byte_no_irq ; get repetitions
        tay
        jsr     _serial_read_byte_no_irq ; get value
store_dest:
        sta     $FFFF,x
        inx
        dey
        bne    store_dest
        jmp    loop

set_offset:
        and     #%01111111      ; Get rid of sign
        tax                     ; set offset
        jsr     _serial_read_byte_no_irq
        bmi     set_base        ; We also have a new base (or repetition?)
        jmp     store_dest      ; Otherwise it's a value


        .bss
base_addr:      .res (NUM_BASES*2)
