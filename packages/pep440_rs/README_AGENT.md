# pep440_rs@1.0.0 (Agent Quickref)

> Zero-heap PEP 440 Python version parser, total-order comparator, and specifier matcher in pure CP (modeled after Astral pep440_rs) | License: MIT OR Apache-2.0
> Import: `import { ... } from "pep440_rs";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PreKind`

## Functions
- `export fn pepInvalid(): Pep440Version;`
- `export fn pepIsDigit(c: u8): bool;`
- `export fn pepIsSpace(c: u8): bool;`
- `export fn pepToLower(c: u8): u8;`
- `export fn pepIsSep(c: u8): bool;`
- `export fn pepMatchWord(s: string, pos: usize, end: usize, word: string): bool;`
- `export fn pepReadU32(s: string, pos: usize, end: usize): PepNumRes;`
- `export fn parsePep440(s: string): Pep440Version;`
- `export fn pepCmpU32(a: u32, b: u32): i32;`
- `export fn pepPreRank(k: PreKind): i32;`
- `export fn pepStageRank(v: Pep440Version): i32;`
- `export fn comparePep440(a: Pep440Version, b: Pep440Version): i32;`
- `export fn pepPrefixMatch(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool;`
- `export fn pepMatchSingle(v: Pep440Version, raw: string): bool;`
- `export fn matchesSpecifier(ver: string, spec: string): bool;`
