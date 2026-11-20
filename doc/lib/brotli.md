# `cap:brotli` — 纯 CP RFC 7932 Brotli Meta-Block 编解码库

> **模块路径**: `lib/brotli/`  
> **导入协议**: `import { BrotliError, BrotliHeader, parseHeader, windowSizeFromBits, compress, compressUncompressed, decompress, decompressWithLimits } from "cap:brotli";`  
> **动态挂载**: `import cap from "cap";` -> `cap.brotli.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:brotli` 是 100% 纯 CP 实现、零外部 C 依赖（无 `std.posix` / `@import("bun")` / `libbrotli`）的 RFC 7932 Brotli 流解析与编解码库：

1. **RFC 7932 §9.1 流头与窗口编码 (`parseHeader`, `windowSizeFromBits`)**：
   - 完整支持可变位长 `WBITS`（10..24），对应窗口大小 `(1 << window_bits) - 16`（1008 B ~ 16,777,200 B）。
   - 严格拦截非法保留编码（如 `WBITS = 9` 返回 `BrotliError.InvalidWindowBits`）。
2. **RFC 7932 Uncompressed Meta-Block 编码 (`compress`, `compressUncompressed`)**：
   - 空输入编码为规范最小 1 字节 `ISLAST=1, ISLASTEMPTY=1` 帧（`window_bits=16` 时为 `0x06`）。
   - 非空输入按 `MAX_META_BLOCK_LEN`（16 MiB）自动分块编码为 `ISUNCOMPRESSED=1` 字节对齐原始块，并以 `ISLAST=1, ISLASTEMPTY=1` 结束。
3. **RFC 7932 全功能解码与防炸弹安全边界 (`decompress`, `decompressWithLimits`)**：
   - 完整解析 `ISLAST`、`ISLASTEMPTY`、`MNIBBLES`（4/5/6 nibbles）、`MNIBBLES=3` Metadata 跳过块（`MSKIPBITS` / `MSKIPBYTES`）及 `ISUNCOMPRESSED=1` 原始块。
   - 完整实现 RFC 7932 Compressed Meta-Block 解码：Simple / Complex Canonical Huffman 前缀码树、Block Type/Length 切换、Context Mode 与 Context Map（含 RLE 与 Inverse MTF）、Insert-and-Copy 704 符号表、Distance Ring Buffer 与 LZ77 重叠拷贝。
   - 严格防御截断输入（`UnexpectedEof`）、超大窗口（`WindowTooLarge`）及超大输出（`OutputTooLarge`）。

---

## 二、 核心 API 速查表 (`lib/brotli/brotli.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `BrotliError` | `UnexpectedEof`, `InvalidWindowBits`, `InvalidMetaBlock`, `InvalidHuffmanCode`, `InvalidDistance`, `OutputTooLarge`, `WindowTooLarge`, `OutOfMemory` |
| `BrotliHeader` | `{ window_bits: u8, window_size: usize }` |
| `parseHeader(compressed)` | 解析 RFC 7932 流头 `WBITS` 与 `window_size` |
| `windowSizeFromBits(window_bits)` | 计算 `(1 << window_bits) - 16`，越界返回 `InvalidWindowBits` |
| `compress(allocator, input)` | 使用默认 `window_bits = 16` 编码为 RFC 7932 Brotli 流（调用方负责 `allocator.free`） |
| `compressUncompressed(allocator, input, window_bits)` | 指定 `window_bits`（10..24）编码为 RFC 7932 Uncompressed Meta-Block 流 |
| `decompress(allocator, compressed)` | 使用默认安全上限（16 MiB 窗口 / 64 MiB 输出）解压 Brotli 流 |
| `decompressWithLimits(allocator, compressed, max_window_bytes, max_output_bytes)` | 带窗口与输出字节上限的安全解压，超限返回 `WindowTooLarge` / `OutputTooLarge` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    BrotliHeader,
    compress,
    compressUncompressed,
    decompress,
    decompressWithLimits,
} from "cap:brotli";

fn demo(alloc: std.mem.Allocator): !void {
    const raw = "hello brotli! hello brotli!";

    const encoded = compress(alloc, raw)?;
    defer alloc.free(encoded);

    const decoded = decompress(alloc, encoded)?;
    defer alloc.free(decoded);

    const custom = compressUncompressed(alloc, raw, 12)?;
    defer alloc.free(custom);

    const bounded = decompressWithLimits(alloc, custom, 8192, 4096)?;
    defer alloc.free(bounded);
}
```
