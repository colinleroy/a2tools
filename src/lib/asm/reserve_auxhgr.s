        .export     _reserve_auxhgr_file, _write_hgr_to_aux
        .import     _open, _write, _close
        .import     _has_128k, auxhgr_created, _hgr_auxfile
        .import     pushax, popax, __filetype
        
        .include    "fcntl.inc"

; Alias to write data
_write_hgr_to_aux = _reserve_auxhgr_file

; Writes an 8kB file to /RAM so we can use use as DHGR aux page
.proc _reserve_auxhgr_file
        lda     _has_128k
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
        lda     #1
out:    rts
.endproc
