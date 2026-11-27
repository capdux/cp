# morse-decoder@1.0.0 (Agent Quickref)

> Zero-heap ITU-R M.1677 International Morse Code encoder and decoder supporting A-Z, 0-9, punctuation, and '/' word separators. | License: MIT
> Import: `import { ... } from "morse-decoder";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeChar(c: u8): string;`
- `export fn decodeSymbol(sym: string): u8;`
- `export fn encodeText(input: string, out: []u8): usize;`
- `export fn decodeText(morse: string, out: []u8): usize;`
