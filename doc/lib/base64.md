# `cap:base64` — Pure CP Base64 / Base64URL Standard Library

## 1. 概述
`cap:base64`（以及 `import cap from "cap"; cap.base64`）提供零 C 依赖、O(1) 查表加速的工业级 RFC 4648 Base64 与 Base64URL 编解码能力。支持 4 种标准变体、零分配切片编解码、Allocator 便捷分配包装、MIME 空白忽略解码，以及任意 `1..N` 字节边界的流式增量编解码，并严格执行 RFC 4648 §3.5 非规范填充残余比特校验。

## 2. 核心类型
- `Base64Variant`：
  - `.Standard`：标准字母表（`+` `/`）+ `=` 填充
  - `.StandardNoPad`：标准字母表（`+` `/`）无填充
  - `.UrlSafe`：URL 安全字母表（`-` `_`）+ `=` 填充
  - `.UrlSafeNoPad`：URL 安全字母表（`-` `_`）无填充
- `Base64Error`：`InvalidCharacter` | `InvalidLength` | `InvalidPadding` | `NonCanonicalPaddingBits` | `DestinationTooSmall` | `OutOfMemory`
- `StreamingBase64Encoder`：增量流式编码器（`init` / `update` / `finish` / `updateToList` / `finishToList`）
- `StreamingBase64Decoder`：增量流式解码器（`init` / `initIgnoreWhitespace` / `update` / `finish` / `updateToList` / `finishToList`）

## 3. API 速查
| 函数 / 方法 | 签名摘要 | 说明 |
| :--- | :--- | :--- |
| `encodedLen` | `(input_len: usize, variant: Base64Variant): usize` | 计算编码后精确字节数 |
| `maxDecodedLen` | `(encoded_len: usize): usize` | 计算解码缓冲区上界 |
| `exactDecodedLen` | `(encoded: []const u8, variant: Base64Variant): Base64Error!usize` | 校验长度/填充并计算精确解码长度 |
| `encodeToSlice` | `(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8` | 零分配编码到目标切片 |
| `decodeToSlice` | `(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8` | 零分配严格解码到目标切片 |
| `decodeIgnoreWhitespaceToSlice` | `(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8` | 忽略 ` ` / `\r` / `\n` / `\t` 的 MIME 解码 |
| `encodeAlloc` | `(alloc: Allocator, src: []const u8, variant: Base64Variant): Base64Error![]u8` | 使用分配器编码并返回拥有所有权的切片 |
| `decodeAlloc` | `(alloc: Allocator, src: []const u8, variant: Base64Variant): Base64Error![]u8` | 使用分配器解码并返回拥有所有权的切片 |

## 4. 示例
```cp
import { Base64Variant, encodeAlloc, decodeAlloc } from "cap:base64";
const std = @import("std");

let enc = encodeAlloc(std.heap.page_allocator, "foobar", .UrlSafeNoPad)?;
defer std.heap.page_allocator.free(enc);
let dec = decodeAlloc(std.heap.page_allocator, enc, .UrlSafeNoPad)?;
defer std.heap.page_allocator.free(dec);
```
