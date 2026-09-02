;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                                               ;
; LOADER.SYSTEM - an Apple][ ProDOS 8 loader for cc65 programs (Oliver Schmidt) ;
;                                                                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

A1L             := $3C
A1H             := $3D
STACK           := $0100
BUF             := $0200
PATHNAME        := $0280
MLI             := $BF00
VERSION         := $FBB3
RDKEY           := $FD0C
PRBYTE          := $FDDA
COUT            := $FDED

QUIT_CALL          = $65
GET_FILE_INFO_CALL = $C4
OPEN_CALL          = $C8
READ_CALL          = $CA
CLOSE_CALL         = $CC
FILE_NOT_FOUND_ERR = $46

; Decompressor variables
offset_hi = $80
bitr      = $81
ZX0_src   = $82
ZX0_dst   = $84
pntr      = $86

; ------------------------------------------------------------------------

        .import __CODE_0300_SIZE__, __DATA_0300_SIZE__
        .import __CODE_0300_LOAD__, __CODE_0300_RUN__

; ------------------------------------------------------------------------

        .segment        "DATA_2000"

GET_FILE_INFO_PARAM:
                .byte   $0A             ;PARAM_COUNT
                .addr   PATHNAME        ;PATHNAME
                .byte   $00             ;ACCESS
                .byte   $00             ;FILE_TYPE
FILE_INFO_ADDR: .word   $0000           ;AUX_TYPE
                .byte   $00             ;STORAGE_TYPE
FILE_BLOCKS:    .word   $0000           ;BLOCKS_USED
                .word   $0000           ;MOD_DATE
                .word   $0000           ;MOD_TIME
                .word   $0000           ;CREATE_DATE
                .word   $0000           ;CREATE_TIME

OPEN_PARAM:
                .byte   $03             ;PARAM_COUNT
                .addr   PATHNAME        ;PATHNAME
                .addr   MLI - 1024      ;IO_BUFFER
OPEN_REF:       .byte   $00             ;REF_NUM

LOADING:
                .byte   $0D
                .asciiz "LOADING "

ELLIPSES:
                .byte   "...", $0D, $0D, $00

; ------------------------------------------------------------------------

        .segment        "DATA_0300"

READ_PARAM:
                .byte   $04             ;PARAM_COUNT
READ_REF:       .byte   $00             ;REF_NUM
READ_ADDR:      .addr   $0000           ;DATA_BUFFER
                .word   $FFFF           ;REQUEST_COUNT
                .word   $0000           ;TRANS_COUNT

CLOSE_PARAM:
                .byte   $01             ;PARAM_COUNT
CLOSE_REF:      .byte   $00             ;REF_NUM

QUIT_PARAM:
                .byte   $04             ;PARAM_COUNT
                .byte   $00             ;QUIT_TYPE
                .word   $0000           ;RESERVED
                .byte   $00             ;RESERVED
                .word   $0000           ;RESERVED

FINAL_START_ADDR:
                .addr   $0000

FILE_NOT_FOUND:
                .asciiz "... FILE NOT FOUND"

ERROR_NUMBER:
                .asciiz "... ERROR $"

PRESS_ANY_KEY:
                .asciiz " - PRESS ANY KEY "

; ------------------------------------------------------------------------

        .segment        "CODE_2000"

        jmp     :+
        .byte   $EE
        .byte   $EE
        .byte   $7F
PARAMS: .res    $7F

        ; Reset stack
:       ldx     #$FF
        txs

        ; Remove ".SYSTEM" from pathname
        lda     PATHNAME
        sec
        sbc     #.strlen(".SYSTEM")
        sta     PATHNAME

        ; Add trailing '\0' to pathname
        tax
        lda     #$00
        sta     PATHNAME+1,x

        ; Copy parameters and trailing '\0' to stack
        ; 
        ldx     PARAMS
        beq     finish_copy
        ldx     #$FF
:       inx
        lda     PARAMS+1,x
        beq     execname_copied
        cmp     #' '
        beq     execname_copied
        sta     PATHNAME+1,x
        bne     :-
execname_copied:
        lda     #$00
        sta     PATHNAME+1,x
        stx     PATHNAME

        ; Move the rest of the parameters to STACK
        inx
finish_copy:
        ldy     #$00
:       lda     PARAMS+1,x
        sta     STACK,y
        beq     load_file
        inx
        iny
        bne     :-
        
load_file:
        ; Provide some user feedback
        lda     #<LOADING
        ldx     #>LOADING
        jsr     PRINT_2000
        lda     #<(PATHNAME+1)
        ldx     #>(PATHNAME+1)
        jsr     PRINT_2000
        lda     #<ELLIPSES
        ldx     #>ELLIPSES
        jsr     PRINT_2000

        jsr     MLI
        .byte   GET_FILE_INFO_CALL
        .word   GET_FILE_INFO_PARAM
        bcc     :+
        jmp     ERROR_2000

:       jsr     MLI
        .byte   OPEN_CALL
        .word   OPEN_PARAM
        bcc     opened
        brk
        jmp     ERROR_2000

opened:
        ; Now we don't need PATHNAME anymore and can relocate CODE_0300 and DATA_0300
        ; We expect to copy more than one full page and less than three.
        .assert (>(__CODE_0300_SIZE__ + __DATA_0300_SIZE__)) = 1, error
        ldx     #$00
:       lda     __CODE_0300_LOAD__,x
        sta     __CODE_0300_RUN__,x
        dex
        bne     :-
        ; and second page
        ldx     #<(__CODE_0300_SIZE__ + __DATA_0300_SIZE__)
        beq     moved
:       lda     __CODE_0300_LOAD__+256-1,x
        sta     __CODE_0300_RUN__+256-1,x
        dex
        bne     :-

moved:
        ; Copy file reference number
        lda     OPEN_REF
        sta     READ_REF
        sta     CLOSE_REF

        ; Blocks to bytes - caveat: limited to $FF blocks, which will be enough
        ; anyway as we can, globally, use $800-$BEFF.
        asl     FILE_BLOCKS   ; Blocks are 512 bytes

        lda     #<($BEFF-$200)
        sta     READ_ADDR
        sta     ZX0_src

        sec
        lda     #>($BEFF-$200)
        sbc     FILE_BLOCKS
        sta     READ_ADDR+1
        sta     ZX0_src+1

        ; It's high time to leave this place
        jmp     __CODE_0300_RUN__

; ------------------------------------------------------------------------

        .segment        "CODE_0300"

        ; Read compressed data
        jsr     MLI
        .byte   READ_CALL
        .word   READ_PARAM
        bcs     ERROR

        ; Close the file
        jsr     MLI
        .byte   CLOSE_CALL
        .word   CLOSE_PARAM
        bcs     ERROR

        ; Get uncompress start address from aux-type
        lda     FILE_INFO_ADDR
        ldx     FILE_INFO_ADDR+1
        ; Store it as destination for zx decompression,
        sta     ZX0_dst
        stx     ZX0_dst+1
        ; And remember it for jumping as both ZX0_dst and FILE_INFO_ADDR
        ; will be overwritten after decompression.
        sta     FINAL_START_ADDR
        stx     FINAL_START_ADDR+1

        ; Copy REM and startup filename to BASIC input buffer
        ldx     #$00
        lda     #$B2            ; REM token
        bne     :++             ; Branch always
:       inx
        lda     a:STACK-1,x
:       sta     BUF,x
        bne     :--

        ; We've loaded our compressed program.
        jsr     _decompress_zx02_direct

        ; Go for it ...
        jmp     (FINAL_START_ADDR)

PRINT_2000 = * - __CODE_0300_RUN__ + __CODE_0300_LOAD__
PRINT:
        sta     A1L
        stx     A1H
        ldy     #$00
:       lda     (A1L),y
        beq     :+
        ora     #$80
        jsr     COUT
        iny
        bne     :-              ; Branch always
:       rts

ERROR_2000 = * - __CODE_0300_RUN__ + __CODE_0300_LOAD__
ERROR:
        pha
        lda     #<ERROR_NUMBER
        ldx     #>ERROR_NUMBER
        jsr     PRINT
        pla
        jsr     PRBYTE
        lda     #<PRESS_ANY_KEY
        ldx     #>PRESS_ANY_KEY
        jsr     PRINT
        jsr     RDKEY
        jsr     MLI
        .byte   QUIT_CALL
        .word   QUIT_PARAM

.include "zx02_direct.s"
