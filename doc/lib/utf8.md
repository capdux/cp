# `cap:utf8` — Strict RFC 3629 UTF-8 Codec & Boundary Foundation

## 1. 概述
`cap:utf8` 是 CP 标准库与生态包的 L0 零堆严格 UTF-8 编解码基座，完整遵循 RFC 3629，严格拒绝过长编码（Overlong Encodings）、UTF-16 代理对（`U+D800..=U+DFFF`）、超界码点（`> U+10FFFF`）、孤立续字节与截断尾部。

## 2. 核心类型
- `DecodedChar`：
  ```cp
  pub struct DecodedChar {
      codepoint: u32,
      len: usize,
      valid: bool,
  }
  ```

## 3. API 速查

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isValidUtf8` | `(data: []const u8): bool` | 严格校验整段字节流是否为合法 RFC 3629 UTF-8（含 8 字节 ASCII 快路径） |
| `decodeCodepoint` | `(data: []const u8, offset: usize): DecodedChar` | 从 `offset` 解码单个码点；非法或越界返回 `valid = false` 与 `U+FFFD` |
| `encodeCodepoint` | `(cp: u32, out: []u8): usize` | 编码合法标量码点至 `out` 返回写入字节数（1..=4）；非法码点或缓冲不足返回 0 |
| `encodeCodepointLossy` | `(cp: u32, out: []u8): usize` | 编码码点至 `out`，非法标量自动写入替换字符 `U+FFFD`（`EF BF BD`） |
| `utf8SeqLen` | `(lead_byte: u8): usize` | 根据合法首字节返回序列长度（1..=4），非法首字节返回 0 |
| `countCodepoints` | `(data: []const u8): usize` | 统计 UTF-8 码点个数 |
| `isCharBoundary` | `(data: []const u8, idx: usize): bool` | 判断索引 `idx` 是否位于 UTF-8 字符边界（含 `0` 与 `data.len`） |
| `floorCharBoundary` | `(data: []const u8, idx: usize): usize` | 向下对齐至不超过 `idx`（且不超 `data.len`）的最大合法字符边界 |

## 4. 示例
```cp
import { isValidUtf8, decodeCodepoint, encodeCodepoint, countCodepoints } from "cap:utf8";

let text = "Hello, 世界🦀";
let ok = isValidUtf8(text);
let total = countCodepoints(text);

let buf: [4]u8 = undefined;
let n = encodeCodepoint(0x1F980, buf[0..]);
let d = decodeCodepoint(buf[0..n], 0);
```
