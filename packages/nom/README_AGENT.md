# nom@1.0.0 (Agent Quickref)

> Zero-allocation streaming parser combinator primitives (tag, tagNoCase, take, takeUntil, character classes, delimited/separated sequences, integer parsers) for CP (clean-room MIT, inspired by Rust nom v7). | License: MIT
> Import: `import { ... } from "nom";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn nomFail(input: string): ParseRes;`
- `export fn nomOk(input: string, split_at: usize): ParseRes;`
- `export fn nomContainsByte(set: string, b: u8): bool;`
- `export fn nomIsDigit(b: u8): bool;`
- `export fn nomIsHexDigit(b: u8): bool;`
- `export fn nomHexVal(b: u8): u64;`
- `export fn nomIsAlpha(b: u8): bool;`
- `export fn nomIsAlnum(b: u8): bool;`
- `export fn nomIsSpace(b: u8): bool;`
- `export fn nomIsMultispace(b: u8): bool;`
- `export fn nomSliceEq(a: string, b: string): bool;`
- `export fn tag(input: string, expected: string): ParseRes;`
- `export fn tagNoCase(input: string, expected: string): ParseRes;`
- `export fn take(input: string, count: usize): ParseRes;`
- `export fn takeUntil(input: string, needle: string): ParseRes;`
- `export fn isNot(input: string, forbidden_chars: string): ParseRes;`
- `export fn isA(input: string, allowed_chars: string): ParseRes;`
- `export fn digit0(input: string): ParseRes;`
- `export fn digit1(input: string): ParseRes;`
- `export fn hexDigit1(input: string): ParseRes;`
- `export fn alpha0(input: string): ParseRes;`
- `export fn alpha1(input: string): ParseRes;`
- `export fn alphanumeric0(input: string): ParseRes;`
- `export fn alphanumeric1(input: string): ParseRes;`
- *(+13 more in `src/index.d.cp`)*
