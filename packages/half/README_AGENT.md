# half@1.0.0 (Agent Quickref)

> CP package half | License: MIT
> Import: `import { ... } from "half";` | Full types: `src/index.d.cp`

## Functions
- `export fn f32ToBits(x: f32): u32;`
- `export fn bitsToF32(b: u32): f32;`
- `export fn f32SliceToF16(src: &[]f32, dst: []u16): usize;`
- `export fn f16SliceToF32(src: &[]u16, dst: []f32): usize;`
