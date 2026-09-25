# Toki Pona drills

A small corpus of Toki Pona drills with two executable front ends:

- `grease/toki-pona.ysh` — terminal reference implementation;
- Android NDK `NativeActivity` — DEX-free touch UI using the same
  `quizzes.txt` data.

## Data

- `quizzes.txt` — seven lessons and 56 exercises as ordinary text.
- `tokipona/` — one newline-terminated definition file per current Linku word.
- `dictionaries/` — attributed dictionary snapshots and source registry.
- `TokiPonaQuizTypes.idric` — the current Idriç type sketch.

## Language history

`fieldmouse/` accounts for every retired JavaScript source/test file and keeps
the old browser architecture as Fieldmouse migration material. There is no
JavaScript or Node package surface in the current tree.

Grease was then written from scratch over the ordinary quiz data rather than
treating the Fieldmouse port as the product design.

## Native Android

The phone app now uses Android's platform `NativeActivity`:

- no Java or Kotlin application source;
- no Smali application class;
- no WebView;
- no `classes.dex`;
- C17 quiz parser and mutable quiz state;
- `AAssetManager` reads `quizzes.txt`;
- `AInputEvent` handles taps;
- Android `Canvas` and `Paint`, reached from C through JNI, draw the UI;
- `armeabi-v7a`, `arm64-v8a`, and `x86_64` builds.

The first native slice starts a 12-question daily drill immediately. It supports
both four-choice and four-pair matching exercises, shows the existing
explanations, and reports a session score. Persistence, calendar/history,
sitelen pona glyph rendering, and richer lesson selection are deliberately
later slices rather than inherited WebView architecture.

Build a sideloadable APK with:

```sh
scripts/build_apk.sh
```

The script needs Android SDK/build-tools 36 and NDK 27.2.12479018. It preserves
the existing `TOKI_PONA_SIGNING_KEY_*` signing boundary so a retained signing
key can continue updating an installed copy.

The Gradle path builds the same native program as an Android App Bundle:

```sh
./gradlew :app:bundleRelease
```

See `docs/google-play-release.md`.

## Fieldmouse

Every removed browser source and test file has an exact blob ID and a Fieldmouse
translation path in `fieldmouse/README.txt`. Current Fieldmouse does not yet
provide all collection/function/browser-host surfaces needed to execute that old
UI, so those files remain migration targets rather than runtime dependencies.

## Font

The repository retains **sitelen seli kiwen asuki** by KreativeKorp / jan
Lepeka under the SIL Open Font License 1.1 for the later native glyph-rendering
slice.

The repository's own source is MIT-licensed; vendored dictionary data retains
the licenses recorded beside each snapshot.
