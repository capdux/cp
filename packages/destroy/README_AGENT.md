# destroy@1.0.0 (Agent Quickref)

> Zero-heap safe idempotent stream and file-descriptor destroyer with lifecycle completion guard (npm destroy equivalent, reuses on-finished) | License: MIT
> Import: `import { ... } from "destroy";` | Full types: `src/index.d.cp`
> Deps: `on-finished`

## Functions
- `export fn destroyStream(s: &mut DestroyableStream): u8;`
- `export fn destroyWithError(s: &mut DestroyableStream, err_code: i32): u8;`
