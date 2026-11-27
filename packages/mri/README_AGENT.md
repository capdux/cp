# mri@1.0.0 (Agent Quickref)

> Zero-allocation POSIX/GNU CLI argument parser (--flag, --key=val, --key val, -abc short clusters, -- terminator, stdin '-') in pure CP | License: MIT
> Import: `import { ... } from "mri";` | Full types: `src/index.d.cp`

## Functions
- `export fn strEq(a: string, b: string): bool;`
- `export fn parseArgs(argv: &[]string, out: []ParsedArg): usize;`
- `export fn hasFlag(argv: &[]string, long_name: string, short_char: u8): bool;`
- `export fn getOption(argv: &[]string, long_name: string, short_char: u8): string;`
- `export fn countPositionals(argv: &[]string): usize;`
