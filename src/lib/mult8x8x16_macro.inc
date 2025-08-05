        .import         sqrLow, sqrHigh, iSqrLow, iSqrHigh

; Multiply A by X, result in (h:A, l:Y)
.macro MULT_AX_YA
.scope
        sta sm1           ; 4
        sta sm3           ; 8
        eor #$ff          ; 10
        sta sm2           ; 14
        sta sm4           ; 18

        sec               ; 20
sm1 = * + 1
        lda sqrLow,x      ; 24
sm2 = * + 1
        sbc iSqrLow,x     ; 28
        tay               ; 30
sm3 = * + 1
        lda sqrHigh,x     ; 34
sm4 = * + 1
        sbc iSqrHigh,x    ; 38
.endscope
.endmacro

; Multiply A by X, result in (h:X, l:A)
.macro MULT_AX
.scope
        sta sm1           ; 4
        sta sm3           ; 8
        eor #$ff          ; 10
        sta sm2           ; 14
        sta sm4           ; 18

        sec               ; 20
sm1 = * + 1
        lda sqrLow,x      ; 24
sm2 = * + 1
        sbc iSqrLow,x     ; 28
        tay               ; 30
sm3 = * + 1
        lda sqrHigh,x     ; 34
sm4 = * + 1
        sbc iSqrHigh,x    ; 38
        tax               ; 40
        tya               ; 42
.endscope
.endmacro

; Multiply A by X, result in (h:X, l:LSTORE (or nowhere if no param))
.macro MULT_AX_STORE_LOW LSTORE
.scope
        sta sm1           ; 4
        sta sm3           ; 8
        eor #$ff          ; 10
        sta sm2           ; 14
        sta sm4           ; 18

        sec               ; 20
sm1 = * + 1
        lda sqrLow,x      ; 24
sm2 = * + 1
        sbc iSqrLow,x     ; 28
        .if .paramcount = 1
        sta LSTORE        ; 32
        .endif
sm3 = * + 1
        lda sqrHigh,x     ; 36
sm4 = * + 1
        sbc iSqrHigh,x    ; 40
.endscope
.endmacro

; Multiply A by X, result in (l:A, not calculating high byte)
.macro MULT_AX_NO_HIGH
.scope
        sta sm1           ; 4
        eor #$ff          ; 10
        sta sm2           ; 14

        sec               ; 20
sm1 = * + 1
        lda sqrLow,x      ; 24
sm2 = * + 1
        sbc iSqrLow,x     ; 28
.endscope
.endmacro
