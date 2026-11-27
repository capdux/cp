# ansi-escapes@1.0.0 (Agent Quickref)

> Zero-allocation ANSI/VT100 terminal cursor control, line/screen erasing, OSC-8 hyperlinks, and window title escape sequence engine (ansi-escapes parity) for CP | License: MIT
> Import: `import { ... } from "ansi-escapes";` | Full types: `src/index.d.cp`

## Functions
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pushUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn cursorTo(x: usize, out: []u8): string;`
- `export fn cursorToXY(x: usize, y: usize, out: []u8): string;`
- `export fn cursorMove(dx: i32, dy: i32, out: []u8): string;`
- `export fn cursorUp(n: usize, out: []u8): string;`
- `export fn cursorDown(n: usize, out: []u8): string;`
- `export fn cursorForward(n: usize, out: []u8): string;`
- `export fn cursorBackward(n: usize, out: []u8): string;`
- `export fn cursorLeft(): string;`
- `export fn cursorSave(): string;`
- `export fn cursorRestore(): string;`
- `export fn cursorHide(): string;`
- `export fn cursorShow(): string;`
- `export fn eraseLine(): string;`
- `export fn eraseEndLine(): string;`
- `export fn eraseStartLine(): string;`
- `export fn eraseDown(): string;`
- `export fn eraseUp(): string;`
- `export fn eraseScreen(): string;`
- `export fn clearTerminal(): string;`
- `export fn scrollUp(): string;`
- `export fn scrollDown(): string;`
- *(+4 more in `src/index.d.cp`)*
