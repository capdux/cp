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

## 5. Wave 32 工业级安全加固与 16-Lane SIMD 加速（对标 `marshallpierce/rust-base64` MIT/Apache-2.0）

`lib/base64/codec.cp` 与 `lib/base64/index.cp` 已对照 `marshallpierce/rust-base64`（MIT/Apache-2.0）与 RFC 4648 完成严格规范化加固与纯 CP 16-lane `@Vector(16, u8)` SIMD 加速，保持 100% 向后兼容。

| 符号 / 改进 | 所在文件 | 说明 |
| :--- | :--- | :--- |
| `encodedLenChecked` / `decodedLenEstimate` / `encodeInto` / `decodeInto` | `lib/base64/codec.cp` | 无溢出编码长度计算与零堆编解码便捷别名 |
| `exactDecodedLen` 填充加固 | `lib/base64/codec.cp` | 拒绝前两位含 `=`（如 `"===="`、`"A==="`）或非法位置 `=` 的畸形填充 |
| `simdValidateBlock16` / `simdValidateBase64Alphabet` | `lib/base64/codec.cp` | 16-lane `@Vector(16, u8)` Standard & URL-Safe 字母表并行合法性预检 |
| `simdEncodeChunk12To16` / `simdDecodeChunk16To12` | `lib/base64/codec.cp` | 纯 16-lane SIMD 范围算术快路径（12B -> 16B 编码、16B -> 12B 解码，零查表），并保留 `encodeToSliceScalar` / `decodeToSliceScalar` 标量参考用于 0 差异对拍 |

- **权威回归与 Fuzz 套件**：`cap/test/testBase64RustRefSuite.cp` 覆盖 RFC 4648 §10 官方向量、全 4 种模式、`RequireCanonical` 尾比特拒绝（`AB==`, `ABC=`）、`len % 4 == 1` 与字母表混用拦截、0..257 字节全长度 SIMD vs 标量 0 差异对拍，以及 1,600 组确定性 PRNG 随机字节往返与畸形 Base64 Fuzz（0 panic / 0 OOB / 0 差异）。
