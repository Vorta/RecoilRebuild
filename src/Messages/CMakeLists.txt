if(NOT MSVC)
    message(FATAL_ERROR "RECOIL_BUILD_MESSAGES_DLL requires an MSVC x86 environment.")
endif()

add_library(recoil_messages SHARED
    messages.c
    messages.def
)

target_include_directories(recoil_messages PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(recoil_messages PRIVATE
    WIN32
    _WINDOWS
    NDEBUG
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_compile_options(recoil_messages PRIVATE
    /TC
    /W3
    /O2
    /Ob0
    /Zp4
)

target_link_options(recoil_messages PRIVATE
    /DEF:${CMAKE_CURRENT_SOURCE_DIR}/messages.def
    /BASE:0x10000000
    /INCREMENTAL:NO
)

set_target_properties(recoil_messages PROPERTIES
    OUTPUT_NAME messages
)
