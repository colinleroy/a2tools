#ifndef __surl_protocol_h
#define __surl_protocol_h

#define SURL_PROTOCOL_VERSION 0x0E

#define SURL_METHOD_ABORT           ('d'|0x80) // $E4
#define SURL_METHOD_RAW             0x05
#define SURL_METHOD_GET             0x06
#define SURL_METHOD_POST            0x07
#define SURL_METHOD_PUT             0x08
#define SURL_METHOD_DELETE          0x09
#define SURL_METHOD_POST_DATA       0x10
#define SURL_METHOD_GETTIME         0x11
#define SURL_METHOD_PING            0x12
#define SURL_METHOD_DEBUG           0x13
#define SURL_METHOD_STREAM_VIDEO    0x14
#define SURL_METHOD_STREAM_AUDIO    0x15
#define SURL_METHOD_STREAM_AV       0x16

#define SURL_IS_METHOD(x) ((x) == SURL_METHOD_ABORT || ((x) >= SURL_METHOD_RAW && (x) <= SURL_METHOD_STREAM_AV))

#define SURL_ANSWER_WAIT            0x20
#define SURL_ANSWER_SEND_SIZE       0x21
#define SURL_ANSWER_RAW_START       0x22
#define SURL_ANSWER_SEND_NUM_FIELDS 0x23
#define SURL_ANSWER_TIME            0x24
#define SURL_ANSWER_PONG            0x25
#define SURL_ANSWER_STREAM_ERROR    0x26
#define SURL_ANSWER_STREAM_START    0x27
#define SURL_ANSWER_STREAM_LOAD     0x28
#define SURL_ANSWER_STREAM_READY    0x29
#define SURL_ANSWER_STREAM_ART      0x2A
#define SURL_ANSWER_STREAM_METADATA 0x2B

#define SURL_IS_ANSWER(x) ((x) >= SURL_ANSWER_WAIT && (x) <= SURL_ANSWER_STREAM_METADATA)

#define SURL_CLIENT_READY           0x2F

#define SURL_CMD_SEND      0x30
#define SURL_CMD_HEADERS   0x31
#define SURL_CMD_FIND      0x32
#define SURL_CMD_JSON      0x33
#define SURL_CMD_HGR       0x34
#define SURL_CMD_STRIPHTML 0x35
#define SURL_CMD_TRANSLIT  0x36
#define SURL_CMD_SKIP      0x37

#define SURL_IS_CMD(x) ((x) >= SURL_CMD_SEND && (x) <= SURL_CMD_SKIP)

#define SURL_ERROR_OK           0x40
#define SURL_ERROR_NOT_FOUND    0x41
#define SURL_ERROR_NOT_JSON     0x42
#define SURL_ERROR_CONV_FAILED  0x43

#define SURL_IS_ERROR(x) ((x) >= SURL_ERROR_OK && (x) <= SURL_ERROR_CONV_FAILED)

#define SURL_UPLOAD_GO          0x50
#define SURL_UPLOAD_PARAM_ERROR 0x51

#define HGR_LEN 8192U

#define SURL_DATA_X_WWW_FORM_URLENCODED_HELP  0
#define SURL_DATA_X_WWW_FORM_URLENCODED_RAW   1
#define SURL_DATA_APPLICATION_JSON_HELP       2

#define SURL_HTMLSTRIP_NONE       0
#define SURL_HTMLSTRIP_FULL       1
#define SURL_HTMLSTRIP_WITH_LINKS 2

enum HeightScale {
  HGR_SCALE_FULL,
  HGR_SCALE_HALF,
  HGR_SCALE_MIXHGR
};

#endif
