# @cap/chalk

Zero-allocation terminal string styling library for CP — clean-room MIT parity with npm `chalk` v5. Composes a 6-package dependency chain (`@cap/ansi-styles` → `@cap/color-convert` → `@cap/color-name` + `@cap/supports-color` → `@cap/has-flag`).

纯 CP 零堆分配终端着色库（对标 npm `chalk` v5），支持 4 级终端色彩能力自动探测（`0` 无色透传 / `1` 16 色 / `2` 256 色 / `3` Truecolor）、`rgb`/`bgRgb`/`hex`/`keyword` 智能降级以及组合样式渲染。

## Install / 安装

```bash
cap add @cap/chalk
```

## Dependencies / 跨包复用

- `@cap/ansi-styles`: SGR 转义序列、`wrapStyle` 与色彩转义格式化（底层串联 `@cap/color-convert` → `@cap/color-name`）
- `@cap/supports-color`: 终端色彩能力自动探测与 CLI 标志覆写（底层串联 `@cap/has-flag`）

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Chalk.new` | `(level: u8): Chalk` | Create styler with explicit level `0..3` |
| `Chalk.fromEnv` | `(env: TerminalEnv, argv: []const string): Chalk` | Auto-detect color level via `@cap/supports-color` |
| `red` / `green` / `yellow` / `blue` / `magenta` / `cyan` / `white` / `gray` | `(text: string, out: []u8): usize` | Foreground 16-color styling (plain passthrough when `level == 0`) |
| `bold` / `dim` / `italic` / `underline` / `strikethrough` | `(text: string, out: []u8): usize` | Text modifiers |
| `rgb` / `bgRgb` | `(r, g, b: u8, text: string, out: []u8): usize` | Automatic 4-level downsampling (`16M -> 256 -> 16 -> plain`) |
| `hex` / `keyword` | `(color: string, text: string, out: []u8): usize` | CSS Hex & CSS4 keyword colors with 4-level downsampling |
| `styled` | `(text, is_bold, is_underline, fg: StylePair, out): usize` | Zero-alloc composed modifier + color styling |

## Usage / 使用示例

```cp
import { Chalk, TerminalEnv, FG_CYAN } from "@cap/chalk";

const c = Chalk.new(3);
let buf: [128]u8 = undefined;
const n1 = c.rgb(255, 136, 0, "hello", buf[0..128]);
const n2 = c.styled("warn", true, true, FG_CYAN, buf[0..128]);
```
