[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `cli-table3`

Zero-dependency CLI table renderer for CP — a clean-room MIT counterpart to npm [`cli-table3`](https://github.com/cli-table/cli-table3) (v0.6 parity), featuring 5 border styles (`Unicode`, `Rounded`, `Ascii`, `Markdown`, `Compact`), ANSI escape-aware & CJK wide-character width calculation, ellipsis truncation, and per-column alignment.

## Installation & Import

```bash
cap add cli-table3
```

```cp
import {
    BorderStyle,
    Align,
    displayWidth,
    truncateDisplay,
    padCell,
    Table,
} from "cli-table3";
```

## Core API

### Enums & Display Helpers

| Symbol | Signature / Variants | Description |
| :--- | :--- | :--- |
| `BorderStyle` | `Unicode = 0`, `Rounded = 1`, `Ascii = 2`, `Markdown = 3`, `Compact = 4` | Box-drawing & GFM border presets |
| `Align` | `Left = 0`, `Center = 1`, `Right = 2` | Horizontal cell alignment (emits `:---`, `:---:`, `---:` in Markdown mode) |
| `displayWidth` | `(s: string): usize` | Compute terminal column width (ignores ANSI CSI/OSC sequences; CJK wide chars count as `2`) |
| `truncateDisplay` | `(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error` | Truncate text to `max_width` columns with ellipsis while keeping ANSI reset safety |
| `padCell` | `(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error` | Pad cell text to exact terminal column width |

### `Table` Builder

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Table.init` | `(alloc: std.mem.Allocator): Table` | Initialize table bound to caller's allocator |
| `deinit` | `(*_)` | Release internal header/row/alignment lists (pair with `defer table.deinit()`) |
| `setHeaders` | `(*_, headers: []const string): void ! std.mem.Allocator.Error` | Set header row cells |
| `addRow` | `(*_, row: []const string): void ! std.mem.Allocator.Error` | Append a data row (handles jagged rows automatically) |
| `setAlign` | `(*_, col: usize, alignment: Align): void ! std.mem.Allocator.Error` | Set alignment for column `col` |
| `setStyle` | `(*_, style: BorderStyle)` | Choose border preset (`Unicode` default) |
| `setMaxColWidth` | `(*_, max_width: usize)` | Cap column width (`0` = unlimited) with automatic ellipsis truncation |
| `render` | `(alloc: std.mem.Allocator): []u8 ! std.mem.Allocator.Error` | Render full table into newly allocated `[]u8` (caller frees with `alloc.free(out)`) |

## Examples

### 1. Rendering a Rounded Unicode Table with Alignment & CJK Support

```cp
const std = @import("std");
import { print } from "cap:io";
import { Table, BorderStyle, Align } from "cli-table3";

fn renderReport(alloc: std.mem.Allocator): void ! std.mem.Allocator.Error {
    let tbl = Table.init(alloc);
    defer tbl.deinit();

    tbl.setStyle(.Rounded);
    tbl.setAlign(1, .Right)?;
    tbl.setAlign(2, .Center)?;

    const headers: [3]string = ["Package", "Downloads", "Status"];
    tbl.setHeaders(&headers)?;

    const row1: [3]string = ["zod", "150M/wk", "OK"];
    const row2: [3]string = ["中文表头测试", "42M/wk", "OK"];
    tbl.addRow(&row1)?;
    tbl.addRow(&row2)?;

    const rendered = tbl.render(alloc)?;
    defer alloc.free(rendered);
    print(rendered);
}
```

### 2. ANSI-Safe Display Width & Ellipsis Truncation

```cp
const std = @import("std");
import { print } from "cap:io";
import { displayWidth, truncateDisplay } from "cli-table3";

fn demoWidth(alloc: std.mem.Allocator): void ! std.mem.Allocator.Error {
    const colored = "\x1b[32mHello 世界\x1b[0m";
    print("columns:", displayWidth(colored)); // 10 (ANSI codes excluded, CJK = 2 each)

    const short = truncateDisplay(alloc, colored, 8)?;
    defer alloc.free(short);
    print(short);
}
```

## Differences from Upstream & Performance Highlights

- **Explicit Allocator Control (RAII)**: Every heap allocation flows through `std.mem.Allocator` with error propagation `?`, allowing arena allocators (`ArenaAllocator`) for one-shot CLI rendering and deterministic `defer` cleanup.
- **Built-in Markdown (GFM) & Compact Presets**: Beyond npm `cli-table3`'s box styles, includes native `BorderStyle.Markdown` (with alignment colons) and `BorderStyle.Compact`.
- **Accurate CJK & ANSI Width Engine**: Decodes UTF-8 codepoints and skips ANSI CSI/OSC sequences natively with zero external dependencies.
