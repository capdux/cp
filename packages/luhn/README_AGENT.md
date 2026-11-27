# luhn@1.0.0 (Agent Quickref)

> Zero-heap ISO/IEC 7812 Luhn Mod-10, Damm, and Verhoeff check digit algorithms in pure CP | License: MIT
> Import: `import { ... } from "luhn";` | Full types: `src/index.d.cp`

## Functions
- `export fn luhnValidate(input: &[]u8): bool;`
- `export fn luhnCheckDigit(payload: &[]u8): i32;`
- `export fn dammCheckDigit(payload: &[]u8): i32;`
- `export fn dammValidate(input: &[]u8): bool;`
- `export fn verhoeffCheckDigit(payload: &[]u8): i32;`
- `export fn verhoeffValidate(input: &[]u8): bool;`
