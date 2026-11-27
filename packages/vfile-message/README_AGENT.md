# vfile-message@4.0.2 (Agent Quickref)

> Zero-allocation structured lint/AST diagnostic message with unist Point/Position formatting | License: MIT
> Import: `import { ... } from "vfile-message";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Severity`

## Functions
- `export fn makePoint(line: u32, column: u32, offset: u32): Point;`
- `export fn makePosition(start: Point, end: Point): Position;`
- `export fn pointAtOffset(text: string, offset: usize): Point;`
- `export fn createMessage(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage;`
- `export fn stringifyPosition(pos: Position, out: []u8): usize;`
- `export fn formatMessage(msg: VFileMessage, out: []u8): usize;`
