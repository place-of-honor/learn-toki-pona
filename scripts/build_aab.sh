#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$project_root/build"
bundletool_version=1.18.3
bundletool_sha256=a099cfa1543f55593bc2ed16a70a7c67fe54b1747bb7301f37fdfd6d91028e29
bundletool_jar=${BUNDLETOOL_JAR:-"$build_dir/bundletool-all-$bundletool_version.jar"}

mkdir -p "$build_dir"

if [ ! -f "$bundletool_jar" ]; then
    curl --fail --location --retry 3         --output "$bundletool_jar"         "https://github.com/google/bundletool/releases/download/$bundletool_version/bundletool-all-$bundletool_version.jar"
fi

printf '%s  %s\n' "$bundletool_sha256" "$bundletool_jar" |
    sha256sum --check

(
    cd "$project_root"
    ./gradlew --no-daemon :app:bundleRelease
)

agp_bundle="$project_root/app/build/outputs/bundle/release/app-release.aab"
if [ ! -s "$agp_bundle" ]; then
    echo "Gradle did not produce the staging app bundle" >&2
    exit 2
fi

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/toki-pona-aab.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

unpacked="$work_dir/unpacked"
module="$work_dir/base"
mkdir -p "$unpacked" "$module"

unzip -q "$agp_bundle"     'base/manifest/*'     'base/resources.pb'     'base/assets/*'     'base/lib/*'     -d "$unpacked"

test -s "$unpacked/base/manifest/AndroidManifest.xml"
test -s "$unpacked/base/resources.pb"
test -s "$unpacked/base/assets/quizzes.txt"

cp -R "$unpacked/base/manifest" "$module/"
cp "$unpacked/base/resources.pb" "$module/"
cp -R "$unpacked/base/assets" "$module/"
cp -R "$unpacked/base/lib" "$module/"

module_zip="$work_dir/base.zip"
(
    cd "$module"
    zip -q -r "$module_zip" .
)

version_name=${PLAY_VERSION_NAME:-1.1.0}
final_bundle="$build_dir/Toki-Pona-Drills-native-v$version_name.aab"

rm -f "$final_bundle"
java -jar "$bundletool_jar" build-bundle     --modules="$module_zip"     --output="$final_bundle"

java -jar "$bundletool_jar" validate     --bundle="$final_bundle"

if unzip -l "$final_bundle" | grep -Fq '/dex/'; then
    echo "final native app bundle unexpectedly contains DEX" >&2
    exit 3
fi

if [ -n "${ANDROID_UPLOAD_KEYSTORE_PATH:-}" ]; then
    : "${ANDROID_UPLOAD_KEYSTORE_PASSWORD:?ANDROID_UPLOAD_KEYSTORE_PASSWORD is required}"
    : "${ANDROID_UPLOAD_KEY_ALIAS:?ANDROID_UPLOAD_KEY_ALIAS is required}"
    : "${ANDROID_UPLOAD_KEY_PASSWORD:?ANDROID_UPLOAD_KEY_PASSWORD is required}"

    jarsigner         -keystore "$ANDROID_UPLOAD_KEYSTORE_PATH"         -storetype PKCS12         -storepass "$ANDROID_UPLOAD_KEYSTORE_PASSWORD"         -keypass "$ANDROID_UPLOAD_KEY_PASSWORD"         "$final_bundle"         "$ANDROID_UPLOAD_KEY_ALIAS"

    jarsigner -verify -strict "$final_bundle"
fi

echo "$final_bundle"
