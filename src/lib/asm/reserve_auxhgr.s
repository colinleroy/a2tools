        .export     _reserve_auxhgr_file, _write_hgr_to_aux
        .export     _can_dhgr
        .import     _open, _write, _close, _unlink
        .import     _has_128k, auxhgr_created, _hgr_auxfile
        .import     pushax, popax, __filetype
        
        .include    "apple2.inc"
        .include    "fcntl.inc"

; Alias to write data
_write_hgr_to_aux = _reserve_auxhgr_file

PRODOS_RAM_BLOCKS_BITMAP = $3C2

.segment "LC"

.proc check_auxhgr_free
        ; Unlink in case it existed
        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        jsr     _unlink

        ; Clear 80COL so we can read AUX
        lda     RD80COL
        pha
        sta     CLR80COL

        ldy     #$FF          ; Set result to free

        sta     $C003         ; RDCARDRAM
        lda     PRODOS_RAM_BLOCKS_BITMAP+1
        and     PRODOS_RAM_BLOCKS_BITMAP+2
        cmp     #$FF
        bne     busy
        lda     PRODOS_RAM_BLOCKS_BITMAP+3
        and     #$80
        bne     done
busy:
        iny
done:
        sta     $C002         ; RDMAINRAM
        pla                   ; Restore 80 cols
        bpl     :+
        sta     SET80COL
:       sty     _can_dhgr
        tya
        rts
.endproc

.segment "CODE"

; Writes an 8kB file to /RAM so we can use use as DHGR aux page
; Returns 0 on failure!
.proc _reserve_auxhgr_file
        lda     _has_128k
        beq     out

        jsr     check_auxhgr_free
        beq     out

        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype

        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        jsr     pushax
        lda     #<(O_WRONLY|O_CREAT)
        ldx     #>(O_WRONLY|O_CREAT)
        jsr     pushax
        ldy     #4
        jsr     _open

        cmp     #$00
        beq     out

        jsr     pushax        ; Push for write
        jsr     pushax        ; and close

        lda     #<$2000       ; Use HGR page as source of dummy data
        ldx     #>$2000
        jsr     pushax

        ; and len is $2000 already
        jsr     _write

        ; Note we created the file
        lda     #$01
        sta     auxhgr_created

        ; and close
        jsr     popax
        jsr     _close
        lda     #$01
out:    ldx     #$00
        rts
.endproc

.segment "BSS"
_can_dhgr:      .res 1
