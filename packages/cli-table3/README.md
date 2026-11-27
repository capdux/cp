[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/table` — Zero-Dependency CLI Table Renderer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-table3` ([https://github.com/cli-table/cli-table3](https://github.com/cli-table/cli-table3))（原库名 `cli-table3` → CP 包名 `@cap/table`）

> Clean-room pure-CP CLI table rendering library modeled after npm `cli-table3` / `table` and Rust `comfy-table` / `tabled` | MIT License
>
> 零外部依赖的纯 CP 终端表格渲染库，对标 npm `cli-table3` / `table` 与 Rust `comfy-table` / `tabled`（MIT 协议）。

## Installation / 安装

```bash
cap add @cap/table
```

## Features / 核心特性

- **5 Border Presets / 5 种边框预设**: `Unicode`, `Rounded`, `Ascii`, `Markdown` (GFM alignment markers), `Compact`
- **Column Alignment / 列对齐**: `Left`, `Center`, `Right`
- **ANSI & CJK Display Width / ANSI 与 CJK 宽字符宽度**: `displayWidth` skips ANSI CSI/OSC escape sequences (`\x1b[...m`) and measures UTF-8 CJK / fullwidth / emoji characters as 2 terminal columns
- **Truncation & Padding / 截断与填充**: `truncateDisplay` (with `"…"` ellipsis and automatic `\x1b[0m` reset) and `padCell`
- **Dynamic Multi-Column Builder / 动态多列构建器**: `Table` handles jagged rows, empty tables, headers, per-column alignment, and `setMaxColWidth`

## Exported API / 导出 API

| Symbol / Signature | Description / 说明 |
| :--- | :--- |
| `BorderStyle` | `Unicode` / `Rounded` / `Ascii` / `Markdown` / `Compact` |
| `Align` | `Left` / `Center` / `Right` |
| `displayWidth(s: string): usize` | Visual terminal column width (ignores ANSI escapes, CJK = 2) |
| `truncateDisplay(alloc, s, max_width)` | Truncates to `max_width` columns with `"…"` ellipsis |
| `padCell(alloc, s, width, alignment)` | Pads cell text with spaces according to `Align` |
| `Table.init(alloc)` / `table.deinit()` | Creates / frees a `Table` builder |
| `table.setHeaders(headers)` | Sets table header cells |
| `table.addRow(row)` | Appends a data row (supports jagged column counts) |
| `table.setAlign(col, alignment)` | Sets 0-based column alignment |
| `table.setStyle(style)` | Sets border style preset |
| `table.setMaxColWidth(max_width)` | Caps column width (`0` = unlimited) |
| `table.render(alloc)` | Renders the table into an owned UTF-8 byte slice |

## Usage Example / 使用示例

```cp
const std = @import("std");
import { Table, BorderStyle, Align } from "@cap/table";

fn main() {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let tbl = Table.init(alloc);
    defer tbl.deinit();

    const headers: [3]string = [ "Service", "Status", "Latency" ];
    tbl.setHeaders(headers[0..]) catch unreachable;

    const r1: [3]string = [ "gateway", "\x1b[32mOK\x1b[0m", "12ms" ];
    const r2: [3]string = [ "支付中心", "\x1b[33mWARN\x1b[0m", "145ms" ];
    tbl.addRow(r1[0..]) catch unreachable;
    tbl.addRow(r2[0..]) catch unreachable;

    tbl.setStyle(BorderStyle.Rounded);
    tbl.setAlign(1, Align.Center) catch unreachable;
    tbl.setAlign(2, Align.Right) catch unreachable;

    const out = tbl.render(alloc) catch unreachable;
    defer alloc.free(out);
    std.debug.print("{s}\n", .{out});
}
```
