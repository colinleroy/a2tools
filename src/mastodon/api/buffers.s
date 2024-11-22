        .export _gen_buf, _selector, _endpoint_buf, _lines

BUF_SIZE          = 512
SELECTOR_SIZE     = 256
ENDPOINT_BUF_SIZE = 128
MAX_LINES_NUM     = 32

_gen_buf           = $800
_selector          = (_gen_buf + BUF_SIZE)
_endpoint_buf      = (_selector + SELECTOR_SIZE)
_lines             = (_endpoint_buf + ENDPOINT_BUF_SIZE)
