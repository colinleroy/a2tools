        .export     auxhgr_created
        .import     _unlink, _hgr_auxfile

.ifndef AUXHGR_NO_AUTO_REMOVE
        .destructor unlink_auxhgr_file

.proc unlink_auxhgr_file
        lda     auxhgr_created
        beq     out
        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        jmp     _unlink
out:    rts
.endproc
.endif

        .bss

auxhgr_created: .res 1
