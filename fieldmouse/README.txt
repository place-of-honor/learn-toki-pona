FIELDMOUSE TRANSLATION OF THE RETIRED BROWSER PROGRAM
====================================================

This directory accounts for every JavaScript source file that existed at
main commit e7f27738b6ca79fb930b79ac6d75747f89541138.

The translation is deliberately not the reference implementation.  The current
executable reference is ../grease/toki-pona.ysh, written from scratch around
the plain quiz and dictionary files.

Current Fieldmouse can execute scalar expressions, branches, loops, logging,
and basic text-file operations.  It does not yet implement the collection,
user-function, property-access, or browser-host surfaces needed to execute the
old browser application.  The files here therefore preserve the old program's
semantic structure in Fieldmouse-oriented source notation while making those
missing language surfaces explicit.  They are a migration target, not an
acceptance receipt for today's Fieldmouse interpreter.

Every retired source file has one translation:

app/src/main/assets/www/core/drill_queue.js
  blob 2fde00b77cb133c9ef4ac099ea5f4b0d9b906253
  -> fieldmouse/core/drill_queue.fieldmouse

app/src/main/assets/www/core/progress_store.js
  blob 06384c25fef2f33d88596f8c445d8fa1d7314a2a
  -> fieldmouse/core/progress_store.fieldmouse

app/src/main/assets/www/core/sitelen_pona.js
  blob a20fd8317447d858624516326e9863f72e73a2d0
  -> fieldmouse/core/sitelen_pona.fieldmouse

app/src/main/assets/www/data/lessons.js
  blob 485495e13a419d6728f33b4e696458914fa5cf90
  -> fieldmouse/data/lessons.fieldmouse
  lesson content itself now lives canonically in ../quizzes.txt

app/src/main/assets/www/data/lexicon.js
  blob b4cf8ab0cc24a24dcca8ab9a1bfee5c4089e23bf
  -> fieldmouse/data/lexicon.fieldmouse
  word definitions now live in ../tokipona/ and ../dictionaries/

app/src/main/assets/www/ui/app.js
  blob 691dd2898cde091ab11f231dacb39140160e6e02
  -> fieldmouse/ui/app.fieldmouse

tests/browser_smoke_check.js
  blob 4804cbee1c7e8126829ad6ca8a399319657694d7
  -> fieldmouse/tests/browser_smoke_check.fieldmouse

tests/content.test.js
  blob 5655df62576a62f9308c3b2c5cec4e70a0452a0c
  -> fieldmouse/tests/content.fieldmouse

tests/drill_queue.test.js
  blob 25a35356aefda63d324438dbfbdd45d8ec903f7f
  -> fieldmouse/tests/drill_queue.fieldmouse

tests/lexicon_and_rendering.test.js
  blob 06b4849c14c50bb7519c6d45080b51224131d68b
  -> fieldmouse/tests/lexicon_and_rendering.fieldmouse

tests/progress_store.test.js
  blob aa3653fbd9d6a34585f64bc8448c0fd28ccffa94
  -> fieldmouse/tests/progress_store.fieldmouse

The old source remains recoverable from Git history at the blob IDs above.
No JavaScript runtime or Node package metadata remains in the working tree.
