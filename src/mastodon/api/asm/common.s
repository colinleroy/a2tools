
        .export               _gen_buf
        .export               _selector
        .export               _endpoint_buf
        .export               _lines
        .export               _translit_charset
        .export               _monochrome
        .export               _arobase

        .export               _get_surl_for_endpoint
        .export               _id_copy

        .import               _surl_start_request

        .import               _strncpy, _strcpy, _strcat, _malloc0
        .import               pushax, pusha, pushptr1
        .import               popa, swapstk

        .import               _instance_url, _oauth_token

        .importzp             ptr1, tmp1

        .include              "constants.inc"

; Share buffers with IO buffer - so don't use both at the same time.
_gen_buf           = $800
_selector          = (_gen_buf + BUF_SIZE)
_endpoint_buf      = (_selector + SELECTOR_SIZE)
_lines             = (_endpoint_buf + ENDPOINT_BUF_SIZE)

        .rodata

US_CHARSET:        .asciiz "US-ASCII"
auth_header_start: .asciiz "Authorization: Bearer "

        .data

_translit_charset: .byte <US_CHARSET, >US_CHARSET
_monochrome:       .byte 1
_arobase:          .byte '@'

        .bss

; Not initialized, but counting on zerobss to set those to zero.
surl_hdrs:        .res 2
n_headers:        .res 1

        .segment "LC"

;void id_copy(char *dst, char *src)
.proc _id_copy
        jsr       pushax
        lda       #<SNOWFLAKE_ID_LEN
        ldx       #>SNOWFLAKE_ID_LEN
        jmp       _strncpy
.endproc

;const surl_response *get_surl_for_endpoint(char method, char *endpoint)
.proc _get_surl_for_endpoint
        jsr       pushax      ; Save endpoint to TOS

        ldx       surl_hdrs+1           ; Check if headers are allocated
        bne       :+
        ldx       _oauth_token+1        ; Check if OAuth token exists
        beq       :+

        lda       #<ENDPOINT_BUF_SIZE
        ldx       #>ENDPOINT_BUF_SIZE
        jsr       _malloc0              ; Allocate

        jsr       pushax
        lda       #<auth_header_start
        ldx       #>auth_header_start
        jsr       _strcpy               ; Set start of header

        jsr       pushax
        lda       _oauth_token
        ldx       _oauth_token+1
        jsr       _strcat               ; Add oauth token

        sta       surl_hdrs             ; Save pointer to header
        stx       surl_hdrs+1
        lda       #1
        sta       n_headers

:       lda       #<_gen_buf            ; Now build URL
        ldx       #>_gen_buf
        jsr       pushax
        lda       _instance_url
        ldx       _instance_url+1
        jsr       _strcpy

        ; We have _gen_buf in AX, endpoint in TOS
        jsr       swapstk
        ; Now we have _gen_buf in TOS, endpoint in AX
        jsr       _strcat

        ; Now we have _gen_buf in AX and TOS only contains method
        sta       ptr1
        stx       ptr1+1
        jsr       popa        ; Time to get method from TOS
        sta       tmp1
        lda       #<surl_hdrs ; Use address instead of value
        ldx       #>surl_hdrs ; because an array is expected
        jsr       pushax
        lda       n_headers
        jsr       pusha
        jsr       pushptr1
        lda       tmp1
        jmp       _surl_start_request
.endproc
