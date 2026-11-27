# argparse@1.0.0 (Agent Quickref)

> Zero-allocation Python-style CLI argument parser supporting short/long flags, counts, bundling, and required checks for CP | License: Python-2.0
> Import: `import { ... } from "argparse";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ArgAction`

## Functions
- `export fn argpStrEq(a: string, b: string): bool;`
- `export fn argpStripDashes(s: string): string;`
- `export fn argpFindEq(s: string): usize?;`
- `export fn argpResolveDest(def: ArgDef): string;`
- `export fn argpFindLong(defs: &[]ArgDef, name: string): usize?;`
- `export fn argpFindShort(defs: &[]ArgDef, ch: u8): usize?;`
