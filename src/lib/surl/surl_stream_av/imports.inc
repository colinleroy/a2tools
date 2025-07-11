        .export         _surl_start_stream_av
        .export         _surl_stream_av
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp9, _zp10, tmp1, tmp2, tmp3, ptr1, ptr2, ptr3, ptr4

        .import         _ser_data_reg
        .import         _ser_status_reg
        .import         _serial_open, _serial_close

        .import         _enable_subtitles
        .import         _video_size
        .import         _translit_charset
        .import         _scrh
        .import         _hgr_mixon, _hgr_mixoff
        .import         _hgr_mix_is_on
        .import         _hgr_force_mono40, _text_mono40, hgr_mono_file

        .import         _serial_putc_direct
        .import         _simple_serial_getc
        .import         _simple_serial_puts_nl
        .import         _serial_read_byte_no_irq
        .import         _simple_serial_flush
        .import         _simple_serial_set_irq

        .import         _surl_start_request
        .import         _surl_read_with_barrier

        .import         _ser_params

        .import         acia_status_reg_r, acia_data_reg_r

        .import         _init_text, _init_hgr, _set_scrollwindow
        .import         _gotoxy, _cprintf, _try_videomode, _clrscr
        .import         _bzero, _memset, _sleep, _cputs
        .import         pusha, pusha0, push0, pushax, shlax3, popptr1

        .import         ostype, _has_80cols
        .importzp       ptr1, tmp3

        .include        "apple2.inc"
        .include        "../../simple_serial.inc"
        .include        "../../../surl-server/surl_protocol.inc"
        .include        "../../surl.inc"
        .include        "stdio.inc"
        .include        "ser-kernel.inc"
