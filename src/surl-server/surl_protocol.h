#ifndef __surl_protocol_h
#define __surl_protocol_h

#define SURL_METHOD_ABORT     0x04
#define SURL_METHOD_RAW       0x05
#define SURL_METHOD_GET       0x06
#define SURL_METHOD_POST      0x07
#define SURL_METHOD_PUT       0x08
#define SURL_METHOD_DELETE    0x09
#define SURL_METHOD_POST_DATA 0x10
#define SURL_METHOD_GETTIME   0x11
#define SURL_METHOD_PING      0x12

#define SURL_IS_METHOD(x) ((x) >= SURL_METHOD_ABORT && (x) <= SURL_METHOD_PING)

#define SURL_ANSWER_WAIT            0x20
#define SURL_ANSWER_SEND_SIZE       0x21
#define SURL_ANSWER_RAW_START       0x22
#define SURL_ANSWER_SEND_NUM_FIELDS 0x23
#define SURL_ANSWER_TIME            0x24
#define SURL_ANSWER_PONG            0x25

#define SURL_IS_ANSWER(x) ((x) >= SURL_ANSWER_WAIT && (x) <= SURL_ANSWER_PONG)

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

#define HGR_LEN 8192

#define SURL_DATA_X_WWW_FORM_URLENCODED_HELP  0
#define SURL_DATA_X_WWW_FORM_URLENCODED_RAW   1
#define SURL_DATA_APPLICATION_JSON_HELP       2

#define SURL_HTMLSTRIP_NONE       0
#define SURL_HTMLSTRIP_FULL       1
#define SURL_HTMLSTRIP_WITH_LINKS 2
#endif
