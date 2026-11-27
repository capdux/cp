[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# target-lexicon

Zero-heap LLVM/Rust/Zig target triple parser and platform metadata resolver (arch, vendor, os, env, pointer width, endianness, executable suffix)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:target-lexicon` ([https://github.com/bytecodealliance/target-lexicon](https://github.com/bytecodealliance/target-lexicon))（原库名 `target-lexicon` → CP 包名 `target-lexicon`）

| Metadata | Value |
| --- | --- |
| Package | `target-lexicon` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add target-lexicon
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ArchKind` | `X86_64, Aarch64, X86, Arm, Riscv64, Riscv32, Wasm32, Wasm64 ...` |
| `enum` | `OsKind` | `Linux, Darwin, Windows, FreeBsd, OpenBsd, NetBsd, Android, Ios ...` |
| `enum` | `EnvKind` | `Gnu, Musl, Msvc, Eabi, Eabihf, None, Unknown` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TargetTriple` | `class` | Fields: `valid: bool, raw_arch: string, raw_vendor: string, raw_os: string, raw_env: string, arch: ArchKind, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseArch` | `(s: string): ArchKind` | Parses input via `parseArch` and returns `ArchKind`. |
| `parseOs` | `(s: string): OsKind` | Parses input via `parseOs` and returns `OsKind`. |
| `parseEnv` | `(s: string): EnvKind` | Parses input via `parseEnv` and returns `EnvKind`. |
| `pointerWidthFor` | `(a: ArchKind): u8` | Invokes `pointerWidthFor` and returns `u8`. |
| `isLittleEndianFor` | `(a: ArchKind, raw_arch: string): bool` | Checks predicate `isLittleEndianFor` and returns `bool`. |
| `parseTargetTriple` | `(raw: string): TargetTriple` | Parses input via `parseTargetTriple` and returns `TargetTriple`. |
| `exeSuffix` | `(t: TargetTriple): string` | Invokes `exeSuffix` and returns `string`. |
| `dylibSuffix` | `(t: TargetTriple): string` | Invokes `dylibSuffix` and returns `string`. |

## Minimal Example

```cp
import { parseArch } from "target-lexicon";

pub fn main() {
    let result = parseArch("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
