# CP 标准库：RFC 4180 CSV & TSV 流式解析、写入与表格查询手册 (`cap:csv`)

## 一、 架构总览与设计准则

`cap:csv` 是 CP 语言官方内置的工业生产级纯 CP 表格数据交换与流式处理标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既可通过 `import { ... } from "cap:csv"` 导入，也支持 `import cap from "cap"; cap.csv.*` 懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层数据交换与 AI 管线 (cap:csv)               │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ 流式解析引擎       │ RFC 4180 写入器      │ 表头索引表与 NDJSON 导出       │
│ CsvParser         │ CsvWriter           │ CsvTable                      │
│ StreamingCsvReader│ CsvWriteOptions     │ columnIndex / getCell         │
│ CsvOptions / Row  │ escape_formula 防护 │ filterRows / toJsonLines      │
└───────────────────┴─────────────────────┴───────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/csv/parser.cp` | `// Source: cp/lib/csv/parser.cp (RFC 4180 CSV & TSV Streaming Parser)` | `CsvOptions`、`CsvRow`、`CsvParseError`、`CsvParser` 与 4 状态机增量 `StreamingCsvReader` |
| `lib/csv/writer.cp` | `// Source: cp/lib/csv/writer.cp (RFC 4180 CSV & TSV Writer)` | `CsvWriteOptions` 与 `CsvWriter`（自动按需加引号、`""` 转义与公式注入防护） |
| `lib/csv/table.cp` | `// Source: cp/lib/csv/table.cp (Header-Indexed CsvTable Query & JSON Export)` | `CsvTable`（表头索引、单元格查询、行过滤及 `toJsonLines` NDJSON 导出） |
| `lib/csv/index.cp` | `// Source: cp/lib/csv/index.cp (Unified Exports for cap:csv)` | `cap:csv` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 解析选项与流式解析（`parser.cp`）

- **`CsvOptions`**：
  - `delimiter: u8 = ','`、`quote: u8 = '"'`、`has_header: bool = true`、`trim_spaces: bool = false`、`comment_char: u8? = null`、`allow_flexible_columns: bool = false`、`max_record_bytes: usize = 1048576`；
  - `CsvOptions.defaults()`：标准 RFC 4180 CSV 选项；
  - `CsvOptions.tsv()`：制表符分隔（`\t`）TSV 预设。
- **`CsvRow`**：包含 `fields: [][]const u8` 与 `line_number: usize`，提供 `get(col)`、`len()`、`deinit()` 与 `CsvRow.freeRows(alloc, rows)`。
- **`CsvParseError`**：`UnterminatedQuote`、`FieldCountMismatch`、`RecordTooLarge`。
- **`CsvParser` / `StreamingCsvReader`**：
  - 严格遵守 RFC 4180：支持引号内逗号、引号内 `\r\n`/`\n` 换行、双引号转义 `"" -> "`、UTF-8 BOM (`0xEF 0xBB 0xBF`) 跨 chunk 自动剥离、空行与 `#` 注释行跳过、列数一致性校验及 `max_record_bytes` 单记录上限保护。
  - `CsvParser.parseAll(alloc, input, opts)`：一次性解析完整 CSV/TSV 载荷。
  - `StreamingCsvReader.init(alloc, opts)` / `feed(chunk)` / `finish()` / `deinit()`：支持任意字节切分（含单字节流、引号或转义跨 chunk 边界）。

### 2. 序列化与公式注入防护（`writer.cp`）

- **`CsvWriteOptions`**：`delimiter`、`quote`、`use_crlf`（默认 `true` 输出 `\r\n`）、`always_quote`、`escape_formula`（默认 `false`），提供 `defaults()` 与 `tsv()` 预设。
- **`CsvWriter`**：
  - `writeRow(fields)` / `writeCsvRow(&row)` / `writeField(field)` / `toOwnedSlice()` / `stringifyRows(alloc, rows, opts)`；
  - 启用 `escape_formula: true` 时，对以 `=`、`+`、`-`、`@` 开头的字段自动前置单引号 `'`，防止电子表格软件误执行公式。

### 3. 表头索引表格与 AI NDJSON 导出（`table.cp`）

- **`CsvTable`**：
  - `CsvTable.parse(alloc, input, opts)`：解析并分离表头行与数据行；
  - `columnIndex(name): usize?`、`getCell(row, col_name): ([]const u8)?`、`getCellAt(row, col)`、`getRow(row)`、`rowCount()`、`colCount()`；
  - `filterRows(alloc, col_name, expected)` 与 `filterRowsWhere(alloc, pred)`：按列值或自定义谓词筛选行索引；
  - `toJsonLines(alloc)`：每行序列化为紧凑单行 JSON 对象并换行（NDJSON），自动完成 RFC 8259 字符串转义，直接对接 LLM / RAG 数据管线；
  - `toCsv(alloc, write_opts)`：将表格重新序列化为 CSV/TSV 文本。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { CsvOptions, CsvTable, CsvWriter, CsvWriteOptions } from "cap:csv";
import cap from "cap";

pub fn demoCsvPipeline(): !void {
    const alloc = std.heap.page_allocator;
    const raw = "\xEF\xBB\xBFid,name,note\r\n1,\"Alice, A.\",\"line1\nline2\"\r\n2,Bob,\"said \"\"hi\"\"\"\r\n";

    let table = CsvTable.parse(alloc, raw, CsvOptions.defaults())?;
    defer table.deinit();

    const note = table.getCell(0, "note").?;
    const ndjson = table.toJsonLines(alloc)?;
    defer alloc.free(ndjson);

    let w = CsvWriter.init(alloc, CsvWriteOptions{ escape_formula: true });
    defer w.deinit();
    let row: [2][]const u8 = ["user", "=SUM(A1:A2)"];
    w.writeRow(&row)?;
    const safe_csv = w.toOwnedSlice()?;
    defer alloc.free(safe_csv);

    // 动态命名空间 cap.csv
    let tsv_rows = cap.csv.CsvParser.parseAll(alloc, "k\tv\n1\t2\n", cap.csv.CsvOptions.tsv())?;
    defer cap.csv.CsvRow.freeRows(alloc, tsv_rows);
    * = note;
}
```
