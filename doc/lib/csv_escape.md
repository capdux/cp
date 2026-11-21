# cap:csv_escape

> 源码吸收自 `/data/dev/bun/src/string/immutable.zig`（RFC 4180 CSV / TSV 规范），提供纯 CP、零外部依赖、零堆分配的 CSV/TSV 字段转义、行构造、严格反转义与电子表格公式前缀检测库。

## 导入方式

```cp
import {
    needsCsvQuotes,
    escapeCsvField,
    unescapeCsvField,
    formatCsvRow,
    isFormulaInjectionRisk,
} from "cap:csv_escape";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `needsCsvQuotes` | `fn needsCsvQuotes(field: []const u8, delim: u8): bool` | 含 `delim`、`"`、`\r`、`\n` 返回 `true` |
| `escapeCsvField` | `fn escapeCsvField(field: []const u8, delim: u8, out: []u8): ([]const u8)?` | 无需引号原样拷贝；需要时双引号包裹且 `"`→`""` |
| `unescapeCsvField` | `fn unescapeCsvField(raw: []const u8, out: []u8): ([]const u8)?` | 非引号原样拷贝；引号包裹还原 `""`→`"`，未闭合或孤立 `"` 返回 `null` |
| `formatCsvRow` | `fn formatCsvRow(fields: []const []const u8, delim: u8, crlf: bool, out: []u8): ([]const u8)?` | 构造整行并按 `crlf` 附加 `\r\n` 或 `\n` |
| `isFormulaInjectionRisk` | `fn isFormulaInjectionRisk(field: []const u8): bool` | 检测首字符 `=`/`+`/`-`/`@`/`\t`/`\r` 电子表格公式风险 |

## 使用示例

```cp
import { formatCsvRow, isFormulaInjectionRisk } from "cap:csv_escape";

fn example() {
    let buf: [128]u8 = undefined;
    const row: [3][]const u8 = ["Alice", "a,b", "say \"hi\""];
    * = formatCsvRow(&row, ',', true, &buf); // Alice,"a,b","say ""hi"""\r\n
    * = isFormulaInjectionRisk("=SUM(A1:A5)"); // true
}
```
