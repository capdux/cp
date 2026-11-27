# clean-stack@1.0.0 (Agent Quickref)

> Zero-allocation stack trace cleaner removing internal runtime frames and normalizing base paths (clean-room MIT implementation of npm clean-stack v5) | License: MIT
> Import: `import { ... } from "clean-stack";` | Full types: `src/index.d.cp`

## Functions
- `export fn isInternalStackFrame(line: string): bool;`
- `export fn cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize;`
- `export fn cleanStack(stack: string, out: []u8): usize;`
