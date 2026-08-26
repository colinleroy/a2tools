        .export         _sierra_read_packet

        .import         _sierra_reset, _resetting
        .import         _sierra_packet_type
        .import         _sierra_response_len
        .import         _sierra_response_continues
        .import         _simple_serial_read_no_irq
        .import         _buffer, _header, _footer

        .import         pushax, returnFFFF, _bzero

        .include        "sierra.inc"

        .segment "SIERRA"

; uint8 sierra_read_packet(void)
.proc _sierra_read_packet
        lda     #$03
        sta     tries
        lda     #$EE
        sta     _sierra_packet_type

        lda     #<_header
        ldx     #>_header
        jsr     pushax
        lda     #3
        ldx     #0
        jsr     _bzero

        lda     #<_footer
        ldx     #>_footer
        jsr     pushax
        lda     #2
        ldx     #0
        jsr     _bzero

        lda     #<_buffer
        ldx     #>_buffer
        jsr     pushax
        lda     #16
        ldx     #0
        jsr     _bzero

        lda     #<_sierra_packet_type
        ldx     #>_sierra_packet_type
        jsr     pushax
        lda     #1
        ldx     #0
        jsr     _simple_serial_read_no_irq
        cmp     #$00
        bne     out_err

        lda     _sierra_packet_type
        cmp     #SIERRA_PACKET_DATA
        beq     read_more
        cmp     #SIERRA_PACKET_DATA_END
        beq     read_more
        cmp     #SIERRA_PACKET_COMMAND
        beq     read_more
        cmp     #SIERRA_PACKET_SESSION_END
        beq     session_end
        ; We got a single byte, we're good
        lda     #$00
        tax
        rts
session_end:
        ; SESSION_END. Try again maybe?
        lda     _resetting
        bne     out_err
        dec     tries
        beq     out_err
        ; Reset,
        jsr     _sierra_reset
        lda     #SIERRA_PACKET_RETRY_INTERNAL
        sta     _sierra_packet_type
        ; and return -1 for caller to retry
out_err:
        jmp     returnFFFF

read_more:
        ; We got a data packet
        ; Update sierra_response_continues
        ldx     #$00
        cmp     #SIERRA_PACKET_DATA
        bne     :+
        inx
:       stx     _sierra_response_continues

        ; Read header now
        lda     #<_header
        ldx     #>_header
        jsr     pushax
        lda     #3
        ldx     #0
        jsr     _simple_serial_read_no_irq
        cmp     #$00
        bne     out_err

        ; Push buffer for reading
        lda     #<_buffer
        ldx     #>_buffer
        jsr     pushax

        ; Set response_len
        lda     _header+1
        ldx     _header+2
        sta     _sierra_response_len
        stx     _sierra_response_len+1

        ; and read data
        jsr     _simple_serial_read_no_irq
        cmp     #$00
        bne     out_err

        ; Now read checksum
        lda     #<_footer
        ldx     #>_footer
        jsr     pushax
        lda     #2
        ldx     #0
        jsr     _simple_serial_read_no_irq
        cmp     #$00
        bne     out_err
        
        ; We're good!
        tax
        rts
.endproc

        .segment "BSS"
tries:          .res 1
