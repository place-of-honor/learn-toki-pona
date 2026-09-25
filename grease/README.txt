GREASE REFERENCE IMPLEMENTATION
===============================

toki-pona.ysh is the executable reference implementation.

It is intentionally a new terminal program, not a transliteration of the
retired browser application.  It consumes quizzes.txt directly and therefore
does not depend on the Fieldmouse translation.

Current commands:

  grease/toki-pona.ysh daily
      Drill the first twelve exercises in curriculum order.

  grease/toki-pona.ysh lesson NUMBER
      Drill all eight exercises in one lesson.

  grease/toki-pona.ysh all
      Drill all fifty-six exercises.

  grease/toki-pona.ysh list
      Show lesson headings.

  grease/toki-pona.ysh word WORD
      Print the one-line definition in tokipona/WORD.

Multiple-choice exercises use A-D.  Matching exercises are presented as four
small numbered matches with the right-hand column reversed.

This first reference deliberately does not copy the old localStorage,
calendar, streak, or spaced-review architecture.  Those were premature UI
decisions in an application that never had a settled product design.  Their
old behavior is preserved in the Fieldmouse migration files and Git history.

Run this with the current Grease/YSH runtime.
