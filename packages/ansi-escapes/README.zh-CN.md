[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ansi-escapes

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI/VT100 terminal cursor control, line/screen erasing, OSC-8 hyperlinks, and window title escape sequence engine (ansi-escapes parity) for CP

## 安装与导入

```bash
cap add ansi-escapes
```

```cp
import { pushByte } from "ansi-escapes";
```

## 核心导出 API

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
- `export fn eraseLines(count: usize, out: []u8): string;`
- `export fn hyperlink(text: string, url: string, out: []u8): string;`
- `export fn setWindowTitle(title: string, out: []u8): string;`
- `export fn beep(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ansi-escapes@1.0.0`)，许可证：MIT
