# `cap:lzw` — 纯 CP 可变位宽 LZW 字典编解码库 (GIF / TIFF / PDF)

> **模块路径**: `lib/lzw/`  
> **导入协议**: `import { LzwError, LzwBitOrder, LzwConfig, compress, decompress, decompressWithMaxSize } from "cap:lzw";`  
> **动态挂载**: `import cap from "cap";` -> `cap.lzw.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:lzw` 是 100% 纯 CP 实现、零外部 C 依赖（无 `std.posix` / `@import("bun")`）的可变位宽 LZW（Lempel-Ziv-Welch）字典无损压缩与解压库，完整兼容 GIF (`LsbFirst`) 与 TIFF / PDF (`MsbFirst`) 位序标准：

1. **位序双模式 (`LzwBitOrder`)**：
   - `LsbFirst`：低位优先位打包（GIF 图像压缩标准）。
   - `MsbFirst`：高位优先位打包（TIFF / PDF 压缩标准）。
2. **动态位宽与自动字典重置 (`LzwConfig`)**：
   - 支持 `2 <= min_code_size <= 8` 且 `min_code_size < max_code_size <= 12`（最高 12-bit = 4096 字典项）。
   - 自动分配控制码字：`clear_code = 1 << min_code_size`、`eoi_code = clear_code + 1`、`first_free = clear_code + 2`。
   - 位宽从 `min_code_size + 1` 起步，随字典增长自动升档至 `max_code_size`；字典达到 `1 << max_code_size`（4096）满表后自动发出 `clear_code` 重置字典并回退至初始位宽 `min_code_size + 1`。
3. **经典边界与安全防护**：
   - 完整支持解码期 `KwKwK` 连续重复序列边界（`code == next_code`）。
   - 严格拦截越界/未初始化码字（`InvalidCode`）、缺失结束符（`MissingEoiCode`）、截断位流（`TruncatedInput`）及解压防膨胀炸弹限制（`OutputTooLarge`）。

---

## 二、 核心 API 速查表 (`lib/lzw/lzw.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `LzwError` | `InvalidMinCodeSize`, `InvalidCode`, `MissingEoiCode`, `TruncatedInput`, `OutputTooLarge`, `OutOfMemory` |
| `LzwBitOrder` | `LsbFirst`（GIF 风格）, `MsbFirst`（TIFF/PDF 风格） |
| `LzwConfig` | `{ min_code_size: u8 = 8, bit_order: LzwBitOrder = .LsbFirst, max_code_size: u8 = 12 }` |
| `compress(allocator, input, config)` | 使用可变位宽 LZW 压缩字节切片，返回 `LzwError![]u8`（调用方负责 `allocator.free`） |
| `decompress(allocator, compressed, config)` | 解压 LZW 字节流，返回 `LzwError![]u8`（调用方负责 `allocator.free`） |
| `decompressWithMaxSize(allocator, compressed, config, max_bytes)` | 带输出上限 `max_bytes` 的安全解压，超限返回 `LzwError.OutputTooLarge` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    LzwBitOrder,
    LzwConfig,
    compress,
    decompress,
    decompressWithMaxSize,
} from "cap:lzw";

fn demo(alloc: std.mem.Allocator): !void {
    const gif_cfg = LzwConfig{
        min_code_size: 8,
        bit_order: LzwBitOrder.LsbFirst,
        max_code_size: 12,
    };

    const raw = "ABABABABABABABAB_LZW_COMPRESSION_TEST!";
    const comp = compress(alloc, raw, gif_cfg)?;
    defer alloc.free(comp);

    const restored = decompressWithMaxSize(alloc, comp, gif_cfg, 4096)?;
    defer alloc.free(restored);
}
```
