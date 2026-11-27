[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `cli-table3`

CP 零依赖命令行表格渲染器 —— 对齐 npm [`cli-table3`](https://github.com/cli-table/cli-table3)（v0.6）的纯 CP MIT 净室实现，内置 5 套边框样式（`Unicode`、`Rounded`、`Ascii`、`Markdown`、`Compact`）、ANSI 转义序列与 CJK 全角宽度计算、省略号截断及按列对齐。

## 安装与导入

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

## 核心 API

### 枚举与显示宽度工具函数

| 符号 | 签名 / 枚举值 | 说明 |
| :--- | :--- | :--- |
| `BorderStyle` | `Unicode = 0`, `Rounded = 1`, `Ascii = 2`, `Markdown = 3`, `Compact = 4` | 5 套终端/GFM 边框预设 |
| `Align` | `Left = 0`, `Center = 1`, `Right = 2` | 列对齐方式（Markdown 模式下自动输出 `:---`、`:---:`、`---:`） |
| `displayWidth` | `(s: string): usize` | 计算终端列宽（自动跳过 ANSI CSI/OSC 颜色序列，CJK 全角字符计为 `2`） |
| `truncateDisplay` | `(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error` | 按最大显示列宽截断并追加省略号，保证 ANSI 颜色重置安全 |
| `padCell` | `(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error` | 按目标列宽与对齐方式填充单元格 |

### `Table` 构建器

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Table.init` | `(alloc: std.mem.Allocator): Table` | 使用调用方分配器初始化表格 |
| `deinit` | `(*_)` | 释放内部列表资源（建议紧跟 `defer tbl.deinit()`） |
| `setHeaders` | `(*_, headers: []const string): void ! std.mem.Allocator.Error` | 设置表头行 |
| `addRow` | `(*_, row: []const string): void ! std.mem.Allocator.Error` | 追加数据行（自动兼容不等长锯齿行） |
| `setAlign` | `(*_, col: usize, alignment: Align): void ! std.mem.Allocator.Error` | 设置第 `col` 列的对齐方式 |
| `setStyle` | `(*_, style: BorderStyle)` | 切换边框风格（默认 `Unicode`） |
| `setMaxColWidth` | `(*_, max_width: usize)` | 限制最大列宽（`0` 表示不限制），超宽自动截断 |
| `render` | `(alloc: std.mem.Allocator): []u8 ! std.mem.Allocator.Error` | 渲染最终表格字符串（调用方需 `alloc.free(out)`） |

## 代码示例

### 1. 渲染圆角 Unicode 表格（支持对齐与中文宽字符）

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

### 2. ANSI 颜色感知宽度计算与省略号截断

```cp
const std = @import("std");
import { print } from "cap:io";
import { displayWidth, truncateDisplay } from "cli-table3";

fn demoWidth(alloc: std.mem.Allocator): void ! std.mem.Allocator.Error {
    const colored = "\x1b[32mHello 世界\x1b[0m";
    print("columns:", displayWidth(colored)); // 10（跳过 ANSI，每个中文占 2 列）

    const short = truncateDisplay(alloc, colored, 8)?;
    defer alloc.free(short);
    print(short);
}
```

## 与源包差异及性能亮点

- **显式分配器与确定性释放**：通过 `std.mem.Allocator` 与 `?` 错误传播管理内存，支持 Arena 分配器批量释放，无 GC 停顿。
- **原生内置 Markdown / Compact 风格**：除经典 Unicode/Rounded/ASCII 边框外，直接支持 GFM 表格与紧凑无外框模式。
- **零外部依赖**：内建 UTF-8 解码、CJK 宽字符区间判定与 ANSI 转义跳过逻辑。
