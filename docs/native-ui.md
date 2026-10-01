# Native exercise UI

The native Android work is an **exercise renderer**, not a flash-card renderer.
The quiz domain remains separate from Android presentation.

## Current exercise kinds

The first executable slice has three touch interactions:

- `EXERCISE_CHOOSE_ONE` — tap one answer;
- `EXERCISE_WORD_BANK` — tap a word or grammar-particle tile;
- `EXERCISE_MATCH_PAIRS` — tap one item in each column until all pairs match.

The lesson-2 word-bank probe is `MI MOKU [ ] KILI.`, with `E`, `LI`, `LA`,
and `PI` as the choices. It proves that grammar can be practiced through a
different interaction rather than being reduced to front/back cards.

`TokiPonaQuizTypes.idric` is the language-level sketch of the quiz domain and
now includes a word-bank case. `quizzes.txt` remains the course-content source.
The three exercises in `app/native/exercise_model.c` are temporary executable
bring-up data, not a second canonical course.

Later renderers can add sentence ordering, multiple selection, identify-a-word,
typed production, listening, or other exercise kinds without changing the
Android event loop into the lesson model.

## Android boundary

The Gradle build uses the framework `android.app.NativeActivity`; there is no
application Java or Kotlin activity. `NativeActivity` loads
`libtoki_pona_native`, and NDK native-app-glue supplies lifecycle and touch
events.

The renderer locks `ANativeWindow` only when state changes and writes an RGB565
frame on the CPU. It does not run a continuous animation loop.

The default build contains `armeabi-v7a` and `arm64-v8a`. A phone-only ARMv7
artifact can be requested with `TOKI_PONA_ABIS=armeabi-v7a`.

The retired browser payload under `app/src/main/assets` remains repository
history, but Gradle excludes it from the native bundle.

## Input boundary

Touch word banks and sentence tiles cover many production exercises without an
IME. Free typing is deliberately not part of this first slice. If it is added,
keyboard/IME integration gets its own Android input boundary rather than being
mixed into touch hit-testing.

## Text boundary

The first renderer has a tiny built-in bitmap alphabet. That is enough to prove
window, layout, and interaction behavior. It is not final typography and does
not yet render the existing sitelen pona font.

## Evidence

The host tests execute the exercise state machine and touch hit-testing. The
Android CI job separately builds the real NDK library for both ARM ABIs and
checks the finished AAB contents.

Those stages do not prove physical-device installation, launch, touch behavior,
text appearance, persistence, or complete course coverage. Those remain
separate acceptance stages.
