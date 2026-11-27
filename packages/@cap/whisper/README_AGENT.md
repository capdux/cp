# @cap/whisper — AI Agent Quick Ref

whisper.cpp (v1.9.6) local speech-to-text (ASR) engine for CP via prebuilt dynamic library (`v1.9.6`, `MIT`)

## Import

```cp
import {
    WhisperError,
    WhisperContext,
    cp_whisper_version,
    cp_whisper_print_system_info,
    cp_whisper_lang_max_id,
    cp_whisper_lang_id,
    cp_whisper_lang_str,
    cp_whisper_init_from_file,
} from "@cap/whisper";
```

## Core Types & Constants

```cp
error WhisperError { InitFailed, OutOfMemory }
class WhisperContext { ctx_ptr: *auto!? } // methods: initFromFile(...) +3
```

## API Signatures

```cp
fn cp_whisper_version(): [*:0]&u8!;
fn cp_whisper_print_system_info(): [*:0]&u8!;
fn cp_whisper_lang_max_id(): i32;
fn cp_whisper_lang_id(lang: [*:0]&u8!): i32;
fn cp_whisper_lang_str(id: i32): [*:0]&u8!;
fn cp_whisper_init_from_file(path: [*:0]&u8!): *auto!?;
fn cp_whisper_free(ctx: *auto!?);
fn cp_whisper_n_vocab(ctx: *auto!?): i32;
fn versionString(): string;
fn systemInfo(): string;
fn langMaxId(): i32;
fn langId(code: string): i32;
fn langStr(id: i32): string;
```

## Minimal Usage

```cp
import { langId } from "@cap/whisper";

pub fn main() {
    let result = langId("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/whisper"`; install via `cap add @cap/whisper`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
