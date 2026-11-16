# Coding Style

## Top Rule
- correct is more important than simple. simple is more important than
  fast. otherwise, fast is good.

## Encoding
- Source files: printable 7-bit ASCII + space + EOL only. No NUL, HT,
  VT, DEL, other controls, or any 8-bit chars.
- Indent with spaces only; no trailing spaces. Tabs allowed only in
  Makefiles (files named `*Makefile*`).
- Indent four spaces per level. Align if it clearly improves readability.
- End every file with a newline. Existing CRLF/LF stays; new files use LF.
- No 8-bit/multibyte encodings. UTF-8 only in `talk`, limited to
  ISO646-DE chars.
- **Never** touch non-text files when editing.

## Line Length
- Non-negotiable hard limit: **79 chars**. Wrap at outermost operator
  before col 79.
- Imitate existing multiline style.

## Negotiable Rules
- All rules below may be violated if it clearly improves clarity. Ask first.

## Strings
- In-game display strings: German, pure ASCII or ISO646-DE. Umlauts
  encoded as ASCII punctuation (`@[\]{|}~`). Never UTF-8/Latin-1. Those
  ASCII chars are unavailable for literal use in-game.
- `talk`: UTF-8 German. `conf/`: ISO646-DE declared as US-ASCII.
- stderr, comments, identifiers: English.
- German uses pre-1990 orthography + old masculine dative `-e`.
  English => modern US usage.

## Unfamiliar Code
- Do not modify GNU bc code or `conf/vendorScript.xml` XML unless
  you've studied the language first.

## Spacing
- Space before, no space after: `*` (pointer), `&` (lvalue ref),
  `&&` (rvalue ref).

## Integer Types
- Assume: 8-bit `char`, 16-bit `short`, 32-bit `int`. `long` is 32 or 64 bit.
- `short` only if needed.
- Fixed-size types from `<stdint.h>` only if needed.
- Default to signed; use `unsigned` only for bitwise ops.
- Cast `std::size_t` to `int` (all game objects are < `INT_MAX`
  bytes).
- Always write `unsigned int`, never bare `unsigned`.
- `char` for text, `int8_t`/`uint8_t` for numerical bytes. Unicode
  only in keyboard input.
- Signed overflow/underflow is UB.
- Use SDL `Uint8`/`Uint16` etc. where SDL determines the types.
- Default: `int`.

## Floating Point
- Use `double`; avoid `float` and `long double`. Avoid FP unless it
  simplifies code.
- **Never** use FP for game state logic.

## `long` Keyword
- Only where stdlib/external APIs require it, or we model a function
  directly on a stdlib function that requires it.
- Use `std::int64_t` instead of `long long`. No `long double`.

## Comma Operator
- Forbidden except in `for`/`while`/`do-while` parentheses.

## Language Standards
- C++: **C++11** (`-std=gnu++11`). C: **C89** (`-std=gnu90`).

## Const Correctness
- `constexpr` when compile-time evaluatable. `const` for non-mutating.
- Pointer to const: `const *`. Non-mutating pointer: `*const` is **forbidden**.
- `const` objects with pointers-to-mutable are still `const`.
- declare methods `const` if non-mutating their object.

## Casting (C++)
- No `reinterpret_cast` or C-style casts. Prefer `static_cast`, then
  `dynamic_cast`. Avoid `const_cast`.
- New single-arg constructors: `explicit`. Existing non-`explicit`:
  leave alone.
- Cast on non-obvious conversions. String literal to `std::string`
  needs no cast.

## Variable Declarations
- Minimize globals; keep existing unless told otherwise.
  Module-private: `static`, else anonymous namespace.
- `static` function prototypes: near top of `.cpp`, not in headers.
  Non-`static`: in matching header.
- C++: declare + assign together, innermost block. C/C-compatible
  headers: all declarations before statements.

## `auto` Usage
- OK for: iterators, repeated types on same line
  (`new`/`static_cast`), lambdas with trailing return.
- Always qualify: `auto *`, `auto &`.
- Never for single-keyword built-ins (`int`, `char`, etc.).
- Two-keyword built-ins (`unsigned int`) OK if rules above hold.
- Otherwise: no `auto`.

## C++ Class Design
- Rule of five: define all five or `= delete` unneeded ones (unnamed params).
- Move ops only if genuinely cheaper than copy and copies actually occur.
- Pass polymorphic objects by pointer/ref, not by value. Allocate
  polymorphic objects on heap.
- `delete` as soon as possible. don't defer for speed.

## Resource Management
- New code: RAII where feasible. Don't refactor existing non-RAII
  unless directly touched.

## Headers
- Some `.h` files must be C/C++ dual-includable. Keep them C-compatible.

## SDL Layer and Threading
- SDL entities (types, functions, constants, headers) only appear in
  `*_sdl.cpp`/`*_sdl.h`. Everything else goes through `Image`,
  `screenLock()`, etc. Do not increase the existing violations.
- SDL 1.2 is implemented by sdl12-compat over SDL2; `SDL_UpdateRects`
  in its `src/SDL12_compat.c` does the per-frame upload and is where to
  look when reasoning about video threading.
- Keep code compatible with old, real SDL 1.2.15 library if you can.
- A background thread (`screenRefreshThread` in `src/screen_sdl.cpp`)
  presents the screen every frame. It reads the video surface and
  mutates SDL's per-source-surface blit map.
- Lock every blit, fill, and pixel access with `screenLock()`/
  `screenUnlock()` when the screen is the source *or* the destination.
  Per-op locks prevent corruption but do not make a multi-op sequence
  atomic; hold the lock across a whole visual step (see `screenShake()`)
  when a torn frame would be visible, and release it before sleeps.
- `screenLock()` uses an SDL mutex, which is recursive on Linux, macOS,
  and Windows, so nested locking is safe and already used
  (`screenUpdate()` -> `screenRedrawTextArea()`).
- `SDL_MUSTLOCK()`/`SDL_LockSurface()` are no-ops here: sdl12-compat
  skips them for the video surface, and game surfaces are software and
  never `RLEACCEL`. `screenLock()` is the real protection.
- `screenMoving` (`std::atomic_bool`, `src/screen.cpp`) only suppresses
  implicit drawing from the main thread. The refresh thread ignores it,
  so it is not synchronization and must not replace `screenLock()`.
- `SDL_LockMutex(nullptr)` succeeds in SDL2 on all target platforms, so
  `screenLock()` after `screenRefreshThreadEnd()` is a silent no-op.
- The screen is 8-bit indexed. sdl12-compat keeps the app-visible
  `SDL_PixelFormat` separate from the SDL2 format whose palette it swaps
  during `SDL_UpdateRects`, so the palette pointer the app sees is
  stable and is only written by `SDL_SetPalette()`/`SDL_SetColors()`.
- `320x200` and `SCALED()` are baked into the drawing code. Do not
  generalize them without a codebase-wide refactor.

## General Style
- Mirror existing codebase: indentation, naming, `#include` order,
  comments, layout.
- if in doubt: 1TBS, K&R style
- single statements in if or if/else: keep on same line, no braces.
- multiple statements in if/else or too long for single line: do use
  extra lines and always use braces.
- `#include` all directly-used headers and only those.
- External libs (except libxml2): include only the main header.
- use `override`, not `virtual`, wherever appropriate
- Record discovered style conventions here.
- `U4ASSERT(cond, msg)` is for conditions that must never happen; it
  prints a message plus a trace and exits. It compiles out with
  `NDEBUG`, so keep a runtime guard when the check protects a
  pointer dereferencing or an out-of-range access.

## Pass-by-value
- `alignas(int)` types: pass/return by value.
- Built-ins: by value unless output params.
- New non-polymorphic class `T` with `sizeof(T) <= sizeof(int)`:
  declare `alignas(int)`, pass by value.
- Larger types, all polymorphic types: pass by ref or pointer,
  mimicking existing code.

# Build Environment (macOS)

macOS 10.13+ only. Use Xcode_26.app SDK, not Xcode.app or
CommandLineTools (its libc++ rejects `-mmacos-version-min=10.13`).

## Build Command (macOS)
```bash
cd src
SDKROOT=/Applications/Xcode_26.app/Contents/Developer/Platforms/\
MacOSX.platform/Developer/SDKs/MacOSX26.sdk \
    make -f Makefile.MacOS -j15
```
Output: `src/u4` (universal arm64+x86_64). Other binaries are auxiliary.
`make clean` before `make` if any headers touched.

## Key Flags (macOS)
- Compiler: `clang`/`clang++`. Min macOS: `10.13`. Arch: `arm64` + `x86_64`.
- C++: `-std=gnu++11`. Warnings: `-Wall -Wextra -Werror`.

## SDK (macOS)
- Path: `/Applications/Xcode_26.app/.../MacOSX26.sdk` (also `26.5`
  and generic `MacOSX.sdk` exist).
- Do NOT use `xcrun --show-sdk-path` (CommandLineTools): libc++ will fail.
