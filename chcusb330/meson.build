chcusb330 = shared_library(
    'chcusb330',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    vs_module_defs : 'exports.def',
    link_with : [
        util_lib,
        printerbot_lib,
        chcusb,
    ],
    dependencies : [
    ],
    sources : [
        'dllmain.c',
        'shim.c',
        'shim.h',
    ],
)
