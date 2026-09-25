# Google Play release

Toki Pona Drills keeps its package ID, `org.tokipona.drills`.

The current Android build is a DEX-free `android.app.NativeActivity` package.

For the Play artifact, Gradle compiles the same C quiz engine and native
Canvas/touch UI used by the direct APK path and produces the protobuf manifest,
resources, assets, and ABI libraries needed for an App Bundle. Android Gradle
Plugin 8.13.2 currently inserts a 528-byte synthetic `base/dex/classes.dex`
even though this app has no Java/Kotlin source and declares
`android:hasCode="false"`.

That staging bundle is not the deliverable. `scripts/build_aab.sh` extracts
the base module's manifest/resources/assets/native libraries, omits the
synthetic DEX directory, and asks pinned `bundletool 1.18.3` to construct and
validate the final AAB. CI then generates a universal APK from the final AAB and
requires that both artifacts remain DEX-free. When upload-key variables are
present, the final AAB is signed with `jarsigner`, as required for App
Bundles.

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


## Reproducible bundletool boundary

`scripts/build_aab.sh` pins:

- bundletool: `1.18.3`
- SHA-256: `a099cfa1543f55593bc2ed16a70a7c67fe54b1747bb7301f37fdfd6d91028e29`

Build locally with:

```sh
sh scripts/build_aab.sh
```

The final artifact is written under `build/Toki-Pona-Drills-native-v<version>.aab`.
