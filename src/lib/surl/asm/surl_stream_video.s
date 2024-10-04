;
; Colin Leroy-Mira <colin@colino.net>, 2024
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

        .importzp             ptr1, _zp6, _zp7, _zp8, _zp10

        .import               _serial_read_byte_no_irq
        .import               _simple_serial_setup_no_irq_regs
        .import               _simple_serial_set_irq
        .import               _simple_serial_flush
        .import               _serial_putc_direct
        .import               _sleep

.ifdef IIGS
        .import         zilog_status_reg_r, zilog_data_reg_r
.else
        .import         acia_status_reg_r, acia_data_reg_r
.endif

        .export               _surl_stream_video

.ifdef IIGS
serial_status_reg = zilog_status_reg_r
serial_data_reg   = zilog_data_reg_r
HAS_BYTE          = $01
.else
serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
HAS_BYTE          = $08
.endif

KBD          := $C000   ; Read keyboard
KBDSTRB      := $C010   ; Clear keyboard strobe

MAX_OFFSET    = 126
N_BASES       = (8192/MAX_OFFSET)+1

.ifdef DOUBLE_BUFFER
PAGE1_HB      = $20
PAGE2_HB      = $40
.else
PAGE1_HB      = $20
PAGE2_HB      = PAGE1_HB
.endif

page1_addr_ptr= _zp8
page2_addr_ptr= _zp10
page          = _zp6
last_offset   = _zp7

.proc _surl_stream_video: near
        php
        sei                     ; Disable all interrupts

        ; Setup
        lda     #$00
        sta     page
        sta     stop

        ; Patch serial registers
        lda     serial_status_reg+1
        sta     ss0+1
        sta     ss1+1
        sta     ss2+1
        sta     ss3+1
        lda     serial_status_reg+2
        sta     ss0+2
        sta     ss1+2
        sta     ss2+2
        sta     ss3+2

        lda     serial_data_reg+1
        sta     sd0+1
        sta     sd1+1
        sta     sd2+1
        sta     sd3+1
        lda     serial_data_reg+2
        sta     sd0+2
        sta     sd1+2
        sta     sd2+2
        sta     sd3+2

        ; Calculate bases for HGR page 1
        lda     #<(page1_addrs_arr)
        ldy     #>(page1_addrs_arr)
        sta     page1_addr_ptr
        sty     page1_addr_ptr+1
        ldx     #PAGE1_HB
        jsr     calc_bases

        ; Calculate bases for HGR page 2
        lda     #<(page2_addrs_arr)
        ldy     #>(page2_addrs_arr)
        sta     page2_addr_ptr
        sty     page2_addr_ptr+1
        ldx     #PAGE2_HB
        jsr     calc_bases

        ; Shutdown IRQs, inform server we're ready
        lda     #$00
        jsr     _simple_serial_set_irq

        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct

        jsr     _serial_read_byte_no_irq
        cmp     #$27            ; SURL_ANSWER_STREAM_START
        beq     :+
        lda     #$FF            ; Server error
        tax
        plp
        rts

:       bit     $C052           ; Clear HGR mix
        ldx     #$00            ; Start offset
        stx     last_offset
        lda     #($00|$80)      ; Start base

loop:
        ; Main loop - either we get a positive value and it's
        ; a block of pixels to store at the current offset, or
        ; it's a negative value, in which case it's either an
        ; offset between 0 and MAX_OFFSET, or $FF, to indicate
        ; there's a value to repeat N times incoming.
        ldy     #$01            ; Set repetitions to 1

ss0:    lda     $FFFF           ; serial status
        and     #HAS_BYTE
        beq     ss0
sd0:    lda     $FFFF           ; serial data (14 cycles since loop)

        bmi     store_dest      ; It's a value, store it
        cmp     #$7F            ; Is it a rep?
        bne     set_offset      ; No, so it's an offset

do_rep:
        ; handle repetitions: get N,
ss1:    lda     $FFFF           ; serial status
        and     #HAS_BYTE
        beq     ss1
sd1:    ldy     $FFFF           ; serial data

        ; and get value to repeat
ss2:    lda     $FFFF           ; serial status
        and     #HAS_BYTE
        beq     ss2
sd2:    lda     $FFFF           ; serial data

store_dest:
        sta     $FFFF,x
        inx
        dey                     ; Y is N repetitions
        bne    store_dest
        jmp    loop             ; Back to main loop

set_offset:
        and     #%01111111      ; Get rid of sign
        tax                     ; set offset

        cpx     last_offset     ; If offset diminished, presume we're
        bcs     :+              ; going to next base.

        lda     store_dest+1    ; Increment base savagely for less cycles
        adc     #(MAX_OFFSET)
        sta     store_dest+1
        bcc     :+
        inc     store_dest+2

:       stx     last_offset
        ; Read next byte. It will either be positive, in which case
        ; it's a value, or negative, in which case it's either a new
        ; base that is further away than the one we just calculated,
        ; or $FF, indicating a repetition of a value.
ss3:    lda     $FFFF           ; serial status
        and     #HAS_BYTE
        beq     ss3
sd3:    lda     $FFFF           ; serial data
        bmi     store_dest      ; It's a value

set_base:                       ; We have a new base (or rep maybe)
        cmp     #$7F            ; Is it a rep? It can be.
        beq     do_rep          ; Go handle repetition

        asl     a               ; Shift for array index

        bne     :+              ; If base is 0, it's a new frame.
        jsr     frame_done      ; In which case, sync point.

        lda     stop            ; Did user press ESC?
        beq     :+
        jmp     cleanup

:       cmp     #((N_BASES+2)*2); Base not 0. Is it the end of stream?
        bne     :+
        jmp     cleanup

:       tay
page_ptr_a:
        ; Update where to store pixels. Both these pointers are patched
        ; when switching page.
        lda     (page1_addr_ptr),y
        sta     store_dest+1
        iny
page_ptr_b:
        lda     (page1_addr_ptr),y
        sta     store_dest+2
        ; Done with our new base, back to main loop
        jmp     loop

calc_bases:
        ; Precalculate addresses inside pages, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains the base page address's high byte on entry ($20 for
        ; page 1, $40 for page 2)
        sta     ptr1
        sty     ptr1+1

        ldy     #0              ; Y is the index - Start at base 0
        lda     #$00            ; A is the address's low byte
                                ; (and X the address's high byte)

        clc
calc_next_base:
        sta     (ptr1),y        ; Store AX
        iny
        pha
        txa
        sta     (ptr1),y
        pla
        iny

        adc     #(MAX_OFFSET)   ; Compute next base
        bcc     :+
        inx
        clc
:       cpy     #(N_BASES*2)
        bcc     calc_next_base
        rts

frame_done:
        ; New frame begins. We'll switch page, check keyboard input,
        ; send status to server.
        ldx     page
.ifdef DOUBLE_BUFFER
        lda     $C054,x         ; Switch displayed page
.endif
        lda     page_addr_ptr,x ; Update pointers in set_base
        sta     page_ptr_a+1
        sta     page_ptr_b+1
        txa
        eor     #$01            ; Toggle page for next time
        sta     page

        ldx     #$00            ; X (offset) is supposed to be 0 at frame start

        lda     KBD             ; Did user press a key ?
        bmi     :+
        rts
:       and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     handle_kbd      ; Send pressed key to server

handle_kbd:
        cmp     #' '            ; Space to pause ?
        bne     :++

        jsr     _serial_putc_direct
:       lda     KBD             ; Wait for another keypress to resume.
        bpl     :-
        bit     KBDSTRB
        lda     #$00

:       jsr     _serial_putc_direct
        cmp     #$1B            ; Escape to quit ?
        bne     :+
        sta     stop

:       lda     #$00            ; Main loop expects A 0 there
        rts                     ; Return to main loop

cleanup:
        lda     #$01
        ldx     #$00
        jsr     _sleep

        ; We're all done! Re-enable IRQs, and return.
        plp
        lda     #$01
        jsr     _simple_serial_set_irq
        jsr     _simple_serial_flush
        lda     #$00
        tax
        rts
.endproc

        .data

page_addr_ptr:  .byte <(page2_addr_ptr)   ; Base addresses pointer for page 2
                .byte <(page1_addr_ptr)   ; Base addresses pointer for page 1

        .bss
page1_addrs_arr:.res (N_BASES*2)          ; Base addresses arrays
page2_addrs_arr:.res (N_BASES*2)
stop:           .res 1
