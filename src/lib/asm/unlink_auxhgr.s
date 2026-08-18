        .export     auxhgr_created, _unlink_auxhgr_file

.ifdef AUXHGR_NO_AUTO_REMOVE
        .export     _auxhgr_keep
.endif

        .import     _unlink, _hgr_auxfile

        .destructor _unlink_auxhgr_file

.proc _unlink_auxhgr_file
        lda     auxhgr_created
        beq     out
.ifdef AUXHGR_NO_AUTO_REMOVE
        lda     _auxhgr_keep
        bne     out
.endif
        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        jmp     _unlink
out:    rts
.endproc

        .bss

auxhgr_created: .res 1
.ifdef AUXHGR_NO_AUTO_REMOVE
_auxhgr_keep:   .res 1
.endif
