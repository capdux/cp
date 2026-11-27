# split2@1.0.0 (Agent Quickref)

> Zero-allocation streaming line splitter across arbitrary chunk boundaries (LF & CRLF) in pure CP | License: ISC
> Import: `import { ... } from "split2";` | Full types: `src/index.d.cp`

## Functions
- `export fn sp2Min(a: usize, b: usize): usize;`
- `export fn sp2CopyBytes(dst: []u8, dst_off: usize, src: &[]u8, n: usize);`
- `export fn splitLines(input: string, out_slices: []string): usize;`
