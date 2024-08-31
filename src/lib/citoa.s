        .export       _citoa
        .import       _itoa
        .import       pushax
        .import       _cputs
        .import       ntoabuf

_citoa:
        jsr     pushax        ; Push number
        lda     #<ntoabuf
        ldx     #>ntoabuf

        jsr     pushax
        lda     #10
        ldx     #0

        jsr     _itoa

        lda     #<ntoabuf
        ldx     #>ntoabuf
        jsr     _cputs
out:
        rts
