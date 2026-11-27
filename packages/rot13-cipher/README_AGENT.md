# rot13-cipher@1.0.0 (Agent Quickref)

> Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities. | License: MIT
> Import: `import { ... } from "rot13-cipher";` | Full types: `src/index.d.cp`

## Functions
- `export fn shiftLetter(c: u8, norm_shift: u8): u8;`
- `export fn rot13(input: string, out: []u8): usize;`
- `export fn rot13InPlace(buf: []u8);`
- `export fn rot47(input: string, out: []u8): usize;`
- `export fn caesar(input: string, shift: i32, out: []u8): usize;`
- `export fn atbash(input: string, out: []u8): usize;`
