        .export       _cutoa
        .import       _utoa
        .import       pushax
        .import       _cputs
        .import       ntoabuf

_cutoa:
        jsr     pushax        ; Push number
        lda     #<ntoabuf
        ldx     #>ntoabuf

        jsr     pushax
        lda     #10
        ldx     #0

        jsr     _utoa

        lda     #<ntoabuf
        ldx     #>ntoabuf
        jsr     _cputs
out:
        rts
