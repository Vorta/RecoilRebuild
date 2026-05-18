add_library(recoil_compiler_options INTERFACE)

target_compile_definitions(recoil_compiler_options INTERFACE
    _CRT_SECURE_NO_WARNINGS
    WINDOWS_IGNORE_PACKING_MISMATCH
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(recoil_compiler_options INTERFACE
        /W3
        /Zp4
        /fp:precise
    )

    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(recoil_compiler_options INTERFACE /arch:IA32)
    endif()

    if(RECOIL_EMIT_ASM)
        target_compile_options(recoil_compiler_options INTERFACE /FAcs)
    endif()
else()
    target_compile_options(recoil_compiler_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
