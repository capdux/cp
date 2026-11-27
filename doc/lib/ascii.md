# `cap:ascii` — Zero-Allocation ASCII & Byte-String Foundation

## 1. 概述
`cap:ascii` 是 CP 标准库与生态包的 L0 底层零堆 ASCII 与字节字符串基座，无 C 依赖、无堆分配、无有符号除法/取模，采用紧凑位运算与 8 字节块扫描，适用于词法分析、协议解析、HTTP/MIME 头部处理与文本清洗等高频场景。

## 2. API 速查

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isAscii` | `(data: []const u8): bool` | 8 字节块快速判定全部字节是否属于 7-bit ASCII（`0x00..=0x7F`） |
| `firstNonAscii` | `(data: []const u8): usize?` | 返回首个非 ASCII 字节的索引，全 ASCII 返回 `null` |
| `isSpace` | `(b: u8): bool` | 判定 ASCII 空白字符（`' '`、`\t`、`\n`、`\r`、`\v`、`\f`） |
| `isAlpha` | `(b: u8): bool` | 判定 ASCII 字母（`A-Z` / `a-z`） |
| `isDigit` | `(b: u8): bool` | 判定 ASCII 十进制数字（`0-9`） |
| `isAlnum` | `(b: u8): bool` | 判定 ASCII 字母或数字 |
| `isHexDigit` | `(b: u8): bool` | 判定 ASCII 十六进制字符（`0-9` / `a-f` / `A-F`） |
| `isPrintable` | `(b: u8): bool` | 判定可打印 ASCII 字符（`0x20..=0x7E`） |
| `isControl` | `(b: u8): bool` | 判定 ASCII 控制字符（`0x00..=0x1F` 或 `0x7F`） |
| `hexVal` | `(b: u8): u8?` | 将十六进制字符转换为数值 `0..=15`，非法返回 `null` |
| `hexCharLower` | `(n: u8): u8` | 将低 4 位转换为小写十六进制字符 |
| `hexCharUpper` | `(n: u8): u8` | 将低 4 位转换为大写十六进制字符 |
| `toLowerByte` | `(b: u8): u8` | 单字节 ASCII 大写转小写 |
| `toUpperByte` | `(b: u8): u8` | 单字节 ASCII 小写转大写 |
| `strEq` | `(a: string, b: string): bool` | 字节级精确相等比较 |
| `eqIgnoreCase` | `(a: string, b: string): bool` | 忽略 ASCII 大小写的相等比较 |
| `startsWith` | `(s: string, prefix: string): bool` | 判断前缀匹配 |
| `endsWith` | `(s: string, suffix: string): bool` | 判断后缀匹配 |
| `findByte` | `(s: string, ch: u8, start_pos: usize): usize?` | 从 `start_pos` 起正向查找字节 |
| `rfindByte` | `(s: string, ch: u8): usize?` | 反向查找最后一次出现的字节 |
| `trim` | `(s: string): string` | 零分配去除首尾 ASCII 空白 |
| `trimStart` | `(s: string): string` | 零分配去除前导 ASCII 空白 |
| `trimEnd` | `(s: string): string` | 零分配去除尾随 ASCII 空白 |
| `toLower` | `(s: string, out: []u8): usize` | 将 `s` 转小写写入 `out`，返回写入字节数 |
| `toUpper` | `(s: string, out: []u8): usize` | 将 `s` 转大写写入 `out`，返回写入字节数 |

## 3. 示例
```cp
import { isAscii, eqIgnoreCase, trim, toLower } from "cap:ascii";

let raw = "  Content-Type  ";
let header = trim(raw);
let ok = isAscii(header) && eqIgnoreCase(header, "content-type");

let buf: [32]u8 = undefined;
let n = toLower(header, buf[0..]);
```
