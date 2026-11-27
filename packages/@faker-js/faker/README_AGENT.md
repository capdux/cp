# @faker-js/faker@1.0.0 (Agent Quickref)

> Zero-allocation deterministic fake data generator (xoshiro256** PRNG, person/network/text/commerce/UUIDv4/Luhn card/collections) for CP | License: MIT
> Import: `import { ... } from "@faker-js/faker";` | Full types: `src/index.d.cp`

## Functions
- `export fn splitmixStep(state: &mut u64): u64;`
- `export fn rotl7(x: u64): u64;`
- `export fn rotl45(x: u64): u64;`
- `export fn appendByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendLowerAscii(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendCapitalized(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendU64Dec(out: []u8, pos: &mut usize, val: u64);`
- `export fn appendDecPadded(out: []u8, pos: &mut usize, val: u64, width: usize);`
- `export fn appendHexByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn isLeapYear(year: u64): bool;`
- `export fn daysInMonth(year: u64, month: u64): u64;`
- `export fn isValidLuhn(digits: &[]u8): bool;`
