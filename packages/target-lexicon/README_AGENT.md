# target-lexicon@1.0.0 (Agent Quickref)

> Zero-heap LLVM/Rust/Zig target triple parser and platform metadata resolver (arch, vendor, os, env, pointer width, endianness, executable suffix) | License: MIT
> Import: `import { ... } from "target-lexicon";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ArchKind`
- `export enum OsKind`
- `export enum EnvKind`

## Functions
- `export fn parseArch(s: string): ArchKind;`
- `export fn parseOs(s: string): OsKind;`
- `export fn parseEnv(s: string): EnvKind;`
- `export fn pointerWidthFor(a: ArchKind): u8;`
- `export fn isLittleEndianFor(a: ArchKind, raw_arch: string): bool;`
- `export fn parseTargetTriple(raw: string): TargetTriple;`
- `export fn exeSuffix(t: TargetTriple): string;`
- `export fn dylibSuffix(t: TargetTriple): string;`
