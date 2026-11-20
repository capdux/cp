# CP 标准库：电子表格坐标与公式 DAG 拓扑重算引擎手册 (`cap:sheet` / `cap.sheet`)

## 一、 架构总览

`cap:sheet` 是 CP 语言官方内置的纯原生、零 C 依赖电子表格计算引擎，专为 **财务报表引擎**、**数据分析工具** 与 **低代码计算图** 设计。

全模块 **零 C 依赖、零 `std.posix`、零 `@import("bun")`**，提供：
- **A1..ZZ9999 坐标体系 (`CellCoord`)**：支持 0-based 行列坐标与 `A1`..`ZZ9999` 字符串的严格双向解析与格式化；
- **强类型单元格值 (`CellValue`)**：支持 `Empty`、`Number(f64)` 与 `Text(string)`；
- **公式解析与求值引擎**：支持 `=` 前缀或直接表达式、`A1:C5` 区域切片、四则运算、比较运算、括号优先级及 7 大内置函数（`SUM`、`AVG`/`AVERAGE`、`MIN`、`MAX`、`COUNT`、`IF`、`ROUND`）；
- **双向依赖图与 Kahn 拓扑重算 (`Spreadsheet`)**：修改上游单元格自动标记全部传递下游公式格为脏状态，`recalculate()` 按拓扑序无遗漏重算全部受影响公式格，并精确拦截循环引用（`CycleDetected`）；
- **CSV 导出 (`toCsv`)**：导出求值结果的 RFC-4180 兼容 CSV 文本。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sheet/formula.cp` | `// Source: cp/lib/sheet/formula.cp (...)` | `SheetError`、`CellCoord`、`CellValue`、公式依赖提取与递归下降求值器 |
| `lib/sheet/sheet.cp` | `// Source: cp/lib/sheet/sheet.cp (...)` | `Spreadsheet` 容器、双向依赖边、Kahn 拓扑排序重算与 `toCsv` 导出 |
| `lib/sheet/index.cp` | `// Source: cp/lib/sheet/index.cp (...)` | `cap:sheet` 与 `cap.sheet` 统一公开导出入口 |

---

## 二、 核心 API 契约

### 1. 错误类型 (`SheetError`)

- `InvalidCellRef`：非法单元格引用（如 `""`、`"A0"`、`"AAA1"`、`"A10000"`、`"1A"`）
- `InvalidFormula`：公式语法错误、空公式、括号不匹配或函数参数个数错误
- `CycleDetected`：公式依赖图中存在循环引用（如 `A1 -> B1 -> A1`）
- `DivisionByZero`：除数为 `0` 或对零个数值单元格求 `AVG`
- `TypeMismatch`：算术或数值上下文遇到文本单元格
- `UnknownFunction`：调用未注册的公式函数
- `OutOfMemory`：内存分配失败

### 2. 坐标与值 (`CellCoord` / `CellValue`)

- `CellCoord { row: usize, col: usize }`（0-based）：
  - `CellCoord.parseRef(ref_str): CellCoord ! SheetError`
  - `coord.formatRef(alloc): ([]u8) ! SheetError`
  - `coord.key(): u64` / `CellCoord.fromKey(k): CellCoord` / `coord.eql(other): bool`
- `CellValue`：`Empty` | `Number: f64` | `Text: string`
  - `isEmpty(): bool`、`asNumber(): f64?`、`asText(): string?`、`eql(other): bool`

### 3. 电子表格 (`Spreadsheet`)

- `Spreadsheet.init(alloc): Spreadsheet` / `deinit(*_)`
- `setNumber(*_, ref_str: string, val: f64): !`
- `setText(*_, ref_str: string, text: string): !`
- `setFormula(*_, ref_str: string, raw_expr: string): !`
- `clear(*_, ref_str: string): !`
- `recalculate(*_): usize ! SheetError`：按 Kahn 拓扑序重算全部脏公式格，返回本次重算的公式格数量；检测到环时返回 `SheetError.CycleDetected`。
- `getValue(ref_str: string): CellValue ! SheetError`
- `getNumber(ref_str: string): f64? ! SheetError`
- `toCsv(alloc: std.mem.Allocator, max_row: usize, max_col: usize): ([]u8) ! SheetError`：导出 `[0..max_row, 0..max_col)` 区域求值结果的 CSV。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Spreadsheet } from "cap:sheet";

pub fn runBudgetSheet(): !f64? {
    const alloc = std.heap.page_allocator;
    let sheet = Spreadsheet.init(alloc);
    defer sheet.deinit();

    sheet.setNumber("A1", 100.0)?;
    sheet.setFormula("B1", "=A1 * 2")?;
    sheet.setFormula("C1", "=B1 + SUM(A1:B1)")?;
    * = sheet.recalculate()?;

    return sheet.getNumber("C1"); // 500.0
}
```
