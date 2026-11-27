# getopts@1.0.0 (Agent Quickref)

> Zero-allocation POSIX/GNU command-line option parser (short flags, clustered flags, --opt=val, -- separator, required options) in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "getopts";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum HasArg`
- `export enum Occur`

## Functions
- `export fn goptStrEq(a: string, b: string): bool;`
- `export fn goptFindEq(s: string): usize?;`
- `export fn goptFindByName(defs: &[]OptDef, count: usize, name: string): usize?;`
- `export fn goptFindShort(defs: &[]OptDef, count: usize, ch: string): usize?;`
- `export fn goptFindLong(defs: &[]OptDef, count: usize, name: string): usize?;`
- `export fn goptCountForIdx(vals: &[]OptVal, val_count: usize, idx: usize): usize;`
