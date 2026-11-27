# iban_validate@1.0.0 (Agent Quickref)

> Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter. | License: MIT
> Import: `import { ... } from "iban_validate";` | Full types: `src/index.d.cp`

## Functions
- `export fn countryLength(c0: u8, c1: u8): usize;`
- `export fn electronicFormat(input: string, out: []u8): usize;`
- `export fn friendlyFormat(input: string, out: []u8): usize;`
- `export fn mod97(electronic: string): u32;`
- `export fn isValid(input: string): bool;`
- `export fn computeCheckDigits(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool;`
