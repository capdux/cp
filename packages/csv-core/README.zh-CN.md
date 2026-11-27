[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# csv-core

> **源包溯源**：吸收自 **crates.io** [`csv-core`](https://github.com/BurntSushi/rust-csv)（v0.2 / `MIT`），CP 官方洁净室实现 `csv-core@1.0.0`。

纯 CP 实现的零堆分配流式 RFC 4180 CSV 读写状态机。

## 核心亮点

- **零堆分配流式 DFA**：`CsvReader.readField` 可处理任意分片输入与调用方输出缓冲，遇到缓冲边界返回 `CSV_READ_INPUT_EMPTY` 或 `CSV_READ_OUTPUT_FULL`，支持无缝续读且不丢字节。
- **零栈清零优化**：辅助统计函数（`countCsvRecords`、`countCsvFieldsInLine`）内部使用 `undefined` 初始化的 `[128]u8` 暂存缓冲，消除冗余栈清零。
- **完整 RFC 4180 兼容**：正确处理带逗号与嵌入换行符的引号字段、双引号转义（`""` → `"`）、CRLF/LF 换行、空字段（`a,,b`）及自定义分隔符（如 TSV `\t`、`;`）。
- **对称编码器**：`CsvWriter` 自动判断并包裹含分隔符、引号或换行的字段。

## 零成本迁移（Rust → CP）

直接沿用原包名 `csv-core`，无需改名：

**Rust：**
```rust
use csv_core::{Reader, ReadFieldResult, Writer};
```

**CP：**
```typescript
import {
    CsvReader, CsvWriter, CsvReadResult,
    CSV_READ_INPUT_EMPTY, CSV_READ_OUTPUT_FULL, CSV_READ_FIELD,
    CSV_READ_RECORD_END, CSV_READ_END,
    countCsvRecords, countCsvFieldsInLine
} from "csv-core";
```

## 实战示例

### 1. 流式解析 CSV 字段

```typescript
import {
    CsvReader, CSV_READ_FIELD, CSV_READ_RECORD_END, CSV_READ_END
} from "csv-core";

export fn parseAllFields(data: &[]u8): usize {
    let mut reader = CsvReader.new();
    let mut out: [256]u8 = undefined;
    let mut pos: usize = 0;
    let mut fields: usize = 0;

    while (true) {
        let res = reader.readField(data[pos..data.len], out[0..256]);
        pos += res.bytes_read;
        if (res.status == CSV_READ_FIELD) {
            let field = out[0..res.bytes_written];
            fields += 1;
            _ = field;
        } else if (res.status == CSV_READ_RECORD_END) {
            // 当前记录结束
        } else if (res.status == CSV_READ_END) {
            break;
        } else if (pos >= data.len) {
            let fin = reader.readField(data[data.len..data.len], out[0..256]);
            if (fin.status == CSV_READ_FIELD) { fields += 1; }
            if (fin.status == CSV_READ_END) { break; }
        }
    }
    return fields;
}
```

### 2. 编码单行 CSV 记录

```typescript
import { CsvWriter } from "csv-core";

export fn writeRow(out: []u8): usize {
    let writer = CsvWriter.new();
    let cols: [3]string = ["alice", "hello, \"world\"", "42"];
    return writer.writeRecord(cols[0..3], out);
}
```

## 核心 API 速览

- **状态常量 (`i32`)**：`CSV_READ_INPUT_EMPTY (0)`、`CSV_READ_OUTPUT_FULL (1)`、`CSV_READ_FIELD (2)`、`CSV_READ_RECORD_END (3)`、`CSV_READ_END (4)`。
- **`CsvReadResult`**：`status: i32`、`bytes_read: usize`、`bytes_written: usize`。
- **`CsvReader`**：`new()`、`withDelimiter(delim: u8)`、`readField(input, out)`、`line()`、`reset()`。
- **`CsvWriter`**：`new()`、`withDelimiter(delim: u8)`、`writeField(field, out)`、`writeRecord(fields, out)`。
- **统计工具**：`countCsvRecords(input)`、`countCsvFieldsInLine(input)`。

## 许可证

MIT
