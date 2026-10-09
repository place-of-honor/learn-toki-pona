# Platform C division producer

The four arithmetic divisions in `native_main.c` use the literal `÷` token.
Score/progress strings retain their slash bytes. ICK
`c61e448251744a2f40ad743ebef1a027bdcd2f9d` parses and generates assembly for
`native_main.c` and `quiz_model.c`; NDK r27c assembles that output, compiles its
own native_app_glue, and links the Android platform libraries. API26 and all
three ABIs are preserved. Existing Fortify2, stack protection, warning errors,
debug information, section flags and link hardening remain enabled.

The shared producer and checked Fortify adapter are pinned at ai-ci
`015cc7901ae0b3ad262b476f24e129b53c56db95`. `ci/platform.mk` and the three-ABI
`platform-c.yml` job compile the actual owned platform sources through this
producer. Their output is object qualification, not an APK or Lua runtime claim.

Normal CMake/Gradle and the direct APK recipe use the same CMake source path.
Set `TOKI_PONA_AICI_ROOT` to that pinned shared checkout and
`TOKI_PONA_ICK_STAGE_ROOT` to a directory containing the qualified installed
compiler stages under `armeabi-v7a`, `arm64-v8a`, and `x86_64`. The separate
`TOKI_PONA_SESSION_OBJECT_ROOT` remains mandatory; every requested ABI still
checks the old source, header, policy and object receipts before linking.
No runtime receipt is synthesized and no missing ABI is dropped.

The ARM Grease producer retains its exact old compiler/runtime inputs and adds
two final arguments: the pinned ai-ci checkout and the current installed ARM
ICK stage. Its platform source pass uses the same hardened CMake producer.
The old Lua revision `87306483cec50f8c750a22dda1d0742246fad756`, old session
compiler `515c0f29fe6e2e96e10495fbaf25da93532e7722`, exact host runtime object,
policy bytes, and all session gate checks remain unchanged.

Local qualification on 9 October 2026 passes actual platform source generation
and NDK assembly for ARMv7, AArch64 and x86_64 at API26. The full ARM CMake
library also links against the retained exact session/runtime object closure;
its SHA256 is `bfb839baa74be4ed798ae985d71d4ab981a284f77b4cfa57098db89ca59040cd`.
Hosted qualification of the published head is required independently.

Full APK/AAB remains blocked by the previously missing qualified ARM64 and
Android x86_64 session/runtime closures. The old host session qualification is
separate from this current platform compiler qualification. Emulator and
physical device acceptance remain NOT_RUN; package, signer and version are
unchanged.
