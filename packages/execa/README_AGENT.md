# execa@1.0.0 (Agent Quickref)

> Zero-heap subprocess command builder, POSIX/Windows argument escaper, preferLocal PATH resolver, and ExecaResult inspector (npm execa equivalent, reuses @cap/npm-run-path) | License: MIT
> Import: `import { ... } from "execa";` | Full types: `src/index.d.cp`
> Deps: `npm-run-path`

## Functions
- `export fn exaPush(out: []u8, pos: usize, s: string): usize;`
- `export fn exaPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn exaNeedsQuote(arg: string): bool;`
- `export fn escapePosixArg(arg: string, out: []u8): usize;`
- `export fn formatCommand(file: string, args: []string, out: []u8): usize;`
- `export fn stripFinalNewline(s: string): string;`
- `export fn resolveExecPath(opts: ExecaOptions, out: []u8): usize;`
