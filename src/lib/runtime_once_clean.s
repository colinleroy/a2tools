; Colin Leroy-Mira, 2023
;
; Cleanup once-used functions
;

        .export         _runtime_once_clean

        .import         __RT_ONCE_LOAD__, __RT_ONCE_SIZE__    ; Linker generated
        .import         ___bzero, ___heapadd, pushax

_runtime_once_clean:
        lda             _runtime_once_cleaned
        bne             clean_done
        ldx             #>__RT_ONCE_LOAD__
        lda             #<__RT_ONCE_LOAD__
        jsr             pushax
        ldx             #>__RT_ONCE_SIZE__
        lda             #<__RT_ONCE_SIZE__
        jsr             ___bzero

        ldx             #>__RT_ONCE_LOAD__
        lda             #<__RT_ONCE_LOAD__
        jsr             pushax
        ldx             #>__RT_ONCE_SIZE__
        lda             #<__RT_ONCE_SIZE__
        jsr             ___heapadd

        ldx             #1
        stx             _runtime_once_cleaned
clean_done:
        rts

        .data

_runtime_once_cleaned:  .byte 0
