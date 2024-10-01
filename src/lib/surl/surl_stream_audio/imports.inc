        .export         _surl_stream_audio
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1, tmp2, ptr1, ptr2, ptr3

        .import         _simple_serial_get_data_reg
        .import         _simple_serial_get_status_reg
        .import         _ser_params

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         popa, VTABZ, putchardirect, uppercasemask

.ifdef IIGS
        .import         zilog_status_reg_r, zilog_data_reg_r
        .import         _get_iigs_speed, _set_iigs_speed
        .include        "accelerator.inc"
.else
        .import         acia_status_reg_r, acia_data_reg_r
.endif

        .include        "apple2.inc"
        .include        "../../simple_serial.inc"
