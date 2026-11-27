# number_prefix@0.4.0 (Agent Quickref)

> Zero-heap SI decimal (k/M/G/T/P/E) and IEC binary (Ki/Mi/Gi/Ti/Pi/Ei) numeric prefix formatter | License: MIT
> Import: `import { ... } from "number_prefix";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PrefixKind`

## Functions
- `export fn decimalPrefix(amount: f64): PrefixedNumber;`
- `export fn binaryPrefix(amount: f64): PrefixedNumber;`
- `export fn formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize;`
