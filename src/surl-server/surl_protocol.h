#ifndef __surl_protocol_h
#define __surl_protocol_h

/* Update in .inc too! */
#define SURL_PROTOCOL_VERSION 24
#define VERSION "24.0.8"

#define SURL_CLIENT_READY           0x2F
#define HGR_LEN                     8192U

typedef enum {
  SURL_METHOD_RAW             = 0x05,
  SURL_METHOD_GET             = 0x06,
  SURL_METHOD_POST            = 0x07,
  SURL_METHOD_PUT             = 0x08,
  SURL_METHOD_DELETE          = 0x09,
  SURL_METHOD_POST_DATA       = 0x10,
  SURL_METHOD_GETTIME         = 0x11,
  SURL_METHOD_PING            = 0x12,
  SURL_METHOD_DEBUG           = 0x13,
  SURL_METHOD_STREAM_VIDEO    = 0x14,
  SURL_METHOD_STREAM_AUDIO    = 0x15,
  SURL_METHOD_STREAM_AV       = 0x16,
  SURL_METHOD_DUMP            = 0x17,
  SURL_METHOD_MKDIR           = 0x18,
  SURL_METHOD_VSDRIVE         = 0xC5,
  SURL_METHOD_ABORT           = ('d'|0x80) // $E4
} SurlMethod;

#define SURL_IS_METHOD(x) ((x) == SURL_METHOD_ABORT || (x) == SURL_METHOD_VSDRIVE || ((x) >= SURL_METHOD_RAW && (x) <= SURL_METHOD_MKDIR))

typedef enum {
  SURL_ANSWER_WAIT            = 0x20,
  SURL_ANSWER_SEND_SIZE       = 0x21,
  SURL_ANSWER_RAW_START       = 0x22,
  SURL_ANSWER_SEND_NUM_FIELDS = 0x23,
  SURL_ANSWER_TIME            = 0x24,
  SURL_ANSWER_PONG            = 0x25,
  SURL_ANSWER_STREAM_ERROR    = 0x26,
  SURL_ANSWER_STREAM_START    = 0x27,
  SURL_ANSWER_STREAM_LOAD     = 0x28,
  SURL_ANSWER_STREAM_READY    = 0x29,
  SURL_ANSWER_STREAM_ART      = 0x2A,
  SURL_ANSWER_STREAM_METADATA = 0x2B
} SurlAnswer;

#define SURL_IS_ANSWER(x) ((x) >= SURL_ANSWER_WAIT && (x) <= SURL_ANSWER_STREAM_METADATA)


typedef enum {
  SURL_CMD_SEND        = 0x30,
  SURL_CMD_HEADERS     = 0x31,
  SURL_CMD_FIND        = 0x32,
  SURL_CMD_JSON        = 0x33,
  SURL_CMD_HGR         = 0x34,
  SURL_CMD_STRIPHTML   = 0x35,
  SURL_CMD_TRANSLIT    = 0x36,
  SURL_CMD_SKIP        = 0x37,
  SURL_CMD_FIND_HEADER = 0x38,
  SURL_CMD_DHGR        = 0x39,
} SurlCommand;

#define SURL_IS_CMD(x) ((x) >= SURL_CMD_SEND && (x) <= SURL_CMD_DHGR)

typedef enum {
  SURL_ERROR_OK           = 0x40,
  SURL_ERROR_NOT_FOUND    = 0x41,
  SURL_ERROR_NOT_JSON     = 0x42,
  SURL_ERROR_CONV_FAILED  = 0x43,
  SURL_ERROR_INV_REGEXP   = 0x44
} SurlError;

#define SURL_IS_ERROR(x) ((x) >= SURL_ERROR_OK && (x) <= SURL_ERROR_INV_REGEXP)

typedef enum {
  SURL_UPLOAD_GO          = 0x50,
  SURL_UPLOAD_PARAM_ERROR = 0x51
} SurlUploadResponse;

typedef enum {
  SURL_DATA_X_WWW_FORM_URLENCODED_HELP  = 0,
  SURL_DATA_X_WWW_FORM_URLENCODED_RAW   = 1,
  SURL_DATA_APPLICATION_JSON_HELP       = 2
} SurlDataFormat;

typedef enum {
  SURL_HTMLSTRIP_NONE       = 0,
  SURL_HTMLSTRIP_FULL       = 1,
  SURL_HTMLSTRIP_WITH_LINKS = 2
} SurlHtmlStripLevel;

typedef enum {
  HGR_SCALE_FULL     = 0,
  HGR_SCALE_HALF     = 1,
  HGR_SCALE_MIXHGR   = 2
} HGRScale;

typedef enum {
  SUBTITLES_NO       = 0,
  SUBTITLES_AUTO     = 1,
  SUBTITLES_URL      = 2
} SubtitlesMode;

typedef enum {
  SURL_VIDEO_PORT_OK  = 0,
  SURL_VIDEO_PORT_NOK = 1
} VideoPortStatus;

typedef enum {
  SURL_MATCH_CASE_SENSITIVE    = 0,
  SURL_MATCH_CASE_INSENSITIVE  = 1,
  SURL_REGEXP_CASE_SENSITIVE   = 2,
  SURL_REGEXP_CASE_INSENSITIVE = 3
} MatchType;

typedef enum {
  SURL_ERR_TIMEOUT             = 504,
  SURL_ERR_PROTOCOL_ERROR      = 508,
  SURL_ERR_PROXY_NOT_CONNECTED = 600
} SurlHTTPCodes;

#endif
