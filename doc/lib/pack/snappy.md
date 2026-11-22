# `cap:snappy` — Snappy Block Format 高速无损压缩与解压库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:pack`，导入请使用 `import { ... } from "cap:pack"`。

> **模块路径**: `lib/snappy/`  
> **导入协议**: `import { SnappyError, compress, decompress, decompressWithMaxSize, uncompressedLength } from "cap:pack";`  
> **动态挂载**: `import cap from "cap";` -> `cap.snappy.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:snappy` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的 Snappy Block Format 高速无损压缩与解压缩库：

1. **压缩编码 (`compress`)**：
   - 严格遵循 Snappy Block Format：头部写入未压缩长度 LEB128 Varint，随后按 64KB 分块使用 14-bit 哈希表（`0x1e35a7bd` 乘法散列，4 字节前缀匹配）与启发式跳过策略进行压缩。
   - 自动选择最优元素编码：
     - **Literal (`tag & 0x03 == 0`)**：短字面量（`len <= 60`）单字节头，长字面量支持 1/2/3/4 字节小端扩展长度。
     - **Copy-1 (`0x01`)**：11-bit 偏移（`1..2047`）+ 长度 `4..11`（仅占 2 字节）。
     - **Copy-2 (`0x02`)**：16-bit 偏移（`1..65535`）+ 长度 `1..64`（占 3 字节）。
2. **安全解压 (`decompress` / `decompressWithMaxSize` / `uncompressedLength`)**：
   - `uncompressedLength` 仅解析头部 Varint，不分配任何解压缓冲区，适合预检。
   - 完整支持 Literal（0..4 字节扩展长度）、Copy-1、Copy-2 与 Copy-4（`0x03`，32-bit 偏移）。
   - 严格支持重叠拷贝（`offset < copy_len`，例如 `offset == 1` 的 RLE 游程填充）。
   - 严格边界与资源耗尽防护：拒绝 `offset == 0` 或 `offset > current_out_pos`（`InvalidOffset`）、拒绝超限输出声明（`OutputTooLarge`）、拒绝截断流（`TruncatedInput`）与长度不匹配或溢出流（`CorruptedData`）。

---

## 二、 核心 API 速查表 (`lib/snappy/snappy.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `SnappyError` | `InvalidVarint`, `CorruptedData`, `TruncatedInput`, `InvalidOffset`, `OutputTooLarge`, `OutOfMemory` |
| `compress(allocator, input)` | 将 `input` 压缩为 Snappy Block Format 字节切片（调用方负责 `allocator.free`） |
| `uncompressedLength(compressed)` | 仅解析头部 Varint 返回原始字节长度，不分配内存 |
| `decompress(allocator, compressed)` | 解压 Snappy 数据块并返回原始字节切片（调用方负责 `allocator.free`） |
| `decompressWithMaxSize(allocator, compressed, max_bytes)` | 带输出上限 `max_bytes` 的安全解压，超限直接返回 `SnappyError.OutputTooLarge` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { compress, decompress, decompressWithMaxSize, uncompressedLength } from "cap:pack";

fn demo(alloc: std.mem.Allocator): !void {
    const raw = "hello snappy! hello snappy! hello snappy!";
    const compressed = compress(alloc, raw)?;
    defer alloc.free(compressed);

    const declared_len = uncompressedLength(compressed)?;
    // 先校验长度再解压
    const restored = decompressWithMaxSize(alloc, compressed, declared_len)?;
    defer alloc.free(restored);
}
```
