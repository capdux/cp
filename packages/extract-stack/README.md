[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `extract-stack`

> **Source Attribution**: `npm:extract-stack` ([sindresorhus/extract-stack](https://github.com/sindresorhus/extract-stack)) — License: MIT

Zero-allocation, zero-copy `Error.stack` frame extractor and structured stack-frame parser supporting V8/Node.js and Zig stack traces in pure CP.

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`extract-stack`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: extract-stack)
import extractStack from "extract-stack";
const frames = extractStack(err.stack);
const lines = extractStack.lines(err.stack);
```

```cp
// CP (zero-copy slices & structured frame parsing)
import { extractStack, extractStackLines, parseStackFrames, StackFrame } from "extract-stack";

let raw = "Error: boom\n    at run (/app/main.ts:12:5)\n    at main (/app/main.ts:20:1)";
let frames_text = extractStack(raw);

let lines: [16]string = [""; 16];
let line_count = extractStackLines(raw, lines[0..16]);

let parsed: [16]StackFrame = undefined;
let frame_count = parseStackFrames(raw, parsed[0..16]);
```

## Installation

Add `extract-stack` to your `cap.json`:

```json
{
  "dependencies": {
    "extract-stack": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `StackFrame` | `class { function_name: string, file: string, line: u32, column: u32, is_native: bool, is_async: bool, format: u8, valid: bool }` | Parsed V8 (`format=1`) or Zig (`format=2`) stack frame |
| `extractStack` | `(stack: string): string` | Zero-copy slice starting at the first indented `at ...` frame line |
| `extractErrorHeader` | `(stack: string): string` | Zero-copy slice of the error name + (multi-line) message header |
| `extractStackLines` | `(stack: string, out: []string): usize` | Extract trimmed frame lines with leading `"at "` stripped |
| `parseStackFrame` | `(frame_line: string): StackFrame` | Parse a single V8/Node.js or Zig frame line into structured fields |
| `parseStackFrames` | `(stack: string, out: []StackFrame): usize` | Parse all stack frames from a full trace into `out` |
| `countStackFrames` | `(stack: string): usize` | Count valid stack frames without allocating an output array |
| `exs*` | Helper functions (`exsIsFrameLine`, `exsTrimLine`, `exsParseLocation`, etc.) | Low-level scanning and location parsing utilities |

## Practical Examples

### 1. Splitting Header and Stack Frames (Zero-Copy)

```cp
import { extractErrorHeader, extractStack, extractStackLines, countStackFrames } from "extract-stack";

let trace = "TypeError: invalid input\n  details: bad token\n    at parse (/src/parser.cp:42:9)\n    at main (/src/main.cp:10:3)";

let header = extractErrorHeader(trace); // "TypeError: invalid input\n  details: bad token"
let body = extractStack(trace);         // "    at parse (/src/parser.cp:42:9)\n    at main (/src/main.cp:10:3)"
let total = countStackFrames(trace);    // 2

let lines: [8]string = [""; 8];
let n = extractStackLines(trace, lines[0..8]);
// lines[0] == "parse (/src/parser.cp:42:9)"
// lines[1] == "main (/src/main.cp:10:3)"
```

### 2. Parsing Structured V8 & Zig Stack Frames

```cp
import { parseStackFrame, parseStackFrames, StackFrame } from "extract-stack";

// V8 / Node.js frame
let v8 = parseStackFrame("    at async fetchData (/app/net.ts:88:14)");
// v8.valid == true, v8.is_async == true, v8.function_name == "fetchData"
// v8.file == "/app/net.ts", v8.line == 88, v8.column == 14, v8.format == 1

// Zig backtrace frame
let zig = parseStackFrame("/app/src/main.zig:51:12: 0x104a20 in run (app)");
// zig.valid == true, zig.file == "/app/src/main.zig", zig.line == 51, zig.column == 12, zig.format == 2
```

## Notes & Edge Cases

- **Zero heap allocations**: Every string field (`function_name`, `file`, `lines[i]`) is a zero-copy sub-slice of the input `stack` string.
- **Multi-line error messages**: `extractStack` and `extractErrorHeader` safely skip multi-line error descriptions until the first indented `at ` frame.
- **Format codes**: `StackFrame.format` is `0` (unknown), `1` (V8/Node.js), or `2` (Zig).
