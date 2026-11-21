# `cap:cbor_diag` — RFC 8949 CBOR 确定性编解码与诊断表示法

## 1. 概述

`cap:cbor_diag`（动态挂载为 `cap.cbor_diag`）是纯 CP 实现的工业级 RFC 8949 CBOR (Concise Binary Object Representation) 编解码与诊断表示法（Diagnostic Notation, RFC 8949 Section 8）标准库。支持全部 8 种主类型（正整数、负整数、字节串、UTF-8 文本串、数组、Map、Tag、Simple/Bool/Null/Undefined/Float）、定长与不定长编码（`0x5f`/`0x7f`/`0x9f`/`0xbf` + `0xff` break）、最大嵌套深度防护（`MAX_DEPTH = 64`）、Section 4.2 确定性规范编码（`encodeCanonical`）及诊断文本格式化（`toDiag`）。

## 2. 核心类型

- `MAX_DEPTH: usize = 64`：解码最大嵌套深度限制，防止栈溢出。
- `CborError`：`UnexpectedEof`、`InvalidAdditionalInfo`、`IndefiniteLengthNotAllowed`、`UnexpectedBreak`、`NestingTooDeep`、`InvalidUtf8`、`DuplicateMapKey`、`OutOfMemory`、`TrailingBytes`。
- `CborEntry`：`{ key: CborValue, value: CborValue }`。
- `CborTag`：`{ tag: u64, value: CborValue }`。
- `CborValue`（tagged union）：
  - `Unsigned: u64`、`Negative: i64`
  - `Bytes: []const u8`、`Text: []const u8`
  - `Array: []CborValue`、`Map: []CborEntry`、`Tag: *CborTag`
  - `Bool: bool`、`Null`、`Undefined`、`Simple: u8`、`Float: f64`
  - 方法：`deinit(*self, alloc)`、`eql(self, other) bool`。

## 3. API 速查

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `decode` | `fn decode(alloc: std.mem.Allocator, bytes: []const u8) !CborValue` | 解码 CBOR 二进制（支持定长/不定长，校验 UTF-8 与深度） |
| `encode` | `fn encode(alloc: std.mem.Allocator, val: CborValue) ![]u8` | 标准 CBOR 编码（最短整数表示） |
| `encodeCanonical` | `fn encodeCanonical(alloc: std.mem.Allocator, val: CborValue) ![]u8` | RFC 8949 Section 4.2 确定性编码（最短浮点 + Map 键按字节长度与字典序排序 + 重复键检查） |
| `toDiag` | `fn toDiag(alloc: std.mem.Allocator, val: CborValue) ![]u8` | 生成 RFC 8949 Section 8 诊断表示法字符串 |

## 4. 示例

```cp
import std from "std";
import { CborValue, decode, encodeCanonical, toDiag } from "cap:cbor_diag";

fn example(alloc: std.mem.Allocator) !void {
    const raw = &[_]u8{ 0xa2, 0x01, 0x02, 0x61, 'a', 0x82, 0x03, 0xf5 };
    let val = try decode(alloc, raw);
    defer val.deinit(alloc);

    const diag = try toDiag(alloc, val);
    defer alloc.free(diag);
    // diag => "{1: 2, \"a\": [3, true]}"

    const canonical = try encodeCanonical(alloc, val);
    defer alloc.free(canonical);
}
```
