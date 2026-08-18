        .export     auxhgr_created
        .import     _unlink, _hgr_auxfile

        .destructor unlink_auxhgr_file

.proc unlink_auxhgr_file
        lda     auxhgr_created
        beq     out
        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        jmp     _unlink
out:    rts
.endproc

        .bss

auxhgr_created: .res 1
