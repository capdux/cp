[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# csv-parse

Zero-heap RFC 4180 CSV/TSV record parser and serializer with quoted fields, escaped quotes, embedded newlines, custom delimiters, and spreadsheet formula guard.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:csv-parse` ([https://github.com/adaltas/node-csv](https://github.com/adaltas/node-csv))（原库名 `csv-parse` → CP 包名 `csv-parse`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `csv-parse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add csv-parse
```

```cp
import {
    CsvFieldSpan,
    CsvParseResult,
    parseRecord,
    formatField,
    formatFieldSafe,
    formatRecord,
} from "csv-parse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CsvFieldSpan` | `class` | 字段: `start: usize, len: usize` |
| `CsvParseResult` | `class` | 字段: `field_count: usize, next_pos: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseRecord` | `(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult` | 通过 `parseRecord` 解析输入数据，返回 `CsvParseResult`。 |
| `formatField` | `(field: &[]u8, delim: u8, dst: []u8): i64` | 通过 `formatField` 编码或格式化输出，返回 `i64`。 |
| `formatFieldSafe` | `(field: &[]u8, delim: u8, dst: []u8): i64` | 通过 `formatFieldSafe` 编码或格式化输出，返回 `i64`。 |
| `formatRecord` | `(fields: &[]&[]u8, delim: u8, dst: []u8): i64` | 通过 `formatRecord` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { formatField } from "csv-parse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatField(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
