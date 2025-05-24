        .export   _backup_restore_hgrpage
        .import   _open, _read, _write, _close, _unlink

        .import   _bzero, pushax, popax

        .destructor unlink_backup_hgrpage, 25
        .importzp ptr1

        .include  "fcntl.inc"

.segment "CODE"

.proc _backup_restore_hgrpage
        sta     ptr1
        stx     ptr1+1
        ldy     #$00
        lda     (ptr1),y

        pha                       ; Backup for later
        cmp     #'w'
        beq     set_write
set_read:
        lda     #<(O_RDONLY)
        sta     io_mode+1
        lda     #<_read
        ldx     #>_read
        bne     patch_io
set_write:
        lda     #<(O_WRONLY|O_CREAT)
        sta     io_mode+1
        lda     #<_write
        ldx     #>_write
patch_io:
        sta     io_func+1
        stx     io_func+2

        lda     #<hgr_backup_file
        ldx     #>hgr_backup_file
        jsr     pushax
io_mode:
        lda     #$FF              ; Patched with mode
        ldx     #$00
        jsr     pushax
        ldy     #$04
        jsr     _open
        cmp     #$FF
        bne     :+
        pla                       ; Open failed, pop
        cmp     #'r'              ; Fallback to clear page if read
        beq     no_ramcard_fallback
        rts

:
        jsr     pushax            ; For _close
        jsr     pushax
        lda     #<$2000
        ldx     #>$2000
        jsr     pushax
io_func:
        jsr     $FFFF             ; Patched with _read or _write

        pla                       ; Pop mode backup
        jsr     popax
        jmp     _close

no_ramcard_fallback:
        cmp     #'r'
        beq     :+
        rts
:       lda     #<$2000
        ldx     #>$2000
        jsr     pushax
        jmp     _bzero
.endproc

.proc unlink_backup_hgrpage
        lda     #<hgr_backup_file
        ldx     #>hgr_backup_file
        jmp     _unlink
.endproc

.data

hgr_backup_file: .asciiz "/RAM/BCK.HGR"
.bss
