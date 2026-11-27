# potential_utf@1.0.0 (Agent Quickref)

> Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers (ICU4X potential_utf parity) for CP | License: Unicode-3.0
> Import: `import { ... } from "potential_utf";` | Full types: `src/index.d.cp`

## Functions
- `export fn putfIsCont(b: u8): bool;`
- `export fn putfValidSeqLen(bytes: &[]u8, pos: usize): usize;`
- `export fn putfMaximalSubpart(bytes: &[]u8, pos: usize): usize;`
- `export fn putfEncodeScalar(scalar: u32, out: []u8, w: usize): usize;`
- `export fn putfWriteReplacement(out: []u8, w: usize): usize;`
