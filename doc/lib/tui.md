# CP 标准库：终端用户界面（TUI）布局与差分渲染引擎 (`cap:tui` / `cap.tui`)

## 一、 架构总览

`cap:tui` 是 CP 语言官方内置的纯原生终端用户界面（TUI）输入解析、弹性矩形布局与零闪烁差分渲染引擎，专为交互式 CLI AI Agent、状态仪表盘与全屏终端应用打造。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:tui"` 与 `import cap from "cap"; cap.tui.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/tui/input.cp` | `// Source: cp/lib/tui/input.cp (ANSI/VT100/xterm Keyboard Input Sequence Parser)` | `KeyCode`、`KeyModifiers`、`KeyEvent`、`InputParser`（ASCII/UTF-8/Ctrl/Alt/CSI/SS3 解析） |
| `lib/tui/layout.cp` | `// Source: cp/lib/tui/layout.cp (Flex Rect Layout & Margin Solver)` | `TuiError`、`Rect`、`Constraint`、`Layout` 垂直与水平精确弹性布局求解器 |
| `lib/tui/canvas.cp` | `// Source: cp/lib/tui/canvas.cp (2D Cell Buffer, Box Drawing & Zero-Flicker Diff Renderer)` | `BorderStyle`、`CellStyle`、`Cell`、`TuiBuffer`（文本裁剪、边框、Sparkline）与 `TuiDiff` 差分渲染 |
| `lib/tui/index.cp` | `// Source: cp/lib/tui/index.cp (Unified Exports for cap:tui)` | `cap:tui` 与 `cap.tui` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. ANSI/VT100/xterm 键盘序列解析（`input.cp`）

- **`KeyCode`**：`Char(u21)`、`Enter`、`Esc`、`Tab`、`BackTab`、`Backspace`、`Up`、`Down`、`Left`、`Right`、`Home`、`End`、`PageUp`、`PageDown`、`Delete`、`F(u8)`、`Unknown`
- **`KeyModifiers`**：`ctrl: bool`、`alt: bool`、`shift: bool`，提供 `none()`、`fromXtermParam(param)`、`eql(other)`
- **`KeyEvent`**：`code: KeyCode`、`mods: KeyModifiers`、`consumed_bytes: usize`，提供 `eql(other)`
- **`InputParser.parseNext(bytes: []const u8): KeyEvent?`**：
  - 支持普通 ASCII 与多字节 UTF-8 字符解码；
  - 支持 `Ctrl+A..Z`（`0x01..0x1A`）、`Alt+char`（`\x1b<c>`）、`Tab`/`BackTab`/`Enter`/`Backspace`/`Esc`；
  - 支持 CSI 方向键与功能键（`\x1b[A..D`、`\x1b[H`/`F`、`\x1b[Z`、`\x1b[3~`/`5~`/`6~`、`\x1b[1;5C` 等 xterm 修饰键及 CSI-u）；
  - 支持 SS3 功能键（`\x1bOP`..`\x1bOS` 即 `F1`–`F4`）。

### 2. 弹性矩形布局求解器（`layout.cp`）

- **`TuiError`**：`OutOfMemory`、`InvalidDimensions`、`InvalidUtf8`
- **`Rect`**：`x: u16`、`y: u16`、`w: u16`、`h: u16`
  - `Rect.init(x, y, w, h)`、`area(): u32`、`isEmpty(): bool`
  - `inner(pad_x: u16, pad_y: u16): Rect`（防下溢安全内缩）
  - `contains(px: u16, py: u16): bool`、`eql(other): bool`
- **`Constraint`**：`Fixed(u16)`、`Percent(u8)`、`Min(u16)`、`Fill(u16)`
- **`Layout`**：
  - `Layout.splitVertical(alloc, area, constraints): ([]Rect) ! TuiError`
  - `Layout.splitHorizontal(alloc, area, constraints): ([]Rect) ! TuiError`
  - 严格守恒不溢出父容器，整除余数按轮转公平分摊至 `Fill` 槽位。

### 3. 2D 单元格画布与零闪烁差分渲染（`canvas.cp`）

- **`BorderStyle`**：`Single`（`┌─┐│└┘`）、`Double`（`╔═╗║╚╝`）、`Rounded`（`╭─╮│╰╯`）、`Ascii`（`+-+|++`）
- **`CellStyle`**：`fg: u8?`、`bg: u8?`、`bold: bool`、`dim: bool`、`underline: bool`
- **`Cell`**：`ch: u21`、`style: CellStyle`
- **`TuiBuffer`**：
  - `TuiBuffer.init(alloc, w, h): TuiBuffer ! TuiError`、`deinit(*_)`、`clear(*_)`
  - `setCell(*_, x, y, ch, style)`、`getCell(x, y): Cell?`
  - `drawText(*_, rect, text, style)`（按 `rect` 与画布边界严格裁剪，支持 `\n` 换行与 UTF-8）
  - `drawBox(*_, rect, border, title, style)`（支持圆角、单/双线与 ASCII 边框及顶部标题）
  - `drawSparkline(*_, rect, values: []const f64, style)`（渲染 Unicode 8 阶柱形图 `▁▂▃▄▅▆▇█`）
- **`TuiDiff`**：
  - `TuiDiff.renderDiff(alloc, prev: *const TuiBuffer, curr: *const TuiBuffer): ([]u8) ! TuiError`
  - 仅针对发生变化的单元格发射最小 ANSI 光标定位 `\x1b[row;colH` 与差异化 SGR 样式序列，相邻水平变动省略冗余光标指令；前后缓冲完全一致时返回空切片。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    InputParser,
    Rect,
    Constraint,
    Layout,
    BorderStyle,
    CellStyle,
    TuiBuffer,
    TuiDiff,
} from "cap:tui";

pub fn demoTui(): !void {
    const alloc = std.heap.page_allocator;

    if (ev := InputParser.parseNext("\x1b[1;5C")) {
        * = ev;
    }

    const screen = Rect.init(0, 0, 80, 24);
    const rows = Layout.splitVertical(alloc, screen, [
        Constraint.Fixed(3),
        Constraint.Fill(1),
        Constraint.Fixed(1),
    ])?;
    defer alloc.free(rows);

    let prev = TuiBuffer.init(alloc, 80, 24)?;
    defer prev.deinit();
    let curr = TuiBuffer.init(alloc, 80, 24)?;
    defer curr.deinit();

    const style = CellStyle{ fg: 39, bold: true };
    curr.drawBox(rows[0], BorderStyle.Rounded, "CP Agent", style);
    curr.drawText(rows[0].inner(1, 1), "Ready", CellStyle.default());

    const diff = cap.tui.TuiDiff.renderDiff(alloc, &prev, &curr)?;
    defer alloc.free(diff);
}
```
