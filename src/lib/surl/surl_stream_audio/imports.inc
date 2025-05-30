        .export         _surl_stream_audio
        .export         _SAMPLES_BASE, _AUDIO_CODE_START, _AUDIO_CODE_SIZE

        .importzp       _zp6, _zp8, _zp10, tmp1, tmp2, tmp3, ptr1, ptr2, ptr3

        .import         _ser_data_reg
        .import         _ser_status_reg
        .import         _ser_params

        .import         _serial_putc_direct
        .import         _simple_serial_flush
        .import         popa, VTABZ, putchardirect, uppercasemask

        .import         _get_iigs_speed, _set_iigs_speed, ostype
        .import         _is_iie

        .include        "apple2.inc"
        .include        "accelerator.inc"
        .include        "../../simple_serial.inc"
