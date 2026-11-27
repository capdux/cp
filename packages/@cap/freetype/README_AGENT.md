# @cap/freetype — AI Agent Quick Ref

FreeType 2.13.3 industrial-standard TrueType/SFNT font rasterization engine for CP (`v2.13.3`, `FTL`)

## Import

```cp
import {
    FreeTypeError,
    FtVersion,
    FtFace,
    FtLibrary,
    cp_ft_init,
    cp_ft_done,
    cp_ft_version,
    cp_ft_new_memory_face,
} from "@cap/freetype";
```

## Core Types & Constants

```cp
error FreeTypeError { InitFailed, InvalidState, FaceLoadFailed }
class FtVersion { major: i32, minor: i32, patch: i32 }
class FtFace { raw: *auto!? } // methods: numGlyphs(...) +1
class FtLibrary { raw: *auto!? } // methods: init(...) +3
```

## API Signatures

```cp
fn cp_ft_init(): *auto!?;
fn cp_ft_done(lib: *auto!?);
fn cp_ft_version(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!);
fn cp_ft_new_memory_face(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?;
fn cp_ft_done_face(face: *auto!?);
fn cp_ft_face_num_glyphs(face: *auto!?): i64;
```

## Minimal Usage

```cp
import { cp_ft_init } from "@cap/freetype";

pub fn main() {
    let result = cp_ft_init();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/freetype"`; install via `cap add @cap/freetype`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
