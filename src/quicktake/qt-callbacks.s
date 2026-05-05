        .export _cam_wakeup
        .export _cam_set_speed
        .export _cam_set_camera_name
        .export _cam_set_camera_time
        .export _qt_get_information
        .export _cam_set_quality
        .export _cam_set_flash
        .export _cam_take_picture
        .export _qt_get_picture
        .export _cam_get_thumbnail
        .export _cam_delete_pictures
        .export _cam_features

CAM_WAKEUP          = 0
CAM_SET_SPEED       = 1
CAM_SET_CAMERA_NAME = 2
CAM_SET_CAMERA_TIME = 3
CAM_GET_INFORMATION = 4
CAM_SET_QUALITY     = 5
CAM_SET_FLASH       = 6
CAM_TAKE_PICTURE    = 7
CAM_GET_PICTURE     = 8
CAM_GET_THUMBNAIL   = 9
CAM_DELETE_PICTURES = 10

_cam_features = $0C00

_cam_wakeup:
        jmp     ($0C01 + CAM_WAKEUP*2)

_cam_set_speed:
        jmp     ($0C01 + CAM_SET_SPEED*2)

_cam_set_camera_name:
        jmp     ($0C01 + CAM_SET_CAMERA_NAME*2)

_cam_set_camera_time:
        jmp     ($0C01 + CAM_SET_CAMERA_TIME*2)

_qt_get_information:
        jmp     ($0C01 + CAM_GET_INFORMATION*2)

_cam_set_quality:
        jmp     ($0C01 + CAM_SET_QUALITY*2)

_cam_set_flash:
        jmp     ($0C01 + CAM_SET_FLASH*2)

_cam_take_picture:
        jmp     ($0C01 + CAM_TAKE_PICTURE*2)

_qt_get_picture:
        jmp     ($0C01 + CAM_GET_PICTURE*2)

_cam_get_thumbnail:
        jmp     ($0C01 + CAM_GET_THUMBNAIL*2)

_cam_delete_pictures:
        jmp     ($0C01 + CAM_DELETE_PICTURES*2)
