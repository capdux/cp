# stack-utils@2.0.6 (Agent Quickref)

> Zero-heap V8 stack trace line parser extracting function name, file path, line, column, and native flag | License: MIT
> Import: `import { ... } from "stack-utils";` | Full types: `src/index.d.cp`

## Functions
- `export fn stkuInvalid(): StackFrame;`
- `export fn stkuParseLoc(fn_name: string, loc: string): StackFrame;`
- `export fn parseStackLine(line: string): StackFrame;`
