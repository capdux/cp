# `cap:isbn` — ISO 2108 ISBN、ISO 3297 ISSN 与 GS1 条码校验转换库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

零堆分配（Zero-Alloc）的纯 CP 国际标准书号（ISBN-10 / ISBN-13）、国际标准期刊号（ISSN）与 GS1 零售商品条码（EAN-13 / EAN-8 / UPC-A）校验、格式化与双向转换库。

## 核心特性

- **零堆分配**：全部校验、转换与格式化均采用栈上固定缓冲或调用方输出切片。
- **ISO 2108 ISBN-10 / ISBN-13**：支持 Mod-11 加权 10..2（含 `'X'`/`'x'` 校验位）与 `"978"`/`"979"` 前缀 GS1 Mod-10 校验，并支持 `"978"` ISBN-10 <-> ISBN-13 无损双向转换（严格拒收 `"979"` 逆转 ISBN-10）。
- **ISO 3297 ISSN**：支持 8 位期刊号 Mod-11 加权 8..2 校验及标准 9 字节 `XXXX-XXXX` 格式化（自动规范化小写 `'x'` 为大写 `'X'`）。
- **GS1 通用条码校验**：支持 EAN-13、EAN-8、UPC-A 以及任意长度主体数字的通用 `eanCheckDigit`。
- **分隔符容错与严格边界防护**：自动忽略 `'-'` 与 `' '`，对非法字符、错误长度、错误前缀与输出缓冲不足一律严格拒收。

## 导出 API

### 长度常量

`ISBN10_LEN (10)`, `ISBN13_LEN (13)`, `ISSN_LEN (8)`, `ISSN_FORMATTED_LEN (9)`, `EAN13_LEN (13)`, `EAN8_LEN (8)`, `UPCA_LEN (12)`

### ISBN-10 / ISBN-13 校验与转换

- `isbn10CheckDigit(digits9: []const u8): u8?`：计算 9 位主体数字的 ISBN-10 校验位（`'0'..'9'` 或 `'X'`）。
- `isValidIsbn10(code: []const u8): bool`：校验 ISBN-10 合法性（兼容 `'x'`/`'X'` 与 `'-'`/`' '`）。
- `isbn13CheckDigit(digits12: []const u8): u8?`：计算 `"978"`/`"979"` 前缀 12 位主体数字的 ISBN-13 校验位。
- `isValidIsbn13(code: []const u8): bool`：校验 ISBN-13（严格校验 `"978"`/`"979"` 前缀与第 13 位校验码）。
- `isValidIsbn(code: []const u8): bool`：自动识别合法 ISBN-10 或 ISBN-13。
- `isbn10To13(isbn10: []const u8, out: []u8): usize?`：将合法 ISBN-10 转换为 13 字节纯数字 `"978"` ISBN-13。
- `isbn13To10(isbn13: []const u8, out: []u8): usize?`：将合法 `"978"` ISBN-13 转换为 10 字节纯数字 ISBN-10（`"979"` 返回 `null`）。

### ISSN 校验与格式化

- `issnCheckDigit(digits7: []const u8): u8?`：计算 7 位主体数字的 ISSN 校验位（`'0'..'9'` 或 `'X'`）。
- `isValidIssn(code: []const u8): bool`：校验 ISO 3297 ISSN。
- `formatIssn(code: []const u8, out: []u8): usize?`：校验并输出标准 9 字节 `XXXX-XXXX`。

### GS1 商品条码校验

- `eanCheckDigit(body_digits: []const u8): u8?`：通用 GS1 右起奇数位 ×3、偶数位 ×1 Mod-10 校验位计算。
- `isValidEan13(code: []const u8): bool`
- `isValidEan8(code: []const u8): bool`
- `isValidUpcA(code: []const u8): bool`

## 使用示例

```cp
import std from "std";
import {
    isValidIsbn,
    isbn10To13,
    isbn13To10,
    formatIssn,
    isValidEan13,
} from "cap:codec";

fn example() {
    let buf13: [13]u8 = std.mem.zeroes([13]u8);
    let buf10: [10]u8 = std.mem.zeroes([10]u8);
    let issn_buf: [9]u8 = std.mem.zeroes([9]u8);

    if (isValidIsbn("0-306-40615-2")) {
        if (n13 := isbn10To13("0-306-40615-2", buf13[0..])) {
            // buf13[0..n13] == "9780306406157"
            * = isbn13To10(buf13[0..n13], buf10[0..]);
        }
    }

    * = formatIssn("2434 561x", issn_buf[0..]); // "2434-561X"
    * = isValidEan13("4006381333931");          // true
}
```
