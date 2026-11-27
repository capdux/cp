# predicates@1.0.0 (Agent Quickref)

> Zero-heap composable boolean predicates (IntPredicate, StrPredicate, FloatPredicate) for testing and assertions in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "predicates";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum IntPredKind`
- `export enum StrPredKind`
- `export enum FloatPredKind`

## Functions
- `export fn prdToLowerAscii(b: u8): u8;`
- `export fn prdStrEq(a: string, b: string): bool;`
- `export fn prdStrEqIgnoreCase(a: string, b: string): bool;`
- `export fn prdStrHasPrefix(s: string, prefix: string): bool;`
- `export fn prdStrHasSuffix(s: string, suffix: string): bool;`
- `export fn prdStrContains(s: string, needle: string): bool;`
- `export fn prdIsFinite(x: f64): bool;`
