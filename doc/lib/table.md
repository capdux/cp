# CP 标准库：ANSI 与东亚宽字符感知表格渲染引擎 (`cap:table`)

## 一、 架构总览与设计准则

`cap:table` 是 CP 语言官方内置的工业级纯 CP 终端与文本表格渲染引擎，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既可通过 `import { ... } from "cap:table"` 导入，也支持 `import cap from "cap"; cap.table.*` 动态懒挂载调用。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/table/width.cp` | `// Source: cp/lib/table/width.cp (...)` | `TextAlign`、`TableError`、`DisplayWidth`（ANSI CSI 跳过、UTF-8 东亚宽字符宽度、安全截断、词/字符折行、对齐填充） |
| `lib/table/table.cp` | `// Source: cp/lib/table/table.cp (...)` | `TableStyle`、`ColumnSpec`、`Table`（自适应列宽、换行展开、排序、Footer 汇总分隔线与 5 种风格渲染） |
| `lib/table/index.cp` | `// Source: cp/lib/table/index.cp (...)` | `cap:table` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 显示宽度与文本排版（`width.cp`）

- **`TextAlign`**：`Left`、`Center`、`Right`。
- **`TableError`**：`ColumnMismatch`、`InvalidWidth`、`OutOfMemory`。
- **`DisplayWidth`**：
  - `DisplayWidth.visibleWidth(s: string): usize`：跳过 `\x1b[...m` 等 ANSI CSI 转义序列，按 UTF-8 码点计算终端可见列宽（CJK/全角/Emoji 计 `2`、组合标记/控制字符计 `0`、常规 ASCII 计 `1`）。
  - `DisplayWidth.truncate(alloc, s, max_width, ellipsis): ([]u8)!`：按可见宽度截断并追加省略号，绝不切断多字节 UTF-8 字符；若输出包含 ANSI 序列则自动保留末尾 `\x1b[0m` 重置码。
  - `DisplayWidth.wrap(alloc, s, max_width): ([][]u8)!`：按 `\n` 与可见宽度上限进行词边界/字符边界折行；配合 `DisplayWidth.freeLines(alloc, lines)` 释放。
  - `DisplayWidth.pad(alloc, s, target_width, alignment): ([]u8)!`：按左/中/右对齐补齐空格至目标可见列宽。
  - `DisplayWidth.stripAnsi(alloc, s): ([]u8)!`：剥离全部 ANSI CSI 序列返回纯文本。

### 2. 表格构建与渲染（`table.cp`）

- **`TableStyle`**：`Ascii`、`Box`（默认）、`Rounded`、`Markdown`、`Borderless`。
- **`ColumnSpec`**：`title: string`、`alignment: TextAlign`、`max_width: usize`（0 为不限）、`min_width: usize`（0 为不限）。
- **`Table`**：
  - `Table.init(alloc)` / `deinit()`
  - `setStyle(style)` / `setPadding(left_right)`
  - `setColumns(cols)` / `addColumn(title, alignment)`
  - `addRow(cells): !`（严格校验列数与表头一致，否则返回 `TableError.ColumnMismatch`）
  - `setFooter(cells): !`（可选汇总行，渲染时带独立分隔线；列数不匹配同样返回 `ColumnMismatch`）
  - `sortByColumn(col_idx, ascending)`：忽略 ANSI 颜色码按字典序排序数据行
  - `sortNumericByColumn(col_idx, ascending)`：忽略 ANSI 颜色码按 `f64` 数值大小排序数据行
  - `render(): ([]u8)!`：计算自适应列宽，支持单元格 `\n` 与 `max_width` 自动换行，按 `TableStyle` 输出完整表格（`Markdown` 样式自动输出 `:---` / `:---:` / `---:` 对齐标记行）。

---

## 三、 快速使用示例

```typescript
const std = @import("std");
import { Table, TableStyle, TextAlign, ColumnSpec } from "cap:table";

pub fn main()! {
    const alloc = std.heap.page_allocator;
    let tbl = Table.init(alloc);
    defer tbl.deinit();

    tbl.setStyle(.Rounded);
    tbl.setColumns([
        ColumnSpec{ title: "服务", alignment: .Left },
        ColumnSpec{ title: "状态", alignment: .Center },
        ColumnSpec{ title: "延迟(ms)", alignment: .Right },
    ])?;
    tbl.addRow(["api-gateway", "\x1b[32m健康\x1b[0m", "12.4"])?;
    tbl.addRow(["worker-node", "\x1b[33m繁忙\x1b[0m", "48.1"])?;
    tbl.setFooter(["汇总", "2/2", "60.5"])?;

    const rendered = tbl.render()?;
    defer alloc.free(rendered);
    std.debug.print("{s}", .{rendered});
}
```
