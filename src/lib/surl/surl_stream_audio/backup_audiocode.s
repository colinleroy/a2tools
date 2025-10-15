        .export   _backup_restore_audiocode
        .import   _open, _read, _write, _close, _unlink
        .import   _AUDIO_CODE_START, _AUDIO_CODE_SIZE

        .import   _bzero, pushax, popax

        .destructor unlink_backup_audio, 25
        .importzp ptr1

        .include  "fcntl.inc"

.segment "CODE"

.proc _backup_restore_audiocode
        sta     ptr1
        stx     ptr1+1
        ldy     #$00
        lda     (ptr1),y

        cmp     #'w'
        beq     set_write
set_read:
        ldy     #<(O_RDONLY)
        lda     #<_read
        ldx     #>_read
        bne     patch_io
set_write:
        ldy     #<(O_WRONLY|O_CREAT)
        lda     #<_write
        ldx     #>_write
patch_io:
        sty     io_mode+1
        sta     io_func+1
        stx     io_func+2

        lda     #<audio_backup_file
        ldx     #>audio_backup_file
        jsr     pushax
io_mode:
        lda     #$FF              ; Patched with mode
        ldx     #$00
        jsr     pushax
        ldy     #$04
        jsr     _open
        cmp     #$FF
        beq     error

        jsr     pushax            ; For _close
        jsr     pushax
        lda     #<_AUDIO_CODE_START
        ldx     #>_AUDIO_CODE_START
        jsr     pushax
        lda     #<_AUDIO_CODE_SIZE
        ldx     #>_AUDIO_CODE_SIZE
io_func:
        jsr     $FFFF             ; Patched with _read or _write
        cmp     #<_AUDIO_CODE_SIZE
        bne     close_error
        cpx     #>_AUDIO_CODE_SIZE
        bne     close_error

        jsr     popax
        jmp     _close

close_error:
        jsr     popax
        jsr     _close
error:
        lda     #$FF
        tax
        rts

.endproc

.proc unlink_backup_audio
        lda     #<audio_backup_file
        ldx     #>audio_backup_file
        jmp     _unlink
.endproc

.data

audio_backup_file: .asciiz "/RAM/AUDIO.CODE"
.bss
