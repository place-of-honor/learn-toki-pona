#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_root=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
if [ -z "$sdk_root" ]; then
    echo "ANDROID_HOME or ANDROID_SDK_ROOT is required" >&2
    exit 2
fi

ndk_root=${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-$sdk_root/ndk/27.2.12479018}}
toolchain="$ndk_root/toolchains/llvm/prebuilt/linux-x86_64"
glue_dir="$ndk_root/sources/android/native_app_glue"
build_tools="$sdk_root/build-tools/36.0.0"
platform_jar="$sdk_root/platforms/android-36/android.jar"

for required in \
    "$toolchain/bin/aarch64-linux-android23-clang" \
    "$toolchain/bin/armv7a-linux-androideabi23-clang" \
    "$toolchain/bin/x86_64-linux-android23-clang" \
    "$glue_dir/android_native_app_glue.c" \
    "$build_tools/aapt2" \
    "$build_tools/zipalign" \
    "$build_tools/apksigner" \
    "$platform_jar"
do
    if [ ! -e "$required" ]; then
        echo "missing Android build dependency: $required" >&2
        exit 2
    fi
done

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/toki-pona-native-build.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

staging="$work_dir/staging"
output_dir="$project_root/build"
mkdir -p "$staging/assets" "$output_dir"

cp "$project_root/quizzes.txt" "$staging/assets/quizzes.txt"

if [ -d "$project_root/app/src/main/assets/fonts" ]; then
    mkdir -p "$staging/assets/fonts"
    cp "$project_root/app/src/main/assets/fonts/"* "$staging/assets/fonts/"
fi

compile_abi() {
    abi=$1
    compiler_name=$2
    compiler="$toolchain/bin/$compiler_name"
    objects="$work_dir/objects/$abi"
    library_dir="$staging/lib/$abi"
    mkdir -p "$objects" "$library_dir"

    flags="-std=c17 -O2 -g -fPIC -ffunction-sections -fdata-sections"
    warnings="-Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow"
    includes="-I$project_root/app/src/main/c -isystem $glue_dir"

    "$compiler" $flags $warnings -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
        $includes \
        -c "$project_root/app/src/main/c/native_main.c" \
        -o "$objects/native_main.o"

    "$compiler" $flags $warnings -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
        $includes \
        -c "$project_root/app/src/main/c/quiz_model.c" \
        -o "$objects/quiz_model.o"

    "$compiler" $flags -isystem "$glue_dir" \
        -c "$glue_dir/android_native_app_glue.c" \
        -o "$objects/native_app_glue.o"

    "$compiler" -shared \
        -Wl,--no-undefined \
        -Wl,--gc-sections \
        -Wl,-z,relro,-z,now \
        -Wl,-u,ANativeActivity_onCreate \
        "$objects/native_main.o" \
        "$objects/quiz_model.o" \
        "$objects/native_app_glue.o" \
        -landroid -llog -lm \
        -o "$library_dir/libtokipona_drills.so"
}

compile_abi arm64-v8a aarch64-linux-android23-clang
compile_abi armeabi-v7a armv7a-linux-androideabi23-clang
compile_abi x86_64 x86_64-linux-android23-clang

base_apk="$work_dir/base.apk"
unsigned_apk="$work_dir/unsigned.apk"
aligned_apk="$work_dir/aligned.apk"
final_apk="$output_dir/Toki-Pona-Drills-native-v1.1.0.apk"

"$build_tools/aapt2" link \
    -I "$platform_jar" \
    --manifest "$project_root/app/src/main/AndroidManifest.xml" \
    --min-sdk-version 23 \
    --target-sdk-version 36 \
    --version-code 4 \
    --version-name 1.1.0 \
    -o "$base_apk"

cp "$base_apk" "$unsigned_apk"
(
    cd "$staging"
    zip -0 -q "$unsigned_apk" \
        assets/quizzes.txt \
        lib/arm64-v8a/libtokipona_drills.so \
        lib/armeabi-v7a/libtokipona_drills.so \
        lib/x86_64/libtokipona_drills.so
    if [ -d assets/fonts ]; then
        zip -0 -q "$unsigned_apk" assets/fonts/*
    fi
)

"$build_tools/zipalign" -f -P 16 4 "$unsigned_apk" "$aligned_apk"
"$build_tools/zipalign" -c -P 16 4 "$aligned_apk"

signing_key=${TOKI_PONA_SIGNING_KEY_PATH:-$output_dir/Toki-Pona-Drills-update-key.p12}
signing_password=${TOKI_PONA_SIGNING_KEY_PASSWORD:-toki-pona-v1-offline}
signing_alias=${TOKI_PONA_SIGNING_KEY_ALIAS:-toki-pona-drills}

if [ ! -f "$signing_key" ]; then
    keytool -genkeypair \
        -keystore "$signing_key" \
        -storetype PKCS12 \
        -storepass "$signing_password" \
        -keypass "$signing_password" \
        -alias "$signing_alias" \
        -keyalg RSA \
        -keysize 2048 \
        -validity 36500 \
        -dname "CN=Toki Pona Drills, OU=Native Trainer, O=Private Build, C=US" \
        -noprompt
fi

"$build_tools/apksigner" sign \
    --ks "$signing_key" \
    --ks-key-alias "$signing_alias" \
    --ks-pass "pass:$signing_password" \
    --key-pass "pass:$signing_password" \
    --out "$final_apk" \
    "$aligned_apk"

"$build_tools/apksigner" verify \
    --verbose \
    --print-certs \
    "$final_apk"

echo "$final_apk"
