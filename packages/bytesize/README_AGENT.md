# bytesize@1.0.0 (Agent Quickref)

> CP package bytesize | License: Apache-2.0
> Import: `import { ... } from "bytesize";` | Full types: `src/index.d.cp`

## Functions
- `export fn bszWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn bszUnitMultiplier(s: string): u64?;`
- `export fn bszFormatScaled(bytes: u64, unit: u64, suffix: string, out: []u8): usize;`
- `export fn formatSi(bytes: u64, out: []u8): usize;`
- `export fn formatIec(bytes: u64, out: []u8): usize;`
- `export fn parseByteSize(input: string): u64?;`
