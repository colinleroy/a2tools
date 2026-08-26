PACKET_TYPE                   = 0
PACKET_SUBTYPE                = 1
PACKET_LENGTH                 = 2
PACKET_OPERATION              = 4
PACKET_REGISTER               = 5
PACKET_VALUE                  = 6
PACKET_ACTION                 = 5
PACKET_SUBACTION              = 6

SIERRA_PACKET_NUL             = $00
SIERRA_PACKET_DATA            = $02
SIERRA_PACKET_DATA_END        = $03
SIERRA_PACKET_ENQ             = $05
SIERRA_PACKET_ACK             = $06
SIERRA_PACKET_INVALID         = $11
SIERRA_PACKET_NAK             = $15
SIERRA_PACKET_CANCEL          = $18
SIERRA_PACKET_COMMAND         = $1b
SIERRA_PACKET_WRONG_SPEED     = $8c
SIERRA_PACKET_SESSION_ERROR   = $fc
SIERRA_PACKET_SESSION_END     = $ff
SIERRA_PACKET_RETRY_INTERNAL  = $ee

SIERRA_PACKET_SIZE            = 32774

SIERRA_SUBPACKET_CMD_FIRST    = $53
SIERRA_SUBPACKET_CMD          = $43

SIERRA_REG_RESOLUTION         = $01
SIERRA_REG_DATE               = $02
SIERRA_REG_PIC_NUM            = $04
SIERRA_REG_FLASH_MODE         = $07
SIERRA_REG_NUM_PICS           = $0A
SIERRA_REG_LEFT_PICS          = $0B
SIERRA_REG_PIC_SIZE           = $0C
SIERRA_REG_THUMB_SIZE         = $0D
SIERRA_REG_PIC_DATA           = $0E
SIERRA_REG_THUMB_DATA         = $0F
SIERRA_REG_SPEED              = $11
SIERRA_REG_NAME               = $16
SIERRA_REG_MODEL              = $1B
SIERRA_REG_BATTERY            = $1C

SIERRA_ACTION_DELETE_ALL      = $01
SIERRA_ACTION_CAPTURE         = $02

OP_SET_INT                    = $00
OP_GET_INT                    = $01
OP_ACTION                     = $02
OP_SET_STRING                 = $03
OP_GET_STRING                 = $04

SIERRA_SPEED_9600             = $01
SIERRA_SPEED_19200            = $02
SIERRA_SPEED_57600            = $04
SIERRA_SPEED_115200           = $05
