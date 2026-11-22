# cap:csv_stream

> 参考来源：/data/dev/bun/src/bun_core/fmt.cp | 吸收日期：2026-11-22

基于 RFC 4180 CSV/TSV 规范，提供纯 CP、零外部依赖、零堆分配的流式 CSV/TSV 解析与编码器。

## 导入方式

```cp
import {
    CsvField,
    CsvRow,
    CsvReader,
    unescapeField,
    needsCsvQuoting,
    formatCsvField,
    formatCsvRow,
} from "cap:csv_stream";
```

## 核心类型与 API

### `CsvField`

- `raw: []const u8`：原始字段切片（引号字段已剥离外层双引号）
- `was_quoted: bool`：是否被双引号包裹
- `has_escaped_quotes: bool`：内部是否包含 `""` 转义引号
- `is_row_end: bool`：是否为当前行末尾字段

### `CsvRow`

- `fields: [32]CsvField`、`len: usize`、`get(index: usize): CsvField?`

### `CsvReader`

- `CsvReader.init(src: []const u8, delimiter: u8): CsvReader`（支持 `,` / `\t` / `;`）
- `nextField(*_): CsvField?`：流式读取下一个字段；遇到未闭合引号时置位 `had_error` 并安全返回 `null`
- `nextRow(*_): CsvRow?`：读取整行（最多 32 列）
- `CsvReader.unescapeField(field: CsvField, out: []u8): ([]const u8)?`：还原 `""` -> `"`，未转义字段零拷贝返回原切片

### 编码函数

- `needsCsvQuoting(s: []const u8, delimiter: u8): bool`
- `formatCsvField(s: []const u8, delimiter: u8, out: []u8): ([]const u8)?`
- `formatCsvRow(fields: []const []const u8, delimiter: u8, out: []u8): ([]const u8)?`

## 使用示例

```cp
import { CsvReader, formatCsvRow } from "cap:csv_stream";

fn example() {
    let reader = CsvReader.init("name,note\n\"Alice\",\"hi, \"\"cp\"\"\"\n", ',');
    while (reader.nextRow() != null) {
        // 流式消费每行字段
    }
    let out: [128]u8 = undefined;
    const cols: [2][]const u8 = ["Bob", "a,b"];
    * = formatCsvRow(&cols, ',', &out); // Bob,"a,b"
}
```

## 上游行为契约与 RFC 4180 边界

1. **零拷贝快速路径 (`has_escaped_quotes`)**：
   - `CsvField.raw` 直接引用源数据切片，不含 `""` 转义引号时 `unescapeField` 直接零拷贝返回原切片，仅在 `has_escaped_quotes == true` 时写入 `out` 还原 `"" -> "`。
2. **多行引号字段与未闭合引号检测**：
   - 引号包裹字段内部允许包含 `\n`/`\r\n` 与分隔符；遇到未闭合双引号时 `CsvReader` 置位 `had_error` 并安全返回 `null` 停止迭代，防止错误吞并后续整表数据。
3. **列数上限与自定义分隔符**：
   - `CsvRow` 固定栈容量 32 列（零堆分配），超过 32 列的数据表建议直接使用 `nextField()` 逐字段流式消费；通过 `delimiter` 参数同一套状态机无缝兼容 CSV (`,`)、TSV (`\t`) 与分号表 (`;`)。
