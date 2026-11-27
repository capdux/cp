# @cap/ansi-escapes

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-escapes` ([https://github.com/sindresorhus/ansi-escapes](https://github.com/sindresorhus/ansi-escapes))

零堆分配的 CP ANSI/VT100 终端光标控制、行/屏擦除与 OSC-8 超链接引擎 — 洁净室 MIT 实现，对标 npm `ansi-escapes`（~3.5 亿/周）。

Zero-allocation ANSI/VT100 terminal cursor control, erasing, OSC-8 hyperlinks, and window title escape sequences for CP — clean-room MIT parity with npm `ansi-escapes` (~350M/wk).

## 安装 / Install

```bash
cap add @cap/ansi-escapes
```

## API 参考 / API Reference

### 光标控制 / Cursor Control
- `cursorTo(x, out)` — `\x1b[<x+1>G`
- `cursorToXY(x, y, out)` — `\x1b[<y+1>;<x+1>H`
- `cursorMove(dx, dy, out)` — 相对移动（`A`/`B`/`C`/`D`）
- `cursorUp(n, out)` / `cursorDown(n, out)` / `cursorForward(n, out)` / `cursorBackward(n, out)`
- `cursorLeft()` / `cursorSave()` / `cursorRestore()` / `cursorHide()` / `cursorShow()`

### 擦除与滚屏 / Erasing & Scrolling
- `eraseLine()` / `eraseEndLine()` / `eraseStartLine()`
- `eraseDown()` / `eraseUp()` / `eraseScreen()` / `clearTerminal()`
- `scrollUp()` / `scrollDown()`
- `eraseLines(count, out)` — 向上逐行擦除 `count` 行并归位至行首

### 现代终端特性 / Modern Terminal Features
- `hyperlink(text, url, out)` — OSC-8 可点击终端超链接
- `setWindowTitle(title, out)` — OSC-0 终端窗口标题
- `beep()` — 终端提示音 `\x07`

## 使用示例 / Usage

```cp
import { cursorToXY, eraseLines, hyperlink, cursorHide, cursorShow } from "@cap/ansi-escapes";

let buf: [256]u8 = undefined;
const pos = cursorToXY(4 as usize, 2 as usize, buf[0..256]);     // "\x1b[3;5H"
const clr = eraseLines(3 as usize, buf[0..256]);                 // "\x1b[2K\x1b[1A\x1b[2K\x1b[1A\x1b[2K\x1b[G"
const link = hyperlink("CapDux", "https://capdux.org", buf[0..256]);
```

## License

MIT
