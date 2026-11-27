# kind-of@1.0.0 (Agent Quickref)

> Fast zero-heap value & JSON/literal type classifier (clean-room CP implementation of npm kind-of) | License: MIT
> Import: `import { ... } from "kind-of";` | Full types: `src/index.d.cp`

## Functions
- `export fn koIsSpace(c: u8): bool;`
- `export fn koIsDigit(c: u8): bool;`
- `export fn koEq(s: string, start: usize, end: usize, lit: string): bool;`
- `export fn koStartsWith(s: string, start: usize, end: usize, prefix: string): bool;`
- `export fn koIsNumber(s: string, start: usize, end: usize): bool;`
- `export fn koIsBigInt(s: string, start: usize, end: usize): bool;`
- `export fn koIsIsoDate(s: string, start: usize, end: usize): bool;`
- `export fn kindOf(raw: string): u8;`
- `export fn kindName(kind: u8): string;`
- `export fn kindOfName(raw: string): string;`
- `export fn isPrimitive(raw: string): bool;`
- `export fn isPlainObject(raw: string): bool;`
