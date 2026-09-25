# Google Play release

Toki Pona Drills keeps its package ID, `org.tokipona.drills`.

The current Android build is a DEX-free `android.app.NativeActivity` package.
Gradle builds the same C quiz engine and native Canvas/touch UI used by the
direct APK path, packages `quizzes.txt` as an Android asset, and emits an
Android App Bundle targeting API 36.

CI rejects `classes.dex` and a `base/dex/` directory in the bundle.

## Signing continuity

The installed earlier APK has an existing signing identity. If Play installs
should update that sideloaded copy without an uninstall, retain its private key
and import it as the Play **app-signing key** during enrollment. Merely using it
as the CI upload key while asking Google to generate a different app-signing key
does not preserve update compatibility.

If that private key was not retained, Play can use a new identity, but devices
with the sideloaded copy must uninstall it once before installing the Play
build.

## GitHub environment and secrets

Create a protected environment named `google-play` with:

- `ANDROID_UPLOAD_KEYSTORE_BASE64`
- `ANDROID_UPLOAD_KEYSTORE_PASSWORD`
- `ANDROID_UPLOAD_KEY_ALIAS`
- `ANDROID_UPLOAD_KEY_PASSWORD`
- `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON`

The first four values describe the private upload key used by CI.

## First and later uploads

Create the Play app with the exact package ID above and complete its store and
policy declarations. Run `Google Play app bundle` with
`destination: artifact-only`, then manually upload that first signed `.aab`.

After enabling the Google Play Developer API and granting the service account
access to this app, later runs may select `internal-track`. CI has no
production target. Each upload needs a new, increasing integer `version_code`.
