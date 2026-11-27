# regex — AI Agent Quick Ref

PCRE2 10.44 Perl-compatible regular expression engine with SLJIT native JIT acceleration for CP (`v10.44.0`, `BSD-3-Clause`)

## Import

```cp
import {
    Pcre2Error,
    Pcre2Match,
    Pcre2Regex,
    cp_pcre2_compile,
    cp_pcre2_match,
    cp_pcre2_free,
    cp_pcre2_is_jit,
    cp_pcre2_version,
} from "regex";
```

## Core Types & Constants

```cp
error Pcre2Error { CompileFailed, MatchFailed }
class Pcre2Match { start: usize, end: usize } // methods: slice(...)
class Pcre2Regex { raw: *auto!? } // methods: compile(...) +8
```

## API Signatures

```cp
fn cp_pcre2_compile(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?;
fn cp_pcre2_match(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32;
fn cp_pcre2_free(re: *auto!?);
fn cp_pcre2_is_jit(re: *auto!?): i32;
fn cp_pcre2_version(): [*:0]&u8!?;
```

## Minimal Usage

```cp
import { cp_pcre2_version } from "regex";

pub fn main() {
    let result = cp_pcre2_version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"regex"`; install via `cap add regex`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
