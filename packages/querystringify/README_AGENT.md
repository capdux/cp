# querystringify@1.0.0 (Agent Quickref)

> Zero-allocation WHATWG application/x-www-form-urlencoded parser, serializer, and percent-encoding codec for CP | License: MIT
> Import: `import { ... } from "querystringify";` | Full types: `src/index.d.cp`

## Functions
- `export fn isFormSafe(b: u8): bool;`
- `export fn encodedSize(input: string, space_as_plus: bool): usize;`
- `export fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize;`
- `export fn decodedSize(input: string): usize;`
- `export fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize;`
- `export fn parseFormPairs(query: string, out: []FormPair): usize;`
- `export fn decodedMatches(raw: string, plus_as_space: bool, target: string): bool;`
- `export fn getParamDecoded(query: string, target_key: string, out: []u8): i64;`
