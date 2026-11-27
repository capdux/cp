# shell-quote@1.0.0 (Agent Quickref)

> Zero-heap POSIX shell argument quoting and command-line tokenization in pure CP (modeled after npm shell-quote) | License: MIT
> Import: `import { ... } from "shell-quote";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSafeBareChar(c: u8): bool;`
- `export fn needsQuoting(arg: &[]u8): bool;`
- `export fn quoteArg(arg: &[]u8, dst: []u8): i64;`
- `export fn quoteArgs(args: &[]&[]u8, dst: []u8): i64;`
- `export fn splitCommand(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult;`
