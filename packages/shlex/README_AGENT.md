# shlex@1.0.0 (Agent Quickref)

> Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard (shell-quote / Rust shlex parity) for CP | License: MIT
> Import: `import { ... } from "shlex";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ShlexError`

## Functions
- `export fn needsQuote(arg: string): bool;`
- `export fn hasShellMetachars(s: string): bool;`
- `export fn quoteArg(arg: string, out: []u8): string;`
- `export fn joinArgs(args: []string, out: []u8): string;`
- `export fn splitShell(cmd: string): SplitResult;`
