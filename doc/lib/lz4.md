# CP 标准库：纯 CP 极速 LZ4 块与帧压缩引擎 (`cap:lz4`)

## 一、 架构总览与设计准则

`cap:lz4` 是 CP 语言官方内置的高性能、零 C 依赖纯 CP 数据压缩标准库，面向内存缓存、WAL 日志压缩（配合 `cap:kv`）及高速网络帧传输设计，完整实现 **LZ4 Block Format（原始块格式）**、**LZ4 Frame Format（标准帧格式，魔数 `0x184D2204`）** 与 **XXH32 校验和**。

**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:lz4"`，也支持 `import cap from "cap"; cap.lz4.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 纯正 LZ4 极速压缩引擎 (cap:lz4)                   │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ XXH32 校验引擎        │ Lz4Block 原始块编解码   │ Lz4Frame 标准帧编解码 │
│ xxhash32(data, seed)  │ 4096 槽位 4B 哈希表     │ 0x184D2204 魔数       │
│ Header HC 字节校验    │ 64KB 滑窗 + RLE 重叠    │ 64KB 分块 + 未压缩回退│
│ Content Checksum      │ max_output_size 防膨胀  │ EndMark + 完整校验    │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/lz4/lz4.cp` | `// Source: cp/lib/lz4/lz4.cp (...)` | `Lz4Error`、`xxhash32`、`Lz4Block`（块压缩/解压）与 `Lz4Frame`（帧压缩/解压） |
| `lib/lz4/index.cp` | `// Source: cp/lib/lz4/index.cp (...)` | `cap:lz4` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`Lz4Error`)

- `CorruptStream`：帧头部版本/保留位非法或块校验失败
- `InvalidMagic`：帧魔数不等于 `0x184D2204`
- `InvalidHeaderChecksum`：帧头部 `HC` 校验字节与 `((xxhash32(header_fields, 0) >> 8) & 0xFF)` 不匹配
- `InvalidContentChecksum`：帧尾部 4 字节 XXH32 内容校验和与解压载荷不匹配
- `OutputTooLarge`：解压输出超出 `max_output_size` 安全上限（防压缩炸弹/解压膨胀）
- `InvalidOffset`：块内匹配偏移为 `0` 或越界（`offset > out.len`）
- `TruncatedInput`：输入字节流意外截断
- `OutOfMemory`：内存分配失败

### 2. XXH32 哈希 (`xxhash32`)

- **`xxhash32(data: []const u8, seed: u32): u32`**：标准 32 位 xxHash 算法实现，用于 LZ4 Frame Header `HC` 校验字节与可选内容校验和。

### 3. LZ4 原始块格式 (`Lz4Block`)

- **`Lz4Block.compress(alloc: std.mem.Allocator, src: []const u8): []u8 ! Lz4Error`**
  - 使用 4096 槽位 4 字节哈希表在 64KB 滑动窗口内查找 >= 4 字节匹配；
  - 完整编码 token 高/低 4 位、255 扩展长度累加与末尾 5 字节字面量规则；
  - 支持空输入与 `< 13` 字节短输入。
- **`Lz4Block.decompress(alloc: std.mem.Allocator, compressed: []const u8, max_output_size: usize): []u8 ! Lz4Error`**
  - 严格校验 `offset == 0` / `offset > out.len`（`InvalidOffset`）、截断输入（`TruncatedInput`）及 `max_output_size` 上限（`OutputTooLarge`）；
  - 严格逐字节支持重叠 RLE 匹配复制（如 `offset = 1` 全零长序列）。

### 4. LZ4 标准帧格式 (`Lz4Frame`)

- **`Lz4Frame.compressFrame(alloc: std.mem.Allocator, src: []const u8, with_content_checksum: bool): []u8 ! Lz4Error`**
  - 写入标准 `0x184D2204` 魔数、`FLG`/`BD` 描述符及 XXH32 Header Checksum `HC`；
  - 按 `64KB` 分块压缩；对不可压缩块自动回退为最高位置 `1`（`0x80000000 | len`）的未压缩块；
  - 写入 `0x00000000` EndMark，并在 `with_content_checksum == true` 时附加 4 字节小端 `xxhash32(src, 0)`。
- **`Lz4Frame.decompressFrame(alloc: std.mem.Allocator, frame: []const u8, max_output_size: usize): []u8 ! Lz4Error`**
  - 严格校验魔数、头部 `HC` 校验字节、各分块边界与可选内容校验和；发现篡改立即返回对应 `Lz4Error`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Lz4Block, Lz4Frame, xxhash32 } from "cap:lz4";
import cap from "cap";

pub fn demoLz4()! {
    const alloc = std.heap.page_allocator;
    const payload = "CP-Agent WAL entry: key=alpha, val=0000000000000000";

    // 1. 原始块压缩/解压
    const block_comp = Lz4Block.compress(alloc, payload)?;
    defer alloc.free(block_comp);
    const block_dec = Lz4Block.decompress(alloc, block_comp, 4096)?;
    defer alloc.free(block_dec);

    // 2. 标准帧压缩/解压（带 XXH32 内容校验）
    const frame_comp = Lz4Frame.compressFrame(alloc, payload, true)?;
    defer alloc.free(frame_comp);
    const frame_dec = Lz4Frame.decompressFrame(alloc, frame_comp, 4096)?;
    defer alloc.free(frame_dec);

    // 3. 动态挂载调用
    const hash = cap.lz4.xxhash32(payload, 0);
    _ = hash;
}
```
