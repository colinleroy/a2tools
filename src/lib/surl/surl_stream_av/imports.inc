        .export         _surl_stream_av
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp9, _zp10, _zp12, _zp13, tmp1, tmp2, tmp3, ptr1, ptr2, ptr3, ptr4

        .import         _simple_serial_get_data_reg
        .import         _simple_serial_get_status_reg
        .import         _simple_serial_get_cmd_reg
        .import         _simple_serial_get_ctrl_reg

        .import         _serial_putc_direct
        .import         _serial_read_byte_no_irq
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         _sleep, _init_text, _clrscr
        .import         _printer_slot, _data_slot

        .import         acia_status_reg_r, acia_data_reg_r

        .include        "apple2.inc"
