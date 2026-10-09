cmake_minimum_required(VERSION 3.22.1)

project(tokipona_drills C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(TOKI_PONA_AICI_ROOT "$ENV{TOKI_PONA_AICI_ROOT}" CACHE PATH "Pinned shared ICK producer checkout")
include("${TOKI_PONA_AICI_ROOT}/ick-android/OwnedC.cmake")
if(NOT ICK_COMPILER)
    set(TOKI_PONA_ICK_STAGE "$ENV{TOKI_PONA_ICK_STAGE_ROOT}/${ANDROID_ABI}" CACHE PATH "Qualified current ICK ABI stage")
    ick_android_stage(ROOT "${TOKI_PONA_ICK_STAGE}" FORTIFY_SOURCE 2)
endif()
set(platform_options -O2 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow -fstack-protector-strong)
ick_android_objects(platform_objects NAME toki_platform STANDARD 17
    SOURCES native_main.c quiz_model.c
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}" "${ANDROID_NDK}/sources/android/native_app_glue"
    OPTIONS ${platform_options}
    DEFINITIONS TOKI_SESSION_OBJECTS_LINKED=1 _FORTIFY_SOURCE=2)

# This bounded source/assembly target never claims a complete application.
# The normal library route below still requires every source-bound Lua object.
option(TOKI_PONA_PLATFORM_ONLY "Qualify owned platform C objects without an application link" OFF)
if(TOKI_PONA_PLATFORM_ONLY)
    list(GET platform_objects 0 native_object)
    list(GET platform_objects 1 model_object)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native_main.o" "${CMAKE_CURRENT_BINARY_DIR}/quiz_model.o"
        COMMAND "${CMAKE_COMMAND}" -E copy "${native_object}" "${CMAKE_CURRENT_BINARY_DIR}/native_main.o"
        COMMAND "${CMAKE_COMMAND}" -E copy "${model_object}" "${CMAKE_CURRENT_BINARY_DIR}/quiz_model.o"
        DEPENDS ${platform_objects} VERBATIM)
    add_custom_target(toki-platform ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native_main.o" "${CMAKE_CURRENT_BINARY_DIR}/quiz_model.o")
    return()
endif()

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(native_app_glue SYSTEM PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(tokipona_drills SHARED
    ${platform_objects}
)
set_target_properties(tokipona_drills PROPERTIES LINKER_LANGUAGE C)

# The modified-language objects must be produced by qualified ICK, never Clang.
# Preserve all requested ABIs; a missing closure is a build blocker.
set(session_object_root "$ENV{TOKI_PONA_SESSION_OBJECT_ROOT}")
set(session_object_dir "${session_object_root}/${ANDROID_ABI}")
set(session_receipt "${session_object_dir}/source.receipt.tsv")
foreach(required quiz_session.o icky-lua.o source.receipt.tsv)
    if(NOT EXISTS "${session_object_dir}/${required}")
        message(FATAL_ERROR
            "Missing qualified ICK session object closure for ${ANDROID_ABI}: ${required}. "
            "Set TOKI_PONA_SESSION_OBJECT_ROOT to source-bound objects; no Clang or C fallback.")
    endif()
endforeach()
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/quiz_session.c" bridge_sha)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/quiz_session.h" session_header_sha)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/quiz_model.h" model_header_sha)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/../../../../policy/quiz-session.lua" policy_sha)
file(SHA256 "${session_object_dir}/quiz_session.o" bridge_object_sha)
file(SHA256 "${session_object_dir}/icky-lua.o" runtime_object_sha)
file(STRINGS "${session_receipt}" session_receipt_lines)
foreach(required
    "abi\t${ANDROID_ABI}"
    "compiler_stage\tick"
    "ick_revision\t515c0f29fe6e2e96e10495fbaf25da93532e7722"
    "lua_revision\t87306483cec50f8c750a22dda1d0742246fad756"
    "bridge_source_sha256\t${bridge_sha}"
    "session_header_sha256\t${session_header_sha}"
    "model_header_sha256\t${model_header_sha}"
    "policy_source_sha256\t${policy_sha}"
    "bridge_object_sha256\t${bridge_object_sha}"
    "runtime_object_sha256\t${runtime_object_sha}")
    list(FIND session_receipt_lines "${required}" receipt_index)
    if(receipt_index EQUAL -1)
        message(FATAL_ERROR "Stale or substituted ICK session object receipt for ${ANDROID_ABI}: ${required}")
    endif()
endforeach()
set_source_files_properties(
    "${session_object_dir}/quiz_session.o" "${session_object_dir}/icky-lua.o"
    PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
target_sources(tokipona_drills PRIVATE
    "${session_object_dir}/quiz_session.o" "${session_object_dir}/icky-lua.o")
target_compile_definitions(tokipona_drills PRIVATE TOKI_SESSION_OBJECTS_LINKED=1)

target_include_directories(tokipona_drills SYSTEM PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_compile_options(tokipona_drills PRIVATE ${platform_options})

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
    dl
)
