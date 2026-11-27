# regex-lite — AI Agent Quick Ref

Zero-allocation linear-time Thompson NFA regex engine (Rust regex-lite v0.1 parity) composing regex-syntax and memchr (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RegexMatch,
    RlInst,
    RlClass,
    RlCompileCtx,
    RegexLite,
    rlEmptyInst,
    rlEmptyClass,
    rlNoMatch,
} from "regex-lite";
```

## Core Types & Constants

```cp
class RegexMatch { matched: bool, start: usize, end: usize } // methods: len(...)
class RlInst { op: u8, arg1: usize, arg2: usize }
class RlClass { bits: [4]u64 }
class RlCompileCtx { insts: [128]RlInst, inst_len: usize, classes: [16]RlClass, class_len: usize, /* +1 fields */ }
class RegexLite { insts: [128]RlInst, inst_len: usize, classes: [16]RlClass, class_len: usize, /* +5 fields */ } // methods: compile(...) +9
```

## API Signatures

```cp
fn rlEmptyInst(): RlInst;
fn rlEmptyClass(): RlClass;
fn rlNoMatch(): RegexMatch;
fn rlClassSet(c: &mut RlClass, b: u8);
fn rlClassTest(c: RlClass, b: u8): bool;
fn rlClassSetRange(c: &mut RlClass, lo: u8, hi: u8);
fn rlClassInvert(c: &mut RlClass);
fn rlClassAddShorthand(c: &mut RlClass, esc: u8);
fn rlHexVal(b: u8): usize;
fn rlEmit(ctx: &mut RlCompileCtx, op: u8, arg1: usize, arg2: usize): usize;
fn rlAllocClass(ctx: &mut RlCompileCtx): usize;
fn rlInsertSlot(ctx: &mut RlCompileCtx, at_pc: usize);
fn rlDecodeEscapeByte(pat: string, pos: usize): u8;
fn rlEscapeLen(pat: string, pos: usize): usize;
fn rlIsShorthand(esc: u8): bool;
fn rlCompileCharClass(ctx: &mut RlCompileCtx, pat: string, start_pos: usize): usize;
fn rlSkipGroupHeader(pat: string, pos: usize): usize;
fn rlCompileAtom(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
fn rlCompileQuant(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
fn rlCompileConcat(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
fn rlCompileAlt(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
fn rlIsWordAt(text: string, pos: usize): bool;
```

## Minimal Usage

```cp
import { rlDecodeEscapeByte } from "regex-lite";

pub fn main() {
    let result = rlDecodeEscapeByte("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"regex-lite"`; install via `cap add regex-lite`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
