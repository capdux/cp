# cargo-platform — AI Agent Quick Ref

Zero-allocation LLVM/Rust target triple parser and Cargo cfg(...) expression evaluator for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TargetTriple,
    TargetEnv,
    CfgEvalResult,
    CfgParser,
    invalidTriple,
    parseTargetTriple,
    isIdentStart,
    isIdentCont,
} from "cargo-platform";
```

## Core Types & Constants

```cp
class TargetTriple { arch_start: usize, arch_len: usize, vendor_start: usize, vendor_len: usize, /* +6 fields */ }
class TargetEnv { arch: string, os: string, family: string, env: string, /* +2 fields */ }
class CfgEvalResult { matched: bool, ok: bool }
class CfgParser { pos: usize, ok: bool, depth: usize }
```

## API Signatures

```cp
fn invalidTriple(): TargetTriple;
fn parseTargetTriple(triple: string): TargetTriple;
fn isIdentStart(c: u8): bool;
fn isIdentCont(c: u8): bool;
fn skipWs(p: &mut CfgParser, src: string);
fn u32MatchesDecimal(n: u32, s: string): bool;
fn evalKeyValue(key: string, val: string, target: TargetEnv): bool;
fn evalFlag(flag: string, target: TargetEnv): bool;
fn parsePredicate(p: &mut CfgParser, src: string, target: TargetEnv): bool;
fn evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult;
```

## Minimal Usage

```cp
import { parseTargetTriple } from "cargo-platform";

pub fn main() {
    let result = parseTargetTriple("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cargo-platform"`; install via `cap add cargo-platform`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
