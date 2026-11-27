# @cap/utf8proc@2.10.0 (Agent Quickref)

> Clean Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD), case-folding & charwidth library for CP | License: MIT
> Import: `import { ... } from "@cap/utf8proc";` | Full types: `src/index.d.cp`

## Functions
- `export fn utf8proc_version(): [*:0]&u8!;`
- `export fn utf8proc_unicode_version(): [*:0]&u8!;`
- `export fn utf8proc_charwidth(codepoint: i32): i32;`
- `export fn utf8proc_tolower(c: i32): i32;`
- `export fn utf8proc_toupper(c: i32): i32;`
- `export fn utf8proc_NFC(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFD(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKC(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKD(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn utf8proc_NFKC_Casefold(str: [*:0]&u8!): [*:0]u8!?;`
- `export fn free(ptr: *auto!?);`
- `export fn versionString(): string;`
- `export fn unicodeVersion(): string;`
- `export fn charWidth(cp: i32): i32;`
- `export fn toLower(cp: i32): i32;`
- `export fn toUpper(cp: i32): i32;`
