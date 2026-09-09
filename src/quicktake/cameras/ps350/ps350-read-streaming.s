        .export  _ps350_read_dir_list
        .export  _ps350_read_file
        .import  _buffer
        .import  _ps350_get_eot_and_ack
        .import  _ps350_send_packet
        .import  _serial_read_byte_direct
        .import  _is_multi
        .import  _found_ent
        .import  _ent_name
        .import  _ent_size
        .import  _cur_ent
        .export  _read_to_get_ent
        .export  _ent_to_get

.segment "PS350"

FIRST_HEADER_SIZE     = 41
NEXT_HEADERS_SIZE     = 5
FOOTER_SIZE           = 3
DATABUF_SIZE          = $2000
PS350_PKT_LEN         = 300
DATA_SIZE_FIRST_BLOCK = (PS350_PKT_LEN-FIRST_HEADER_SIZE-FOOTER_SIZE)
DATA_SIZE_NEXT_BLOCKS = (PS350_PKT_LEN-NEXT_HEADERS_SIZE-FOOTER_SIZE)
NUM_MULTIPACKETS      = ((DATABUF_SIZE-256)/292)
READ_BLOCK_MAX_SIZE   = (DATA_SIZE_FIRST_BLOCK + (NUM_MULTIPACKETS-1)*DATA_SIZE_NEXT_BLOCKS)
READ_BLOCK_SIZE       = (READ_BLOCK_MAX_SIZE-(READ_BLOCK_MAX_SIZE .mod 256))
PS350_LEN_IDX         = 3

_rem_bytes_in_pack: .res 2,$00
_offset_in_pack:    .res 1,$00
_read_to_get_ent:   .res 1,$00
_ent_to_get:        .res 1,$00

; Warning - very fast serial reader - but can't read more than 255 bytes at once
; Input: AX, pointer to store to; Y, number of bytes to read.
; Returns void. Does not timeout.
.proc read_to_buffer
        lda     #<_buffer
        ldx     #>_buffer
.endproc
.proc read_to_dest
        sta     bufdest
        stx     bufdest+1
.endproc
.proc read_to_preset_dest
        ldx     #$00
read_again:
        jsr     _serial_read_byte_direct
        bcs     read_again
dest = *+1
        sta     _buffer,x
        inx
        dey
        bne     read_again
        rts
.endproc
bufdest = read_to_preset_dest::dest

.proc _get_packet_length: near
        ldy     #$05                    ; Read 5 first bytes of packet
        jsr     read_to_buffer
        lda     _buffer+4
        and     #$80                    ; and check if multi-packet (len MSB has high bit set)
        sta     _is_multi
        rts
.endproc

.proc _ps350_read_dir_list
        jsr     _ps350_send_packet      ; Send the command,
        jsr     _get_packet_length      ; Read (part of) the length to check for multi-packet response

        ldy     #22                     ; Skip 22 bytes about which we don't care
        jsr     read_to_buffer

        lda     #27                     ; We're here. That's where the first entity starts.
        sta     _offset_in_pack

        ldx     #$00                    ; Init searcher var
        stx     _found_ent

packet_loop:                            ; Loop until last (non-multi) packet
        lda     #<299                   ; Compute how many more bytes in the packet
        sec
        sbc     _offset_in_pack
        sta     _rem_bytes_in_pack
        lda     #>299
        sbc     #$00
        sta     _rem_bytes_in_pack+1

file_loop:                              ; Loop for each entry
        ldy     #1                      ; Get type (0x10 = dir, 0x20 = file, 0x00 = done)
        jsr     read_to_buffer

        lda     _buffer                 ; Are we done?
        beq     file_loop_done

        lda     #<_buffer               ; Read size at the correct place
        ldx     #>_buffer
        ldy     _read_to_get_ent        ; Check if should remember name (if search for an entity by index)
        beq     skip_ent_size
        ldy     _found_ent              ; Already found so don't overwrite its name
        bne     skip_ent_size
        lda     #<_ent_size             ; Read size
        ldx     #>_ent_size
skip_ent_size:
        ldy     #4
        jsr     read_to_dest

        ldy     #4                      ; Skip date
        jsr     read_to_buffer

        lda     _read_to_get_ent        ; Check if should remember name (if search for an entity by index)
        beq     skip_ent_name
        lda     _found_ent              ; Already found so don't overwrite its name
        bne     skip_ent_name

        lda     #<_ent_name             ; Not found yet, so store to ent_name...
        ldx     #>_ent_name
        ldy     #12
        jsr     read_to_dest
        lda     #$00                    ; Zero-terminate
        sta     _ent_name+12

        lda     _ent_name               ; Verify if it's valid (starts with [A-Z])
        cmp     #'A'
        bcc     dec_and_cont
        cmp     #('Z'+1)
        bcs     dec_and_cont
        cmp     #'T'
        beq     dec_and_cont

        lda     _cur_ent                ; Check if that's the one we need
        cmp     _ent_to_get
        bne     count_ent

        lda     #$01                    ; If so, mark found
        sta     _found_ent
        jmp     count_ent

skip_ent_name:                          ; Already found so don't overwrite ent_name
        ldy     #12
        jsr     read_to_buffer

        lda     _buffer                 ; Check validity of entity for counting
        cmp     #'A'
        bcc     dec_and_cont
        cmp     #('Z'+1)
        bcs     dec_and_cont
        cmp     #'T'
        beq     dec_and_cont

; cur_ent++;
;
count_ent:
        inc     _cur_ent

dec_and_cont:                           ; One entity, 21 bytes, done.
        lda     _rem_bytes_in_pack
        sec
        sbc     #$15
        sta     _rem_bytes_in_pack
        bcs     file_loop
        dec     _rem_bytes_in_pack+1

        jmp     file_loop

file_loop_done:
        lda     _rem_bytes_in_pack+1    ; Skip unused packet bytes
        beq     :+
        ldy     #0                      ; Can't read more than one page at once, so read one page
        jsr     read_to_buffer
:       ldy     _rem_bytes_in_pack      ; And the remainder
        jsr     read_to_buffer

        lda     _is_multi               ; If not multi, we're done
        beq     ret_all_done

        jsr     _get_packet_length      ; Otherwise get new packet header,
        lda     #$05                    ; in continuation packets, data starts at offset 0x05
        sta     _offset_in_pack

        jmp     packet_loop             ; And go handle packet.

ret_all_done:                           ; All done, time to ack.
        jmp     _ps350_get_eot_and_ack
.endproc

cont:   .res 1

.proc set_cont
        lda     read_to_preset_dest::dest
        ldx     read_to_preset_dest::dest+1
        sta     check_cont
        stx     check_cont+1
check_cont = *+1
        lda     $FFFF,y
        and     #$80
        sta     cont
        rts
.endproc

; Input: AX destination buffer
.proc _ps350_read_file
        sta     read_to_preset_dest::dest
        stx     read_to_preset_dest::dest+1

        jsr     _ps350_send_packet      ; Send the command,

        ; Get first header
        ldy     #FIRST_HEADER_SIZE
        jsr     read_to_preset_dest

        ; Get cont flag
        ldy     #PS350_LEN_IDX+1
        jsr     set_cont

        ; Read first page
        .assert DATA_SIZE_FIRST_BLOCK = 256, error
        ldy     #<256
        jsr     read_to_preset_dest

        ; buffer_walker += DATA_SIZE_FIRST_BLOCK;
        inc     read_to_preset_dest::dest+1

        ; while cont
next_packet:
        lda     cont
        beq     done

        ; Skip footer/next header
        ldy     #FOOTER_SIZE+NEXT_HEADERS_SIZE
        jsr     read_to_preset_dest

        ldy     #FOOTER_SIZE+PS350_LEN_IDX+1
        jsr     set_cont

        ldy     #<256
        jsr     read_to_preset_dest

        ; Get and store data block
        inc     read_to_preset_dest::dest+1
        ldy     #<(DATA_SIZE_NEXT_BLOCKS-256)
        jsr     read_to_preset_dest

        ; And increment dest
        clc
        lda     #<(DATA_SIZE_NEXT_BLOCKS-256)
        adc     read_to_preset_dest::dest
        sta     read_to_preset_dest::dest
        lda     #>(DATA_SIZE_NEXT_BLOCKS-256)
        adc     read_to_preset_dest::dest+1
        sta     read_to_preset_dest::dest+1
        bne     next_packet             ; eq. jmp here

done:
        ldy     #<FOOTER_SIZE
        jsr     read_to_preset_dest

        jmp     _ps350_get_eot_and_ack
.endproc
