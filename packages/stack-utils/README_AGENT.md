# stack-utils — AI Agent Quick Ref

Zero-heap V8 stack trace line parser extracting function name, file path, line, column, and native flag (`v2.0.6`, `MIT`)

## Import

```cp
import { StackFrame, stkuInvalid, stkuParseLoc, parseStackLine } from "stack-utils";
```

## Core Types & Constants

```cp
class StackFrame { function_name: string, file: string, line: u32, column: u32, /* +2 fields */ }
```

## API Signatures

```cp
fn stkuInvalid(): StackFrame;
fn stkuParseLoc(fn_name: string, loc: string): StackFrame;
fn parseStackLine(line: string): StackFrame;
```

## Minimal Usage

```cp
import { parseStackLine } from "stack-utils";

pub fn main() {
    let result = parseStackLine("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"stack-utils"`; install via `cap add stack-utils`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
