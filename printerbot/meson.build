printerbot_lib = static_library(
    'printerbot',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
    ],
    link_with : [
        util_lib,
    ],
    sources : [
        'config.c',
        'config.h',
        'rfid-board.c',
        'rfid-board.h'
    ],
)
