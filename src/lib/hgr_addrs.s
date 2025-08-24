        .export _hgr_baseaddr_l
        .export _hgr_baseaddr_h

        .segment "BSS"

.align 256
_hgr_baseaddr_l:      .res 192
.align 256
_hgr_baseaddr_h:      .res 192
