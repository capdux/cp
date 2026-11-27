# is-plain-obj@1.0.0 (Agent Quickref)

> Zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard (clean-room port of npm is-plain-obj) | License: MIT
> Import: `import { ... } from "is-plain-obj";` | Full types: `src/index.d.cp`

## Functions
- `export fn ipoIsSpace(ch: u8): bool;`
- `export fn ipoSliceEq(s: string, start: usize, end: usize, target: string): bool;`
- `export fn ipoIsDangerousSlice(s: string, start: usize, end: usize): bool;`
- `export fn isPlainObjectMeta(m: ValueMeta): bool;`
- `export fn isPlainJsonObject(json_text: string): bool;`
- `export fn isPlainObjectKey(key: string): bool;`
- `export fn hasDangerousProtoKey(json_text: string): bool;`
