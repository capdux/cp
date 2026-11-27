# idna@1.0.0 (Agent Quickref)

> Zero-allocation UTS #46 Internationalized Domain Names (IDNA domainToAscii / domainToUnicode) reusing @cap/icu-normalizer for NFC and @cap/punycode for RFC 3492 Punycode (idna crate parity) in pure CP. | License: MIT
> Import: `import { ... } from "idna";` | Full types: `src/index.d.cp`
> Deps: `icu_normalizer`, `punycode`

## Functions
- `export fn idnaIsLdhLower(ch: u8): bool;`
- `export fn idnaIsLdhAnyCase(ch: u8): bool;`
- `export fn isPunycodeLabel(label: string): bool;`
- `export fn idnaMapDomain(domain: string, out: []u8): i64;`
- `export fn idnaValidateAsciiLabelLower(label: &[]u8): i64;`
- `export fn idnaAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;`
- `export fn domainToAscii(domain: string, out: []u8): i64;`
- `export fn domainToUnicode(domain: string, out: []u8): i64;`
- `export fn isValidIdnaAscii(domain: string): bool;`
