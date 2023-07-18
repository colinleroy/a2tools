#ifndef __surl_protocol_h
#define __surl_protocol_h

#define SURL_METHOD_ABORT  0x04
#define SURL_METHOD_RAW    0x05
#define SURL_METHOD_GET    0x06
#define SURL_METHOD_POST   0x07
#define SURL_METHOD_PUT    0x08
#define SURL_METHOD_DELETE 0x09

#define SURL_IS_METHOD(x) ((x) >= SURL_METHOD_ABORT && (x) <= SURL_METHOD_DELETE)

#define SURL_ANSWER_WAIT       0x20
#define SURL_ANSWER_SEND_SIZE  0x21
#define SURL_ANSWER_RAW_START  0x22

#define SURL_IS_ANSWER(x) ((x) >= SURL_ANSWER_WAIT && (x) <= SURL_ANSWER_RAW_START)

#define SURL_CMD_SEND     0x30
#define SURL_CMD_HEADERS  0x31
#define SURL_CMD_FIND     0x32
#define SURL_CMD_JSON     0x33
#define SURL_CMD_HGR      0x34

#define SURL_IS_CMD(x) ((x) >= SURL_CMD_SEND && (x) <= SURL_CMD_HGR)

#define SURL_ERROR_OK           0x40
#define SURL_ERROR_NOT_FOUND    0x41
#define SURL_ERROR_NOT_JSON     0x42
#define SURL_ERROR_CONV_FAILED  0x43

#define SURL_IS_ERROR(x) ((x) >= SURL_ERROR_OK && (x) <= SURL_ERROR_CONV_FAILED)

#endif
