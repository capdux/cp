# atoi@1.0.0 (Agent Quickref)

> Zero-heap overflow-checked integer parsing directly from byte slices (base 10, 16, 2..36) matching Rust atoi (262M+ downloads) in pure CP | License: MIT
> Import: `import { ... } from "atoi";` | Full types: `src/index.d.cp`

## Functions
- `export fn digitVal(ch: u8, radix: u32): i32;`
- `export fn fromRadixU64(bytes: string, radix: u32): ParseU64Result;`
- `export fn fromRadix10U64(bytes: string): ParseU64Result;`
- `export fn fromRadix16U64(bytes: string): ParseU64Result;`
- `export fn fromRadixI64(bytes: string, radix: u32): ParseI64Result;`
- `export fn fromRadix10I64(bytes: string): ParseI64Result;`
- `export fn atoiU64(s: string): ParseU64Result;`
- `export fn atoiI64(s: string): ParseI64Result;`
