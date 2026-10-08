# Daily-session Icky Lua pass

The session starts immediately, advances through the same first 12 exercises,
scores the same choice/matching answers, shows the same final screen and
restarts on any tap on that screen. Matching keeps the original reversed right
column, duplicate completed-pair admission and penalty for any wrong pairing.

`policy/quiz-session.lua` owns reset/restart/advance and choice/matching state
sequencing. Its actual UTF-8 source is embedded unchanged into
`quiz_session.c`, parsed in text mode by modified Lua
`87306483cec50f8c750a22dda1d0742246fad756` (5.5.1). There is no ordinary Lua,
symbol-to-ASCII replacement, writable phone policy file or historical C policy
fallback. C admits native event/index domains and checks returned state bounds
before atomically replacing the session snapshot. Android input, rectangles,
JNI Canvas/Paint, course parsing and all numerical rendering remain native.
The adapter logs/finalizes on a VM fault and closes the VM on destruction.

The bounded C composition was executed before this Lua pass at
`7bd9afea6340fdc670e3b92249309cfcb6f91db6` (tree
`6b4eb80160c21a99c883266798912c2623d68ab4`, identical to locally executed
checkpoint `07aaf702deff180f69603580ac6bf1a1aa792ba6`). Its frozen source, independent
original-native function oracle and 180-event trace are in
`qualification/c-session`. Every event compares acceptance and all ten fields,
including navigation, final/restart, correct/wrong answers, partial and repeated
matching selections, wrong pairs, completed pair repeats and invalid domains.
The actual Lua trace compares byte for byte with that frozen C trace. A policy
that compiles but loses the matching mask is rejected by the original oracle.

The policy VM has a 131072-byte heap cap and 10000-instruction call cap. Module
key lookup and registry installation run inside a protected C callback, so
module allocation/metatable faults also return failure. Tests exercise actual
malformed symbolic source, text-only rejection, version/function schema,
infinite load/call loops, captured-data allocation failure, oversize source,
result types/count/mask/score/row/screen bounds and unavailable host libraries.
The previous snapshot remains unchanged and faults remain latched until an
explicit close/reinitialization. A failed allocation need not raise the peak of
successful allocations; the retained-closure test checks denials and the cap.

`scripts/test-session.grease` invokes the canonical exact runtime gate from
[isomorphisms/flexible-pipes PR #52, “Qualify modified Icky Lua interpreter and embedded runtime through exact ICK”](https://github.com/isomorphisms/flexible-pipes/pull/52)
at `5caaba31256bfb424638f8a3d4d554c56fa9a970`, then links the checked runtime
object into this consumer. It also executes the unchanged 56-exercise corpus
test and reproduces the original C checkpoint. The host workflow restores the
existing qualified ICK and exact Grease artifacts; it does not create a new
compiler or orchestrator.
The maintained host parser job is superseded by this exact ICK stage; no stock
host `cc` parser job remains. The shared build-toolchain manifest covers the
actual C-generation, assembly, platform C and link stages and is checked by the
qualified ICK-built verifier from ai-ci
`01608a2493fa409463f70e8fbfd8a123ef59ee85`.
Both producer output directories retain `third_party/lua-LICENSE`. Any later
packaging of a statically linked library/APK must retain that same notice.

## Android stage and current blocker

`scripts/build-session-armv7.grease` produces the ARMv7/API26 session and
runtime objects with qualified ICK. It uses the exact artifact headers and
`-masm-syntax-unified`, an actual compiler option required by the NDK integrated
assembler. NDK 27.2.12479018 (r27c) parses only the ICK-produced assembly for
those objects. Existing ordinary-C parser/Android adapter and foreign native
glue retain their declared NDK C stage; that legacy C profile is a stated
remaining boundary, not a claim that the whole app has completed a glyph pass.
The exact NDK platform link succeeds with no undefined symbols, and the ARM
library's literal policy bytes compare with source. Target execution is NOT_RUN.

All three original Android ABI requests remain: armeabi-v7a, arm64-v8a and
x86_64. `TOKI_PONA_SESSION_OBJECT_ROOT` must contain source-bound ICK session
and actual runtime objects plus `source.receipt.tsv` for each requested ABI.
CMake rejects missing or stale source/object receipts, including both headers
that define state layout, event enums and course bounds. Native direct builds
also require the explicit qualified-object linkage macro; Clang is never asked
to parse the new arrow module. The legacy direct APK script does not yet supply
this closure and is BLOCKED. ARM64 and Android x86_64 ICK object producers are
not qualified by this slice, so full APK/AAB builds remain BLOCKED. The job does
not shrink the ABI set, hide those checks, reuse an old app policy on another
ABI or claim a new APK. The existing package, signer and version are preserved.

No emulator, physical MIRO A1 or TAB_P10 acceptance was executed. This is a
reviewable host/ARM producer slice under
[ai-ci #235, “SUN IL-3: systems and utilities Functorial C → modified Icky Lua pass”](https://github.com/isomorphisms/ai-ci/issues/235),
with the remaining Android object closure as its concrete blocker.
