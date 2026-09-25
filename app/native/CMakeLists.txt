cmake_minimum_required(VERSION 3.22.1)
project(TokiPonaNative LANGUAGES C)

set(native_app_glue_directory "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

add_library(
    native_app_glue
    OBJECT
    "${native_app_glue_directory}/android_native_app_glue.c"
)
set_property(TARGET native_app_glue PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(native_app_glue PUBLIC "${native_app_glue_directory}")

add_library(
    toki_pona_native
    SHARED
    canvas.c
    exercise_model.c
    native_main.c
    native_ui.c
    $<TARGET_OBJECTS:native_app_glue>
)
target_include_directories(toki_pona_native PRIVATE "${native_app_glue_directory}")
target_compile_features(toki_pona_native PRIVATE c_std_17)
target_compile_options(
    toki_pona_native
    PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)
target_link_libraries(
    toki_pona_native
    PRIVATE
    android
    log
)
