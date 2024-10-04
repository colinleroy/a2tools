.struct SURL_RESPONSE
  SIZE              .dword
  CODE              .word
  HEADER_SIZE       .word
  CONTENT_TYPE_SIZE .word
  CUR_HDR_POS       .word
  CUR_POS           .dword
  CONTENT_TYPE      .addr
.endstruct
