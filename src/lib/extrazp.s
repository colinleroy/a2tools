;
; Ullrich von Bassewitz, 06.08.1998
; Colin Leroy-Mira <colin@colino.net>, 2023
;
        .export         _a_backup, _prev_ram_irq_vector, _prev_rom_irq_vector
        .export         _zp2, _zp3, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12, _zp13
        .export         _zp2s, _zp3s, _zp4s, _zp5s, _zp6s, _zp7s, _zp8s, _zp9s, _zp10s, _zp11s, _zp12s, _zp13s
        .export         _zp2p, _zp4p, _zp6p, _zp8p, _zp10p, _zp12p
        .export         _zp2ip, _zp4ip, _zp6ip, _zp8ip, _zp10ip, _zp12ip
        .export         _zp2sp, _zp4sp, _zp6sp, _zp8sp, _zp10sp, _zp12sp
        .export         _zp2sip, _zp4sip, _zp6sip, _zp8sip, _zp10sip, _zp12sip
        .export         _zp2i, _zp4i, _zp6i, _zp8i, _zp10i, _zp12i
        .export         _zp2si, _zp4si, _zp6si, _zp8si, _zp10si, _zp12si

; https://fadden.com/apple2/dl/zero-page.txt
; Of course don't use them badly

; For serial
_prev_ram_irq_vector  := $EB
_prev_rom_irq_vector  := $ED
_a_backup             := $EF

_zp2  := $50
_zp2i := $50
_zp2s := $50
_zp2si:= $50
_zp2sp:= $50
_zp2p := $50
_zp2ip:= $50
_zp2sip:= $50
_zp3  := $51
_zp3s := $51

_zp4  := $52
_zp4i := $52
_zp4s := $52
_zp4si:= $52
_zp4sp:= $52
_zp4p := $52
_zp4ip:= $52
_zp4sip:= $52
_zp5  := $53
_zp5s := $53

_zp6  := $54
_zp6i := $54
_zp6s := $54
_zp6si:= $54
_zp6sp:= $54
_zp6p := $54
_zp6ip:= $54
_zp6sip:= $54
_zp7  := $55
_zp7s := $55

_zp8  := $56
_zp8i := $56
_zp8s := $56
_zp8si:= $56
_zp8sp:= $56
_zp8p := $56
_zp8ip:= $56
_zp8sip:= $56
_zp9  := $57
_zp9s := $57

_zp10 := $58
_zp10i:= $58
_zp10s := $58
_zp10si:= $58
_zp10sp:= $58
_zp10p:= $58
_zp10ip:= $58
_zp10sip:= $58
_zp11 := $59
_zp11s:= $59

_zp12 := $60
_zp12i:= $60
_zp12s:= $60
_zp12si:= $60
_zp12sp:= $60
_zp12p:= $60
_zp12ip:= $60
_zp12sip:= $60
_zp13 := $61
_zp13s:= $61
