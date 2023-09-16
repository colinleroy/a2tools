; Colin Leroy-Mira, 2023
;
; Cleanup once-used functions
;

        .export         _clean_rt_once

        .import         __RT_ONCE_LOAD__, __RT_ONCE_SIZE__    ; Linker generated
        .import         ___heapadd, pushax

_clean_rt_once:
        ldx             #>__RT_ONCE_LOAD__
        lda             #<__RT_ONCE_LOAD__
        jsr             pushax
        ldx             #>__RT_ONCE_SIZE__
        lda             #<__RT_ONCE_SIZE__
        jsr             ___heapadd
        rts
