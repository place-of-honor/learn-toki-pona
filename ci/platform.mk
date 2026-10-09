ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ABI ?=
NDK ?=
CMAKE ?= cmake
NINJA ?= ninja
AICI_ROOT ?= $(ROOT)/.aici-platform
ICK ?=
ICK_COMPILER_OPTIONS ?=
ICK_TARGET_FLAGS ?=
ICK_HEADER_TARGET ?=
ICK_HEADER_OVERLAY ?=
BUILD ?= $(ROOT)/build/platform-$(ABI)

.PHONY: qualify
qualify:
	test -x "$(ICK)"
	"$(CMAKE)" -S "$(ROOT)/app/src/main/c" -B "$(BUILD)" -G Ninja -DCMAKE_MAKE_PROGRAM="$(NINJA)" -DCMAKE_TOOLCHAIN_FILE="$(NDK)/build/cmake/android.toolchain.cmake" -DANDROID_ABI="$(ABI)" -DANDROID_PLATFORM=android-26 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTOKI_PONA_PLATFORM_ONLY=ON -DTOKI_PONA_AICI_ROOT="$(AICI_ROOT)" -DICK_COMPILER="$(ICK)" -DICK_COMPILER_OPTIONS="$(ICK_COMPILER_OPTIONS)" -DICK_TARGET_FLAGS="$(ICK_TARGET_FLAGS)" -DICK_HEADER_TARGET="$(ICK_HEADER_TARGET)" -DICK_HEADER_OVERLAY="$(ICK_HEADER_OVERLAY)"
	"$(CMAKE)" --build "$(BUILD)" --verbose
	"$(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h "$(BUILD)/native_main.o" "$(BUILD)/quiz_model.o" > "$(BUILD)/objects.elf.txt"
	sha256sum "$(ROOT)/app/src/main/c/native_main.c" "$(ROOT)/app/src/main/c/quiz_model.c" "$(BUILD)/native_main.o" "$(BUILD)/quiz_model.o" > "$(BUILD)/source-object-digests.txt"
