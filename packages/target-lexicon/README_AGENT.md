# target-lexicon — AI Agent Quick Ref

Zero-heap LLVM/Rust/Zig target triple parser and platform metadata resolver (arch, vendor, os, env, pointer width, endianness, executable suffix) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ArchKind,
    OsKind,
    EnvKind,
    TargetTriple,
    parseArch,
    parseOs,
    parseEnv,
    pointerWidthFor,
} from "target-lexicon";
```

## Core Types & Constants

```cp
enum ArchKind { X86_64, Aarch64, X86, Arm, Riscv64, Riscv32, ... }
enum OsKind { Linux, Darwin, Windows, FreeBsd, OpenBsd, NetBsd, ... }
enum EnvKind { Gnu, Musl, Msvc, Eabi, Eabihf, None, ... }
class TargetTriple { valid: bool, raw_arch: string, raw_vendor: string, raw_os: string, /* +6 fields */ }
```

## API Signatures

```cp
fn parseArch(s: string): ArchKind;
fn parseOs(s: string): OsKind;
fn parseEnv(s: string): EnvKind;
fn pointerWidthFor(a: ArchKind): u8;
fn isLittleEndianFor(a: ArchKind, raw_arch: string): bool;
fn parseTargetTriple(raw: string): TargetTriple;
fn exeSuffix(t: TargetTriple): string;
fn dylibSuffix(t: TargetTriple): string;
```

## Minimal Usage

```cp
import { parseArch } from "target-lexicon";

pub fn main() {
    let result = parseArch("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"target-lexicon"`; install via `cap add target-lexicon`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
