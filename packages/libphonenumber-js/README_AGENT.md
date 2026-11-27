# libphonenumber-js@1.0.0 (Agent Quickref)

> Zero-allocation international phone number parser, validator, and formatter (E.164, International, National, RFC 3966) for CP | License: MIT
> Import: `import { ... } from "libphonenumber-js";` | Full types: `src/index.d.cp`

## Functions
- `export fn findCountryIndexByIso2(iso2: string): i32;`
- `export fn findCountryIndexByCode(cc: u16): i32;`
- `export fn countryCodeDigitLen(cc: u16): usize;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushGroupRange(out: []u8, pos: usize, buf: [16]u8, start: usize, end: usize): usize;`
- `export fn writeGroupedNational(out: []u8, pos: usize, cc: u16, buf: [16]u8, len: usize, sep: u8): usize;`
- `export fn validateNationalRules(cc: u16, buf: [16]u8, len: usize): bool;`
- `export fn supportedCountryCount(): usize;`
- `export fn countryCodeFromIso2(iso2: string): u16;`
- `export fn iso2FromCountryCode(cc: u16): string;`
- `export fn matchCallingCodePrefix(digits: [24]u8, n: usize): CcMatch;`
- `export fn parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber;`
- `export fn isValidPhone(raw: string, default_iso2: string): bool;`
