;
; Oliver Schmidt, 2012-11-17
; Colin Leroy-Mira, 2023-09-02 - Handle IRQs without ProDOS
;
; IRQ handling (Apple2 version)
;

        .export         _init_fast_irq, _done_fast_irq
        .import         callirq
        .importzp       _a_backup, _prev_ram_irq_vector, _prev_rom_irq_vector
        .constructor    _init_fast_irq, 9
        .destructor     _done_fast_irq, 9

        .include        "apple2.inc"

; ------------------------------------------------------------------------



ROMIRQVEC:= $03fe
RAMIRQVEC:= $fffe

        .segment        "RT_ONCE"

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

        .segment        "LOWCODE"

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
        rts

; ------------------------------------------------------------------------

handle_ram_irq:
        sta     _a_backup       ; Save A
        pla                     ; Check for BRK
        pha
        and     #%00010000       ; Check bit 4 (BRK flag)
        bne     do_brk

        ; It's an IRQ

.ifdef __APPLE2ENH__
        phx                     ; Save X,Y
        phy
        jsr     callirq
        bcc     give_back_irq
        ply                     ; Restore Y,X,A
        plx
        lda     _a_backup
        rti
give_back_irq:
        ply                     ; Restore Y,X,A
        plx
        lda     _a_backup
        jmp     (_prev_ram_irq_vector)

.else
        txa                     ; Save X,Y
        pha
        tya
        pha
        jsr     callirq
        bcc     give_back_irq
        pla                     ; Restore Y,X,A
        tay
        pla
        tax
        lda     _a_backup
        rti
give_back_irq:
        pla                     ; Restore Y,X,A
        tay
        pla
        tax
        lda     _a_backup
        jmp     (_prev_ram_irq_vector)

.endif

do_brk:
        ; Give BRK to the standard handler
        jmp     (_prev_ram_irq_vector)

handle_rom_irq:                 ; ROM saves things for us
        jsr     callirq
        bcs     handled
        jmp     (_prev_rom_irq_vector)
handled:
        rti
