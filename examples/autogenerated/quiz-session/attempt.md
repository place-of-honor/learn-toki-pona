# Daily quiz session transition contract

This IL-3 slice preserves the native learner's current 12-question session,
question reset, score, reversed matching column, final screen and restart.
The owning repository is `place-of-honor/learn-toki-pona`, starting at
`ef55a23acd94cc501e7a0d5e44ab01296b881c30`.

The existing `TokiPonaQuizTypes.idric` is the Idriç data/type sketch. It is not
an executing owner of the current native state. The explicit job requests a
qualified C composition followed by actual modified Icky Lua orchestration;
this attempt therefore uses those two requested languages. It does not claim
an Idriç compilation or replace an Idriç implementation.

## Values and operations

`QuizSession` contains bounded session/question/score counts, a quiz or
finished screen, an answered flag, selected choice, four-pair matching mask,
selected left/right rows and whether a wrong matching pair was tried.
The admitted events are start, restart, next, choice, left row and right row.
The parser supplies the exercise kind and correct choice; the native input
adapter supplies a hit row. Source text, Android objects and pixel geometry
never enter the policy VM.

The central operation is
`quiz_session_apply : Session × Event × ExerciseKind × Index × CorrectIndex →
Accepted Session | Rejected`.
Rejected input or a failed policy leaves the C snapshot byte fields unchanged.
Counts remain within the parsed course and current daily-session bounds;
choice/row indices are in 0..3; matching masks use exactly four bits; the score
cannot exceed answered questions. Restart preserves the session length and
resets question, score and per-question state. Next finishes the last question.
The right column presents pair 3 minus the visible row, exactly as before.

## Effects and evidence

The C parser, native Android input, geometry, JNI drawing and lifecycle remain
in their current owner. C validates incoming event domains and outgoing state
domains; Lua selects and sequences the session transitions. No policy file is
loaded from writable phone storage. Raw symbolic source is embedded unchanged.

The pre-Lua C composition and source-derived original oracle must agree on
every field after each event. Frozen traces include correct/wrong choices,
advance/final, restart, incomplete matching selections, wrong pairs, duplicate
completed pairs and all four correct pair selections. The Lua pass must agree
with the same traces. Parser errors, instruction/heap limits and invalid policy
results must reject without a C fallback.

Host compiler: exact ICK `515c0f29fe6e2e96e10495fbaf25da93532e7722`, GCC
reference `6294f1d9e7536e5ffcde09d1528c918d63abfef5`.
Actual modified Lua: `87306483cec50f8c750a22dda1d0742246fad756`, Lua 5.5.1.
Android declares NDK `27.2.12479018`; Android build/device evidence is separate.

## Idriç language work

The next Idriç-owned native learner slice would need the existing `QuizSession`
type sketch to execute bounded choice/matching transitions through the native
backend and expose a narrow C ABI. Its acceptance is these exact event traces,
not merely successful type checking of the sketch. That language work is not
claimed by the C/Lua pass.
