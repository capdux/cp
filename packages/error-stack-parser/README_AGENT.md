# error-stack-parser@1.0.0 (Agent Quickref)

> Zero-allocation multi-language stack trace parser & cleaner supporting V8/Node/Bun, SpiderMonkey/Safari, and Rust/Zig/CP formats (error-stack-parser / stacktrace-parser / clean-stack parity) for CP | License: MIT
> Import: `import { ... } from "error-stack-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn isInternalLocation(file: string, is_native: bool): bool;`
- `export fn parseLocation(raw_loc: string): LocationInfo;`
- `export fn parseFrameLine(raw_line: string): StackFrame?;`
- `export fn parseStack(raw: string): ParsedStack;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize): usize;`
- `export fn formatFrame(f: StackFrame, out: []u8): string;`
- `export fn cleanStack(raw: string, out: []u8): string;`
