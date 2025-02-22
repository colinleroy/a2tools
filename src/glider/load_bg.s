        .export  _load_bg
        .import  cur_level
        .import  _open, _read, _close, _memcpy
        .import  pushax, popax
        .import  __filetype, __auxtype
        
        .include "apple2.inc"
        .include "fcntl.inc"

_load_bg:
        ; Set correct filename for level
        lda       cur_level
        clc
        adc       #'A'
        sta       bgname+5

        ; Open file
        ; Set filetype  
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda       #<bgname
        ldx       #>bgname
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

        lda     #<$2000
        ldx     #>$2000
        jsr     pushax        ; Push destination pointer (HGR page)

        ; Size is $2000 too
        jsr     _read

        jsr     popax         ; Get fd back
        jsr     _close

        ; Copy background to HGR page 2
        lda     #<$4000
        ldx     #>$4000
        jsr     pushax
        lda     #<$2000
        ldx     #>$2000
        jsr     pushax
        lda     #<$2000
        ldx     #>$2000
        jsr     _memcpy

load_err:
        rts

        .data

bgname: .asciiz "levelX.hgr"
