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

.code

; A = number of level to load
.proc _load_level_data
        pha

        lda       #<level_name_template
        sta       filename
        lda       #>level_name_template
        sta       filename+1

        ; Set correct filename for level
        pla
        clc
        adc       #'A'
        sta       level_name_template+6

        ; Fallthrough to load_level_data
.endproc

.proc load_level_data
        lda      #<__LEVEL_SIZE__
        sta      size
        lda      #>__LEVEL_SIZE__
        sta      size+1

        lda      #<__HGR_START__
        sta      destination
        lda      #>__HGR_START__
        sta      destination+1

        ; Fallthrough to load_data
.endproc

.proc load_data
        ; Open file
        ; Set filetype
        lda     #$06          ; PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda     filename
        ldx     filename+1
        jsr     pushax

        lda     #<O_RDONLY
        ldx     #>O_RDONLY
        jsr     pushax  

        ldy     #$04          ; _open is variadic
        jsr     _open
        cmp     #$FF
        beq     load_err

        jsr     pushax        ; Push for read
        jsr     pushax        ; and for close

        ; Push destination pointer
        lda     destination
        ldx     destination+1
        jsr     pushax

        ; and size
        lda     size
        ldx     size+1
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
        lda       #<splash_name
        sta       filename
        lda       #>splash_name
        sta       filename+1

        jmp        load_level_data
.endproc

.proc _load_lowcode
        lda       #<lowcode_name
        sta       filename
        lda       #>lowcode_name
        sta       filename+1

        lda      #<__LOWCODE_START__
        sta      destination
        lda      #>__LOWCODE_START__
        sta      destination+1

        lda      #<__LOWCODE_SIZE__
        sta      size
        lda      #>__LOWCODE_SIZE__
        sta      size+1

        jmp      load_data
.endproc

        .bss

filename:    .res 2
destination: .res 2
size:        .res 2
        .data

lowcode_name:        .asciiz "lowcode"
splash_name:         .asciiz "splash"
level_name_template: .asciiz "level.X"
