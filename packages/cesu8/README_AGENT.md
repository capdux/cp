# cesu8@1.1.0 (Agent Quickref)

> Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) to/from standard UTF-8 transcoder | License: MIT
> Import: `import { ... } from "cesu8";` | Full types: `src/index.d.cp`

## Functions
- `export fn toCesu8(utf8: &[]u8, modified_nul: bool, out: []u8): i64;`
- `export fn fromCesu8(cesu: &[]u8, modified_nul: bool, out: []u8): i64;`
- `export fn isValidCesu8AsciiOrBmp(utf8: &[]u8, modified_nul: bool): bool;`
