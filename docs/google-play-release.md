# Google Play release

Toki Pona Drills keeps its existing package ID, `org.tokipona.drills`.

The Gradle path now builds the NDK `NativeActivity` replacement candidate. Its
default AAB contains both `armeabi-v7a` and `arm64-v8a`; CI checks that both
native libraries are present while application DEX and the retired browser
payload are absent.

The Apktool path remains historical release material. A successful native AAB
build proves packaging only; it does not prove installation, launch, touch
behavior, typography, persistence, or full course parity on a physical device.

## Signing continuity

The installed v1.0.1 APK has an existing signing identity. If Play installs
should update that sideloaded copy without an uninstall, retain its private key
and import it as the Play **app-signing key** during enrollment. Merely using it
as the CI upload key while asking Google to generate a different app-signing key
does not preserve update compatibility.

If that private key was not retained, Play can use a new identity, but devices
with the sideloaded copy must uninstall it once before installing the Play build.

## GitHub environment and secrets

Create a protected environment named `google-play` with:

- `ANDROID_UPLOAD_KEYSTORE_BASE64`
- `ANDROID_UPLOAD_KEYSTORE_PASSWORD`
- `ANDROID_UPLOAD_KEY_ALIAS`
- `ANDROID_UPLOAD_KEY_PASSWORD`
- `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON`

The first four values describe the private upload key used by CI.

## First and later uploads

Keep replacement builds on artifact-only or internal testing until physical-device acceptance and course/progress migration are complete.

Create the Play app with the exact package ID above and complete its store and
policy declarations. Run `Google Play app bundle` with
`destination: artifact-only`, then manually upload that first signed `.aab`.

After enabling the Google Play Developer API and granting the service account
access to this app, later runs may select `internal-track`. CI has no production
target. Each upload needs a new, increasing integer `version_code`; the current
sideloaded release used version code 2.
