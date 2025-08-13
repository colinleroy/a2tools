        .export _hgr_baseaddr_l
        .export _hgr_baseaddr_h
        .export _div7_table
        .export _mod7_table
        .export _centered_div7_table
        .export _centered_mod7_table

        .segment "BSS"

.align 256
_hgr_baseaddr_l:      .res 192

filler_a:             .res 256-192-12
.assert <(*+12) = $00, error 
_div7_table:          .res 280
_hgr_baseaddr_h:      .res 192
filler_b:             .res 256-192-12-12
.assert <(*+12) = $00, error 
_mod7_table:       .res 280

_centered_div7_table = _div7_table + 12
_centered_mod7_table = _mod7_table + 12
