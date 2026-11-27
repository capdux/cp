# object-inspect@1.13.4 (Agent Quickref)

> Zero-heap debug value inspector and string/bytes/number pretty-formatter with escape and truncation support | License: MIT
> Import: `import { ... } from "object-inspect";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultInspectOptions(): InspectOptions;`
- `export fn inspectString(s: string, opts: InspectOptions, out: []u8): usize;`
- `export fn inspectBool(v: bool, out: []u8): usize;`
- `export fn inspectInt(val: i64, out: []u8): usize;`
- `export fn inspectBytes(data: &[]u8, max_bytes: usize, out: []u8): usize;`
- `export fn inspectStringArray(items: &[]string, opts: InspectOptions, out: []u8): usize;`
