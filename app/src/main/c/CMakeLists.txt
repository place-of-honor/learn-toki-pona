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
    dl
)
