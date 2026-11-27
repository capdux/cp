# vfile-message — AI Agent Quick Ref

Zero-allocation structured lint/AST diagnostic message with unist Point/Position formatting (`v4.0.2`, `MIT`)

## Import

```cp
import {
    Severity,
    Point,
    Position,
    VFileMessage,
    makePoint,
    makePosition,
    pointAtOffset,
    createMessage,
} from "vfile-message";
```

## Core Types & Constants

```cp
enum Severity { Info, Warning, Error }
class Point { line: u32, column: u32, offset: u32 }
class Position { start: Point, end: Point }
class VFileMessage { reason: string, source: string, rule_id: string, file_path: string, /* +2 fields */ } // methods: isFatal(...) +1
```

## API Signatures

```cp
fn makePoint(line: u32, column: u32, offset: u32): Point;
fn makePosition(start: Point, end: Point): Position;
fn pointAtOffset(text: string, offset: usize): Point;
fn createMessage(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage;
fn stringifyPosition(pos: Position, out: []u8): usize;
fn formatMessage(msg: VFileMessage, out: []u8): usize;
```

## Minimal Usage

```cp
import { pointAtOffset } from "vfile-message";

pub fn main() {
    let result = pointAtOffset("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"vfile-message"`; install via `cap add vfile-message`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
