# CP 标准库：纯 CP RFC 8878 Zstandard 压缩与帧解析引擎 (`cap:zstd`)

## 一、 架构总览与设计准则

`cap:zstd` 是 CP 语言官方内置的高性能、零 C 依赖纯 CP Zstandard 标准库，严格遵循 **RFC 8878 (Zstandard Compressed Data Format)**，完整支持 **XXH64 内容校验和**、**帧头元数据探测**、**Skippable Frame 透明跳过**、**Raw/RLE/Compressed 块编解码**与 **多帧串联解压**。

**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:zstd"`，也支持 `import cap from "cap"; cap.zstd.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                 CP 纯正 Zstandard 引擎 (cap:zstd / RFC 8878)              │
├────────────────────────┬─────────────────────────┬───────────────────────┤
│ XXH64 校验引擎         │ 帧头探测与 Skippable    │ 块级编解码与安全限流  │
│ xxhash64(data, seed)   │ inspectFrameHeader()    │ Raw(0) / RLE(1)       │
│ 低 32 位内容校验和     │ 0xFD2FB528 标准魔数     │ Compressed(2) FSE/Huff│
│ 防篡改 ChecksumMismatch│ 0x184D2A50..5F 跳过     │ Window/Output 防膨胀  │
└────────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/zstd/zstd.cp` | `// Source: cp/lib/zstd/zstd.cp (...)` | `ZstdError`、`ZstdFrameInfo`、`xxhash64`、`inspectFrameHeader`、`compress`、`decompress`、`decompressWithLimits` |
| `lib/zstd/index.cp` | `// Source: cp/lib/zstd/index.cp (...)` | `cap:zstd` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`ZstdError`)与常量

- **`ZSTD_MAGIC: u32 = 0xFD2FB528`**：RFC 8878 标准 Zstandard 帧魔数（小端）。
- **`ZSTD_SKIPPABLE_MAGIC_BASE: u32 = 0x184D2A50`**：Skippable Frame 魔数基址（`0x184D2A50..0x184D2A5F`）。
- `ZstdError`：
  - `InvalidMagic`：帧魔数既不是 `ZSTD_MAGIC` 也不是 Skippable Frame
  - `CorruptedFrame`：帧头保留位非零、块长度超限（> 128 KB）、FSE/Huffman 码流损坏或帧内容长度不匹配
  - `ChecksumMismatch`：帧尾 4 字节 XXH64 低 32 位校验和与解压输出不匹配
  - `UnsupportedBlockType`：遇到 RFC 8878 `Reserved` 块类型（`block_type == 3`）
  - `WindowTooLarge`：帧声明的 `window_size` 超出 `max_window_bytes` 安全上限
  - `OutputTooLarge`：解压输出超出 `max_output_bytes` 安全上限（防解压膨胀）
  - `TruncatedInput`：输入字节流意外截断
  - `OutOfMemory`：内存分配失败

### 2. XXH64 哈希 (`xxhash64`)

- **`xxhash64(data: []const u8, seed: u64): u64`**
  - 标准 64 位 xxHash 算法实现，用于计算 Zstandard 帧尾 32 位内容校验和（`@truncate` / `xxhash64(decoded, 0) & 0xFFFFFFFF`）。

### 3. 帧头元数据探测 (`inspectFrameHeader`)

- **`ZstdFrameInfo`**：
  - `frame_content_size: u64?`：帧头声明的原始未压缩大小（若未设置则为 `null`）
  - `window_size: u64`：解码所需滑动窗口大小
  - `has_checksum: bool`：是否包含帧尾 4 字节 XXH64 校验和
  - `dictionary_id: u32?`：可选字典 ID
- **`inspectFrameHeader(bytes: []const u8): ZstdFrameInfo ! ZstdError`**
  - 自动跳过前置 Skippable Frame，解析第一个 Zstandard 帧头元数据。

### 4. 压缩与解压 (`compress`, `decompress`, `decompressWithLimits`)

- **`compress(allocator: std.mem.Allocator, input: []const u8, write_checksum: bool): []u8 ! ZstdError`**
  - 生成标准 RFC 8878 帧头（含 `Single_Segment_Flag` 与 `Frame_Content_Size`）；
  - 自动识别高重复单字节序列并编码为 `RLE_Block`（`type = 1`，极限压缩比），非重复数据编码为 `Raw_Block`（`type = 0`，单块最大 128 KB）；
  - 当 `write_checksum == true` 时在帧尾附加 4 字节小端 `xxhash64(input, 0)` 低 32 位校验和。
- **`decompressWithLimits(allocator: std.mem.Allocator, bytes: []const u8, max_window_bytes: u64, max_output_bytes: usize): []u8 ! ZstdError`**
  - 支持 Skippable Frame 透明跳过与多帧串联解压；
  - 完整支持 `Raw_Block (0)`、`RLE_Block (1)` 与 `Compressed_Block (2)`（含 Raw/RLE/Huffman 字面量与 Predefined/RLE/FSE/Repeat 序列解压）；
  - 严格拦截 `Reserved (3)` 块、`WindowTooLarge`、`OutputTooLarge`、`ChecksumMismatch` 与 `TruncatedInput`。
- **`decompress(allocator: std.mem.Allocator, bytes: []const u8): []u8 ! ZstdError`**
  - 使用默认安全限流（8 MiB 窗口、64 MiB 输出上限）调用 `decompressWithLimits`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { compress, decompress, inspectFrameHeader, xxhash64 } from "cap:zstd";
import cap from "cap";

pub fn demoZstd()! {
    const alloc = std.heap.page_allocator;
    const payload = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    const compressed = compress(alloc, payload, true)?;
    defer alloc.free(compressed);

    const info = inspectFrameHeader(compressed)?;
    // info.has_checksum == true, info.frame_content_size == 64

    const decoded = decompress(alloc, compressed)?;
    defer alloc.free(decoded);

    // 也可通过动态挂载调用
    const h = cap.zstd.xxhash64(decoded, 0);
    _ = h;
}
```
