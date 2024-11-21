                .export         _media_type_str
                .export         _status_free

                .import         _status_new
                .export         _status_fill_from_json

                .import         _free, _account_free, _poll_free
                .import         popptr1, pushptr1, popax, pushax, pusha, swapstk
                .import         incaxy
                .import         return0, returnFFFF
                .import         _snprintf, _strdup

                .import         _get_surl_for_endpoint
                .import         _surl_response_ok
                .import         _surl_get_json
                .import         _endpoint_buf
                .import         _gen_buf
                .import         _lines
                .import         _translit_charset
                .import         _basic_selector

                .import         _strnsplit_in_place

                .importzp       _zp6, sp

                .include        "../../surl-server/surl_protocol.inc"
                .include        "status.inc"

                .rodata

.ifdef __APPLE2ENH__
media_str_media:  .asciiz "media"
media_str_image:  .asciiz "image"
media_str_video:  .asciiz "video"
media_str_audio:  .asciiz "audio"
media_str_gif:    .asciiz "gif"
.else
media_str_media:  .asciiz "img"
media_str_image:  .asciiz "img"
media_str_video:  .asciiz "vid"
media_str_audio:  .asciiz "snd"
media_str_gif:    .asciiz "gif"
.endif

_media_type_str:  .addr media_str_media
                  .addr media_str_image
                  .addr media_str_video
                  .addr media_str_audio
                  .addr media_str_gif

_STATUS_ENDPOINT: .asciiz "/api/v1/statuses/%s"

                .segment "LC"

load_zp6_field_addr:
        iny
        lda       (_zp6),y
        tax
        dey
        lda       (_zp6),y
        rts

load_zp8_field_addr:
        iny
        lda       (_zp8),y
        tax
        dey
        lda       (_zp8),y
        rts

store_zp6_field_addr:
        sta       (_zp6),y
        txa
        iny
        sta       (_zp6),y
        rts

status_free_field:
        jsr       load_zp6_field_addr
        jmp       _free

; void status_free(status *s)
.proc _status_free
        sta       _zp6
        stx       _zp6+1

        ora       _zp6+1      ; s == NULL?
        bne       :+
        rts

:       ldy       #STATUS::ID
        jsr       status_free_field
        ldy       #STATUS::REBLOG_ID
        jsr       status_free_field
        ldy       #STATUS::CREATED_AT
        jsr       status_free_field
        ldy       #STATUS::SPOILER_TEXT
        jsr       status_free_field
        ldy       #STATUS::CONTENT
        jsr       status_free_field
        ldy       #STATUS::REBLOGGED_BY
        jsr       status_free_field

        ldy       #STATUS::ACCOUNT
        jsr       load_zp6_field_addr
        jsr       _account_free

        ldy       #STATUS::POLL
        jsr       load_zp6_field_addr
        jsr       _poll_free

        lda       _zp6
        ldx       _zp6+1
        jmp       _free
.endproc

        .segment "LOWCODE"

; static __fastcall__ char status_fill_from_json(status *s, char *id, char full)
.proc _status_fill_from_json
        sta       full
        jsr       popax                 ; pop status_id

        jsr       _strdup               ; strdup it
        cpx       #$00
        bne       :+

        ; strdup failed
        jsr       popax                 ; pop s
        jmp       returnFFFF

:       jsr       swapstk               ; pop s, push result of strdup
        sta       _zp6
        stx       _zp6+1

        jsr       popax                 ; get result of strdup
        ldy       #STATUS::ID
        jsr       store_zp6_field_addr      ; store it

        lda       #0
        sta       is_reblog
        lda       #8                    ; strlen(".reblog|")
        sta       reblog_offset

@get_json:
        lda       #<_gen_buf
        ldx       #>_gen_buf
        jsr       pushax
        lda       #<_basic_selector
        ldx       #>_basic_selector
        ldy       reblog_offset
        jsr       incaxy
        jsr       pushax
        lda       #<_translit_charset
        ldx       #>_translit_charset
        jsr       pushax
        lda       #SURL_HTMLSTRIP_NONE
        jsr       pusha
        lda       #<BUF_SIZE
        ldx       #>BUF_SIZE
        jsr       _surl_get_json

        cpx       #$FF
        bne       :+
@err_load:
        jmp       returnFFFF

:       lda       #<_gen_buf
        ldx       #>_gen_buf
        jsr       pushax
        lda       #$0A
        jsr       pusha
        lda       #<_lines
        ldx       #>_lines
        sta       _zp8
        stx       _zp8+1
        jsr       pushax
        lda       #17
        ldx       #0
        jsr       _strnsplit_in_place
        cmp       #16
        bcc       @err_load

        lda       is_reblog
        bne       :+
        ldy       #10                   ; lines[5]
        lda       (_zp8),y
        cmp       #'-'
        beq       :+

        ; This is a reblog
        ldy       #2                    ; lines[1]
        jsr       load_zp8_field_addr   ; s->reblogged_by = strdup(lines[1]);
        jsr       _strdup
        ldy       #STATUS::REBLOGGED_BY
        jsr       store_zp6_field_addr

        ldy       #STATUS::ID           ; s->reblog_id = s->id;
        jsr       load_zp6_field_addr
        ldy       #STATUS::REBLOG_ID
        jsr       store_zp6_field_addr

        ldy       #10                   ; s->id = strdup(lines[5]);
        jsr       load_zp8_field_addr
        jsr       _strdup
        ldy       #STATUS::ID
        jsr       store_zp6_field_addr

        ldx       #1
        stx       is_reblog
        dex
        stx       reblog_offset
        beq       @get_json             ; Load status as reblog

        ldy       #8                    ; s->created_at = date_format(lines[4], 1);
        jsr       load_zp8_field_addr
        jsr       pushax
        lda       #1
        jsr       date_format
        ldy       #STATUS::CREATED_AT
        jsr       store_zp6_field_addr

        ldy       #12                   ; lines[6][0]?
        jsr       load_zp8_field_addr
        lda       (_zp8),y
        beq       :+

        ; Spoiler text
        lda       #<TL_SPOILER_TEXT_BUF ; Alloc
        ldx       #$00
        jsr       _malloc0
        ldy       #STATUS::SPOILER_TEXT
        jsr       store_zp6_field_addr
        
        jsr       pushax                ; Strncpy
        lda       _zp8
        ldx       _zp8+1
        jsr       pushax

        lda       #<TL_SPOILER_TEXT_BUF
        ldx       #$00
        jsr       _strncpy

        sta       ptr1                  ; Terminate
        stx       ptr1+1
        ldy       #TL_SPOILER_TEXT_BUF-1
        lda       #$00
        sta       (ptr1),y

:       ldy       #14                   ; lines[7]
        jsr       load_zp8_field_addr
        jsr       _atoc
        ldy       #STATUS::N_MEDIAS
        sta       (_zp6),y

        ldy       #16                   ; lines[8]
        jsr       load_zp8_field_addr
        ldy       #$00
        lda       (_zp8),y
        cmp       'g'
        bne       :+
        lda       #MEDIA_TYPE_GIFV
        bne       @store_media_type
:       cmp       'v'
        bne       :+
        lda       #MEDIA_TYPE_VIDEO
        bne       @store_media_type
:       cmp       'a'
        bne       :+
        lda       #MEDIA_TYPE_AUDIO
        bne       @store_media_type
:       cmp       'i'
        bne       :+
        lda       #MEDIA_TYPE_IMAGE
@store_media_type:
        ldy       #STATUS::MEDIA_TYPE
        sta       (_zp6),y

:       ldy       #18                   ; lines[9]
        jsr       load_zp8_field_addr
        jsr       _atoc
        ldy       #STATUS::N_REPLIES
        sta       (_zp6),y

        ldy       #20                   ; lines[10]
        jsr       load_zp8_field_addr
        jsr       _atoc
        ldy       #STATUS::N_REBLOGS
        sta       (_zp6),y

        ldy       #22                   ; lines[11]
        jsr       load_zp8_field_addr
        jsr       _atoc
        ldy       #STATUS::N_FAVOURITES
        sta       (_zp6),y

        jsr       _account_new_from_lines
        ldy       #STATUS::ACCOUNT
        jsr       store_zp6_field_addr

        ldy       #24                   ; lines[12][1]
        jsr       load_zp8_field_addr
        ldy       #$01
        lda       (_zp8),y
        cmp       'u'
        bne       :+
        lda       #COMPOSE_PUBLIC
        bne       @store_audience
:       cmp       'n'
        bne       :+
        lda       #COMPOSE_UNLISTED
        bne       @store_audience
:       cmp       'r'
        bne       :+
        lda       #COMPOSE_PRIVATE
        bne       @store_audience
:       lda       #COMPOSE_MENTION
@store_audience:
        ldy       #STATUS::VISIBILITY
        sta       (_zp6),y

.endproc

        .bss
full:          .res 1
is_reblog:     .res 1
reblog_offset: .res 1
