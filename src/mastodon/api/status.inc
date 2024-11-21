.struct STATUS
  DISPLAYED_AT                  .byte
  ID                            .addr
  REBLOG_ID                     .addr
  CREATED_AT                    .addr
  SPOILER_TEXT                  .addr
  CONTENT                       .addr
  ACCOUNT                       .addr
  REBLOGGED_BY                  .addr
  N_REPLIES                     .byte
  N_REBLOGS                     .byte
  N_FAVOURITES                  .byte
  N_MEDIAS                      .byte
  MEDIA_TYPE                    .byte
  FLAGS                         .byte
  VISIBILITY                    .byte
  POLL                          .addr
.endstruct
