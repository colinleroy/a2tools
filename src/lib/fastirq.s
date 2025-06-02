;
; Oliver Schmidt, 2012-11-17
; Colin Leroy-Mira, 2023-09-02 - Handle IRQs without ProDOS
;
; IRQ handling (Apple2 version)
;

        .import         callirq, ostype, machinetype

        .constructor    _init_fast_irq, 8
        .destructor     _done_fast_irq, 9

        .importzp       _prev_ram_irq_vector, _prev_rom_irq_vector

        .include        "apple2.inc"

; ------------------------------------------------------------------------



ROMIRQVEC:= $03fe
RAMIRQVEC:= $fffe

        .segment        "ONCE"

_init_fast_irq:
        bit     ostype
        bpl     :+
        rts
:
        ; Disable IRQs
        php
        sei

        ; Save previous ROM IRQ vector
        lda     ROMIRQVEC
        ldx     ROMIRQVEC+1
        sta     _prev_rom_irq_vector
        stx     _prev_rom_irq_vector+1

        ; Update ROM IRQ vector
        lda     #<handle_rom_irq
        ldx     #>handle_rom_irq
        sta     ROMIRQVEC
        stx     ROMIRQVEC+1

        ; Switch to RAM
        bit     LCBANK2
        bit     LCBANK2

        ; Save previous RAM IRQ vector
        lda     RAMIRQVEC
        ldx     RAMIRQVEC+1
        sta     _prev_ram_irq_vector
        stx     _prev_ram_irq_vector+1

        ; And update it
        lda     #<handle_ram_irq
        ldx     #>handle_ram_irq
        sta     RAMIRQVEC
        stx     RAMIRQVEC+1

        ; Enable IRQs
        plp
        rts

; ------------------------------------------------------------------------

        .segment        "LOWCODE"

_done_fast_irq:
        bit     ostype
        bpl     :+
        rts
:
        ; Restore ROM IRQ vector
        lda     _prev_rom_irq_vector
        ldx     _prev_rom_irq_vector+1
        sta     ROMIRQVEC
        stx     ROMIRQVEC+1

        ; Switch to RAM
        bit     LCBANK2
        bit     LCBANK2

        ; Same for RAM
        lda     _prev_ram_irq_vector
        ldx     _prev_ram_irq_vector+1
        sta     RAMIRQVEC
        stx     RAMIRQVEC+1

        ; And back to ROM, we're exiting
        bit     ROMIN
        rts

; ------------------------------------------------------------------------

handle_ram_irq:
        sta     a_bck           ; Save A
        pla                     ; Check for BRK
        pha
        and     #%00010000       ; Check bit 4 (BRK flag)
        bne     do_brk

        ; It's an IRQ

        stx     x_bck            ; Save X,Y
        sty     y_bck
        jsr     callirq
        bcc     give_back_irq
        ldx     x_bck
        ldy     y_bck
        lda     a_bck
        rti
give_back_irq:
        ldx     x_bck
        ldy     y_bck
        lda     a_bck
        jmp     (_prev_ram_irq_vector)

do_brk:
        ; Give BRK to the standard handler
        jmp     (_prev_ram_irq_vector)

handle_rom_irq:                 ; ROM saves things for us (apart from A on IIe)
        .ifndef __APPLE2ENH__
        bit     machinetype
        bvs     c
        lda     $45             ; Backup A to a safe place
        sta     a_bck
        stx     x_bck
        sty     y_bck
        tsx
        stx     s_bck
        .endif
c:      jsr     callirq
        bcs     handled
        jmp     (_prev_rom_irq_vector)
handled:
        .ifndef __APPLE2ENH__
        bit     machinetype
        bvs     :+
        ldx     s_bck
        txs
        lda     a_bck
        ldx     x_bck
        ldy     y_bck
:       .endif
        rti

        .segment "BSS"

a_bck: .res 1
x_bck: .res 1
y_bck: .res 1
s_bck: .res 1
