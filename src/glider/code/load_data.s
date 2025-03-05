        .export  _load_level_data, _load_splash_screen, _load_lowcode
        .import  cur_level
        .import  _open, _read, _close, _memcpy
        .import  pushax, popax
        .import  __filetype, __auxtype
        .import  _strcpy
        .import  __LOWCODE_START__, __LOWCODE_SIZE__
        .import  __HGR_START__, __LEVEL_SIZE__

        .include "apple2.inc"
        .include "fcntl.inc"

.segment "LOWCODE"

; A = number of level to load
.proc _load_level_data
        pha

        lda       #<_level_name
        ldx       #>_level_name
        jsr       pushax
        lda       #<_level_name_template
        ldx       #>_level_name_template
        jsr       _strcpy

        ; Set correct filename for level
        pla
        clc
        adc       #'A'
        sta       _level_name+6
        ; Fallthrough to load_data_to_hgr
.endproc

.proc load_data_to_hgr
        ; Open file
        ; Set filetype
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda       #<_level_name
        ldx       #>_level_name
        jsr       pushax

        lda       #<O_RDONLY
        ldx       #>O_RDONLY
        jsr     pushax  

        ldy     #$04          ; _open is variadic
        jsr     _open
        cmp     #$FF
        beq     load_err

        jsr     pushax        ; Push for read
        jsr     pushax        ; and for close

        lda     #<__HGR_START__
        ldx     #>__HGR_START__
        jsr     pushax        ; Push destination pointer (HGR page)

        ; Size is $2100
        lda     #<__LEVEL_SIZE__
        ldx     #>__LEVEL_SIZE__
        jsr     _read

        jsr     popax         ; Get fd back
        jsr     _close
        clc
        rts

load_err:
        sec
        rts
.endproc

.proc _load_splash_screen
        lda       #<_level_name
        ldx       #>_level_name
        jsr       pushax
        lda       #<_splash_name
        ldx       #>_splash_name
        jsr       _strcpy

        jmp        load_data_to_hgr
.endproc

.code

.proc _load_lowcode
        ; Open file
        ; Set filetype
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda       #<_lowcode_name
        ldx       #>_lowcode_name
        jsr       pushax

        lda       #<O_RDONLY
        ldx       #>O_RDONLY
        jsr     pushax  

        ldy     #$04          ; _open is variadic
        jsr     _open
        cmp     #$FF
        beq     load_err

        jsr     pushax        ; Push for read
        jsr     pushax        ; and for close

        lda     #<__LOWCODE_START__
        ldx     #>__LOWCODE_START__
        jsr     pushax        ; Push destination pointer (HGR page)

        lda     #<__LOWCODE_SIZE__
        ldx     #>__LOWCODE_SIZE__
        jsr     _read

        jsr     popax         ; Get fd back
        jsr     _close
        clc
        rts

load_err:
        sec
        rts
.endproc

        .bss

_level_name: .res 12

        .data

_lowcode_name:        .asciiz "lowcode.bin"
_splash_name:         .asciiz "splash.bin"
_level_name_template: .asciiz "level.X.bin"
