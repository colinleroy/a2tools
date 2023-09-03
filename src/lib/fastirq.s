;
; Oliver Schmidt, 2012-11-17
; Colin Leroy-Mira, 2023-09-02 - Handle IRQs without ProDOS
;
; IRQ handling (Apple2 version)
;

        .export         _init_fast_irq, _done_fast_irq
        .import         callirq
        .include        "apple2.inc"

; ------------------------------------------------------------------------



ROMIRQVEC:= $03fe
RAMIRQVEC:= $fffe

        .segment       "DATA"

_prev_rom_irq_vector: .res 2
_prev_ram_irq_vector: .res 2

        .segment        "LOWCODE"

_init_fast_irq:
        ; Disable IRQs
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
        cli
        rts

; ------------------------------------------------------------------------

_done_fast_irq:
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

; ------------------------------------------------------------------------

handle_ram_irq:
        .ifdef __APPLE2ENH__

        pha                     ; Save A,Y,X,P
        phy
        phx
        php
        jsr     callirq
        plp                     ; Restore everything
        plx
        ply
        pla

        .else

        pha                     ; Save A,Y,X,P
        tya
        pha
        txa
        pha
        php
        jsr     callirq
        plp                     ; Restore everything
        pla
        tax
        pla
        tay
        pla

        .endif
        rti

handle_rom_irq:                 ; ROM saves things for us
        jsr     callirq
        rti
