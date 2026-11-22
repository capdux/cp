# `cap:roman` — 规范罗马数字与电子表格 Bijective Base-26 列号转换库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

零堆分配（Zero-Alloc）的纯 CP 罗马数字（`1..3999`）编解码与电子表格 Bijective Base-26 列号（`1 -> "A"` .. `16384 -> "XFD"` .. `u32::MAX -> "FXSHRXW"`）双向转换库。

## 核心特性

- **零堆分配**：全部编解码直接写入调用方缓冲区，无任何动态内存分配。
- **100% 严格规范罗马数字校验**：`romanDecode` 通过与 `romanEncode` 逆向比对，确保拒收一切非规范写法（`"IIII"`, `"VV"`, `"IC"`, `"IL"`, `"IIV"`, `"VX"`, `"XM"`, `"MMMM"` 等），并同时提供 `romanDecodeLenient` 兼容传统钟表 `"IIII"` = 4 等写法。
- **Bijective Base-26 电子表格列号**：支持完整 32-bit 正整数 `1..4294967295` 与 `"A".."FXSHRXW"` 的无溢出双向转换。

## 导出 API

### 协议常量

- `ROMAN_MIN: u16 = 1`
- `ROMAN_MAX: u16 = 3999`
- `ROMAN_MAX_STR_LEN: usize = 15`（`"MMMDCCCLXXXVIII"`）
- `MAX_COL_LETTERS_LEN: usize = 7`（`"FXSHRXW"`）

### 罗马数字编解码

- `romanEncode(n: u16, out: []u8): usize?`：编码 `1..3999` 为大写规范罗马数字。
- `romanEncodeLower(n: u16, out: []u8): usize?`：编码 `1..3999` 为小写规范罗马数字（如 `"xiv"`）。
- `romanDecode(s: []const u8): u16?`：大小写无关严格解码规范罗马数字，拒收一切非规范串。
- `romanDecodeLenient(s: []const u8): u16?`：宽松解码，兼容 `"IIII"` (4) 等传统钟表写法。
- `isValidRoman(s: []const u8): bool`：判断字符串是否为严格规范罗马数字。

### 电子表格 Bijective Base-26 列号

- `colIndexToLetters(col_1based: u32, out: []u8): usize?`：1-based 列号转大写字母（`0` 返回 `null`）。
- `lettersToColIndex(letters: []const u8): u32?`：字母转 1-based 列号（大小写无关，严格防 32-bit 溢出）。

## 使用示例

```cp
import std from "std";
import {
    romanEncode,
    romanDecode,
    colIndexToLetters,
    lettersToColIndex,
} from "cap:codec";

fn example() {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    if (n := romanEncode(2026, buf[0..])) {
        // buf[0..n] == "MMXXVI"
        * = romanDecode(buf[0..n]); // 2026
    }

    if (c := colIndexToLetters(16384, buf[0..])) {
        // buf[0..c] == "XFD"
        * = lettersToColIndex(buf[0..c]); // 16384
    }
}
```
