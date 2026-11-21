# cap:csv_stream

> 源码吸收自 `/data/dev/bun/src/bun_core/fmt.cp`（RFC 4180 CSV/TSV 规范），提供纯 CP、零外部依赖、零堆分配的流式 CSV/TSV 解析与编码器。

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
