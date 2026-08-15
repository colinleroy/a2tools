        .export _cam_features
        .export _cam_wakeup
        .export _cam_set_speed
        .export _cam_set_camera_name
        .export _cam_set_camera_time
        .export _cam_get_information
        .export _cam_set_quality
        .export _cam_set_flash
        .export _cam_take_picture
        .export _cam_get_picture
        .export _cam_get_thumbnail
        .export _cam_delete_pictures
        .export _cam_get_filename
        .export _cam_thumb_histogram
        .export _cam_thumb_load_data

CAM_FEATURES        = 0
CAM_WAKEUP          = 1
CAM_SET_SPEED       = 2
CAM_SET_CAMERA_NAME = 3
CAM_SET_CAMERA_TIME = 4
CAM_GET_INFORMATION = 5
CAM_SET_QUALITY     = 6
CAM_SET_FLASH       = 7
CAM_TAKE_PICTURE    = 8
CAM_GET_PICTURE     = 9
CAM_GET_THUMBNAIL   = 10
CAM_DELETE_PICTURES = 11
CAM_GET_FILENAME    = 12
CAM_THUMB_HISTOGRAM = 13
CAM_THUMB_LOAD_DATA = 14

_cam_features = $0C00

_cam_wakeup:
        jmp     ($0C00 + CAM_WAKEUP*2)

_cam_set_speed:
        jmp     ($0C00 + CAM_SET_SPEED*2)

_cam_set_camera_name:
        jmp     ($0C00 + CAM_SET_CAMERA_NAME*2)

_cam_set_camera_time:
        jmp     ($0C00 + CAM_SET_CAMERA_TIME*2)

_cam_get_information:
        jmp     ($0C00 + CAM_GET_INFORMATION*2)

_cam_set_quality:
        jmp     ($0C00 + CAM_SET_QUALITY*2)

_cam_set_flash:
        jmp     ($0C00 + CAM_SET_FLASH*2)

_cam_take_picture:
        jmp     ($0C00 + CAM_TAKE_PICTURE*2)

_cam_get_picture:
        jmp     ($0C00 + CAM_GET_PICTURE*2)

_cam_get_thumbnail:
        jmp     ($0C00 + CAM_GET_THUMBNAIL*2)

_cam_delete_pictures:
        jmp     ($0C00 + CAM_DELETE_PICTURES*2)

_cam_get_filename:
        jmp     ($0C00 + CAM_GET_FILENAME*2)

_cam_thumb_histogram:
        jmp     ($0C00 + CAM_THUMB_HISTOGRAM*2)

_cam_thumb_load_data:
        jmp     ($0C00 + CAM_THUMB_LOAD_DATA*2)
