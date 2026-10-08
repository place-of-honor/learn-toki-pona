# Bounded quiz session C → actual modified Icky Lua receipt

Qualified source commit: `df10b46e92d36a81b62155a161f70d0ecd4b3f46`.
Source tree: `c0f3b64b326abe74798ea2336402aeba381c7d8c`, identical to local executed checkout `652d20e98e6c643c9819d2e478e6170f72870129`.
This receipt-only continuation changes no consumer source.

The independently executed C-before-Lua checkpoint is remote `7bd9afea6340fdc670e3b92249309cfcb6f91db6`, local `07aaf702deff180f69603580ac6bf1a1aa792ba6`, identical tree `6b4eb80160c21a99c883266798912c2623d68ab4`. Its frozen original-native oracle and 180-event trace remain in `qualification/c-session`. Every event compares acceptance and all ten fields. The actual Lua trace matches the frozen trace SHA256 `3c678221855f507f9d6c1b27313ae1bfae0030d8892c0dfea94efe79679bbb9e`. The compiling matching-mask mutant fails the independent oracle.

The host receipt includes the actual loaded literal policy, unchanged 56-exercise parser output, protected module installation, heap/instruction/result-domain/parser rejection tests, immutable error snapshots and explicit recovery. The checked static Lua object SHA256 is verified before and after consumer links; no dynamic Lua dependency is loaded. The near-cap returned-closure input is rejected gracefully under the heap bound. Its attempted capture does not establish that module installation reached an allocation peak; no such claim is made.

The canonical runtime gate is [flexible-pipes PR #52](https://github.com/isomorphisms/flexible-pipes/pull/52), exact source `5caaba31256bfb424638f8a3d4d554c56fa9a970`. Its actual modified fork source is `87306483cec50f8c750a22dda1d0742246fad756`, tree `6b2968928611542c8f82448f49c32ea8bc98b559`, version 5.5.1. The gate producer run [37808205749](https://github.com/isomorphisms/flexible-pipes/actions/runs/37808205749) passed positive interpreter/embedded symbolic parsing and all eight stock/stale/substitution negative controls. The local consumer executes the same pinned gate; it does not normalize source.

ARMv7/API26 C generation through ICK, separately declared NDK assembly/platform compilation/link, ELF ABI checks, raw policy section comparison and strict undefined-symbol linking PASS. The exact CMake 3.22.1 clean ARMv7 build also passes. Independent session-layout and model-constant header mutations are rejected during configure, before linking. Target runtime execution is NOT_RUN.

The shared current ai-ci build-toolchain contract `01608a2493fa409463f70e8fbfd8a123ef59ee85` is compiled through exact ICK and passes all six manifest assertions. It preserves explicit NDK stage gaps. The maintained generic host `cc` parser job is superseded by the qualified ICK host workflow.

Full APK/AAB remains BLOCKED: the legacy direct APK recipe lacks the qualified object boundary; the retained multi-ABI Gradle path first lacks the ARM64 ICK session/runtime closure and also requires Android x86_64. All original requested ABIs are retained. No APK, emulator, physical MIRO A1/TAB_P10, merge or release acceptance is claimed. Both producer directories retain the actual static Lua notice.
