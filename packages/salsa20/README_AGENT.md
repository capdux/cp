# salsa20@0.10.2 (Agent Quickref)

> Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce) verified against official Bernstein/eSTREAM vectors | License: MIT
> Import: `import { ... } from "salsa20";` | Full types: `src/index.d.cp`

## Functions
- `export fn salsa20Block(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8);`
- `export fn salsa20Apply(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize;`
