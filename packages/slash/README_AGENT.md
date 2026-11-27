# slash@1.0.0 (Agent Quickref)

> Zero-allocation Windows backslash <-> POSIX forward-slash path normalizer preserving extended-length (\\?\) paths for CP. | License: MIT
> Import: `import { ... } from "slash";` | Full types: `src/index.d.cp`

## Functions
- `export fn isExtendedLengthPath(path: string): bool;`
- `export fn slash(path: string, out: []u8): usize;`
- `export fn toBackslash(path: string, out: []u8): usize;`
- `export fn countSeparators(path: string): usize;`
