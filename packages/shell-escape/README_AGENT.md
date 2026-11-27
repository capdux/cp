# shell-escape@1.0.0 (Agent Quickref)

> Zero-allocation POSIX sh and Windows CommandLineToArgvW argument escaper, command joiner, and env var name validator in pure CP | License: MIT
> Import: `import { ... } from "shell-escape";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSafePosixByte(b: u8): bool;`
- `export fn isSafePosixWord(arg: string): bool;`
- `export fn escapePosix(arg: string, out: []u8): usize;`
- `export fn needsWindowsQuotes(arg: string): bool;`
- `export fn escapeWindows(arg: string, out: []u8): usize;`
- `export fn joinPosixCommand(args: &[]string, out: []u8): usize;`
- `export fn isValidEnvName(name: string): bool;`
