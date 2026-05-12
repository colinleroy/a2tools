        .export _api_login
        .export _api_get_post
        .export _api_get_creds

        .import __API_START__

LOGIN               = 0
GET_POST            = 1
GET_CREDS           = 2

_api_features = __API_START__

_api_login:
        jmp     (__API_START__ + 2 + LOGIN*2)
_api_get_post:
        jmp     (__API_START__ + 2 + GET_POST*2)
_api_get_creds:
        jmp     (__API_START__ + 2 + GET_CREDS*2)
