cmake_minimum_required(VERSION 3.22.1)

project(tokipona_drills C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(native_app_glue SYSTEM PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(tokipona_drills SHARED
    native_main.c
    quiz_model.c
)

target_include_directories(tokipona_drills SYSTEM PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_compile_options(tokipona_drills PRIVATE
    -O2
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wconversion
    -Wshadow
    -fstack-protector-strong
)

target_compile_definitions(tokipona_drills PRIVATE
    _FORTIFY_SOURCE=2
)

target_link_options(tokipona_drills PRIVATE
    -Wl,--no-undefined
    -Wl,--gc-sections
    -Wl,-z,relro,-z,now
    -Wl,-u,ANativeActivity_onCreate
)

target_link_libraries(tokipona_drills
    native_app_glue
    android
    log
    m
)
