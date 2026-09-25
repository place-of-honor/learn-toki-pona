# Toki Pona drills

A small corpus of Toki Pona drills with a terminal-first reference
implementation.

## Current structure

- `quizzes.txt` — the seven lessons and 56 exercises as ordinary text.
- `tokipona/` — one newline-terminated definition file per current Linku word.
- `dictionaries/` — attributed dictionary snapshots and source registry.
- `grease/toki-pona.ysh` — current executable reference, written from scratch
  in Grease/YSH.
- `fieldmouse/` — complete semantic translation/accounting of the retired
  browser source. It is migration material, not the reference implementation.
- `TokiPonaQuizTypes.idric` — today's Idriç type sketch for the quiz domain.

There is no JavaScript or Node package surface in the working tree.

## Run the terminal quiz

With the current Grease/YSH runtime:

```text
grease/toki-pona.ysh daily
grease/toki-pona.ysh lesson 2
grease/toki-pona.ysh all
grease/toki-pona.ysh list
grease/toki-pona.ysh word pona
```

The first Grease reference intentionally stays close to the original request:
drill the material in a terminal. It does not copy the abandoned browser
application's localStorage, calendar, streak, or spaced-review design.

## Fieldmouse migration

Every removed browser source and test file has an exact blob ID and a
Fieldmouse translation path in `fieldmouse/README.txt`. The current Fieldmouse
interpreter does not yet provide all of the collection/function/browser host
surfaces required to execute that old UI, so the translation is preserved as a
language-development target rather than treated as a runtime dependency.

## Android / Google Play

The Android wrapper, signing continuity, and manual Google Play workflow remain
in the repository because that deployment work was real. The old WebView
application logic is no longer the product reference. The bundled HTML page is
now only a static notice/design placeholder until a native phone UI is chosen.

See `docs/google-play-release.md` for the existing Play signing notes.

## Font

The Android assets still include **sitelen seli kiwen asuki** by KreativeKorp /
jan Lepeka under the SIL Open Font License 1.1.

The repository's own source is MIT-licensed; vendored dictionary data retains
the licenses recorded beside each snapshot.
