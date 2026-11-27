# `extract-stack` — AI Agent Quick Reference

- **Package**: `extract-stack` (`1.0.0`, MIT)
- **Origin**: `npm:extract-stack`
- **Import**: `import { extractStack, extractErrorHeader, extractStackLines, parseStackFrame, parseStackFrames, countStackFrames, StackFrame } from "extract-stack";`

## Types

```cp
class StackFrame {
    function_name: string,
    file: string,
    line: u32,
    column: u32,
    is_native: bool,
    is_async: bool,
    format: u8, // 0=UNKNOWN, 1=V8, 2=ZIG
    valid: bool,
}
```

## Core Signatures

```cp
fn extractStack(stack: string): string
fn extractErrorHeader(stack: string): string
fn extractStackLines(stack: string, out: []string): usize
fn parseStackFrame(frame_line: string): StackFrame
fn parseStackFrames(stack: string, out: []StackFrame): usize
fn countStackFrames(stack: string): usize
// Low-level helpers: exsIsHspace, exsIsFrameLine, exsFindFirstFrameOffset,
// exsTrimLine, exsEmptyFrame, exsParseLocation, exsParseU32, exsIsAllDigits,
// exsFindLastByte, exsStartsWith, exsStrEq, exsFindSubstr
```

## Minimal Pattern

```cp
let frames: [16]StackFrame = undefined;
let n = parseStackFrames(err_stack, frames[0..16]);
// frames[0..n] each have .valid == true, .file, .line, .column, .function_name
```

## Pitfalls

1. **Zero-copy lifetimes**: Returned strings and `StackFrame.file` / `function_name` slice directly into `stack`; do not outlive the input buffer.
2. **Check `out.len`**: `extractStackLines` and `parseStackFrames` stop writing once `out` is full; call `countStackFrames(stack)` first if you need total count.
3. **No frame found**: `extractStack` returns `""` when no indented `at ` line exists.
4. **Native frames**: V8 frames like `at Array.forEach (<anonymous>)` or `(native)` set `is_native = true` with `line = 0, column = 0`.
