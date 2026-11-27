[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# cli-spinners

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-spinners` ([https://github.com/sindresorhus/cli-spinners](https://github.com/sindresorhus/cli-spinners))

Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols for CP — clean-room MIT parity with npm `cli-spinners` (35M/wk), `ora` (35M/wk), and `log-symbols` (45M/wk).

纯 CP 零堆分配终端加载动画帧集与状态指示器引擎，内置 12 套高频 Spinner 动画预设与 Unicode/ASCII 状态符号。

## Install / 安装

```bash
cap add cli-spinners
```

## Built-in Spinner Presets / 内置动画预设

| Preset | Interval | Frames | Sample |
| --- | --- | --- | --- |
| `Dots` | `80ms` | 10 | `⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏` |
| `Dots2` | `80ms` | 8 | `⣾ ⣽ ⣻ ⢿ ⡿ ⣟ ⣯ ⣷` |
| `Line` | `130ms` | 4 | `- \ \| /` |
| `Pipe` | `100ms` | 8 | `┤ ┘ ┴ └ ├ ┌ ┬ ┐` |
| `Arc` | `100ms` | 6 | `◜ ◠ ◝ ◞ ◡ ◟` |
| `Circle` | `120ms` | 4 | `◐ ◓ ◑ ◒` |
| `SquareCorners` | `180ms` | 4 | `◰ ◳ ◲ ◱` |
| `Bounce` | `120ms` | 4 | `⠁ ⠂ ⠄ ⠂` |
| `BouncingBar` | `80ms` | 8 | `[    ]` .. `[=== ]` |
| `Arrow` | `100ms` | 8 | `← ↖ ↑ ↗ → ↘ ↓ ↙` |
| `Clock` | `100ms` | 12 | `🕛 🕐 ... 🕚` |
| `AsciiDots` | `200ms` | 3 | `.  ` / `.. ` / `...` |

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SpinnerKind` | `enum { Dots, Dots2, Line, Pipe, Arc, Circle, SquareCorners, Bounce, BouncingBar, Arrow, Clock, AsciiDots }` | 12 built-in spinner presets |
| `SymbolKind` | `enum { Success, Error, Warning, Info }` | Status indicator symbol enum |
| `intervalMs` | `(kind: SpinnerKind) -> u32` | Recommended frame interval in ms |
| `frameCount` | `(kind: SpinnerKind) -> usize` | Total frames in preset |
| `frameAt` | `(kind: SpinnerKind, tick: usize) -> string` | Frame string at tick (auto-wraps) |
| `frameAtTime` | `(kind: SpinnerKind, elapsed_ms: u64) -> string` | Frame string at elapsed milliseconds |
| `findSpinnerByName` | `(name: string) -> SpinnerKind?` | Lookup preset by name (supports camelCase/kebab-case) |
| `logSymbol` | `(kind: SymbolKind, ascii_only: bool) -> string` | Unicode (`✔✖⚠ℹ`) or fallback (`√×!i`) symbol |
| `renderSpinnerLine` | `(kind: SpinnerKind, tick: usize, text: string, out: []u8) -> string` | Render `<frame> <text>` into caller buffer |
| `renderStatusLine` | `(sym: SymbolKind, ascii_only: bool, text: string, out: []u8) -> string` | Render `<symbol> <text>` into caller buffer |

## Usage / 使用示例

```cp
import {
    SpinnerKind,
    SymbolKind,
    frameAtTime,
    renderSpinnerLine,
    renderStatusLine,
} from "cli-spinners";

let buf: [128]u8 = undefined;
const line = renderSpinnerLine(SpinnerKind.Dots, 2, "Compiling modules...", buf[0..128]);
// "⠹ Compiling modules..."

const done = renderStatusLine(SymbolKind.Success, false, "Build succeeded", buf[0..128]);
// "✔ Build succeeded"
```
