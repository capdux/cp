# shell-escape — AI Agent Quick Ref

Zero-allocation POSIX sh and Windows CommandLineToArgvW argument escaper, command joiner, and env var name validator in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isSafePosixByte,
    isSafePosixWord,
    escapePosix,
    needsWindowsQuotes,
    escapeWindows,
    joinPosixCommand,
    isValidEnvName,
} from "shell-escape";
```

## API Signatures

```cp
fn isSafePosixByte(b: u8): bool;
fn isSafePosixWord(arg: string): bool;
fn escapePosix(arg: string, out: []u8): usize;
fn needsWindowsQuotes(arg: string): bool;
fn escapeWindows(arg: string, out: []u8): usize;
fn joinPosixCommand(args: &[]string, out: []u8): usize;
fn isValidEnvName(name: string): bool;
```

## Minimal Usage

```cp
import { isSafePosixWord } from "shell-escape";

pub fn main() {
    let result = isSafePosixWord("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shell-escape"`; install via `cap add shell-escape`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
