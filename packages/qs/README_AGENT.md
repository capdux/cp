# qs@1.0.0 (Agent Quickref)

> Zero-allocation query string parser and serializer with nested brackets, dot notation, arrays, strict null handling, and DoS limits for CP | License: MIT
> Import: `import { ... } from "qs";` | Full types: `src/index.d.cp`

## Functions
- `export fn isUnreserved(c: u8): bool;`
- `export fn needsDecode(s: &[]u8, plus_as_space: bool): bool;`
- `export fn parseDecimalIndex(s: &[]u8): usize?;`
- `export fn percentDecode(input: &[]u8, plus_as_space: bool, out: []u8): &[]u8 ! QsError;`
- `export fn percentEncode(input: &[]u8, space_as_plus: bool, out: []u8): &[]u8 ! QsError;`
- `export fn makeSegment(slice: &[]u8, base_off: u16, seg_rel_off: usize, in_pool: bool, is_empty_bracket: bool): QsPathSegment;`
- `export fn parseKeySegments(dec_key: &[]u8, key_off: u16, key_in_pool: bool, allow_dots: bool, max_depth: usize, out_segs: &mut [8]QsPathSegment): usize ! QsError;`
