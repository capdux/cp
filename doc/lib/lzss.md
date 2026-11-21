# `cap:lzss` — 零堆分配 LZSS 滑动窗口压缩与解压库

> **模块路径**: `lib/lzss/`  
> **导入协议**: `import { lzssCompress, lzssDecompress, lzssMaxCompressedSize, lzssCompressFramed, lzssDecompressFramed } from "cap:lzss";`  
> **动态挂载**: `import cap from "cap";` -> `cap.lzss.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:lzss` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的经典 LZSS 滑动窗口编解码库：

1. **经典 12-bit/4KB 滑动窗口 & Flag Byte 分组**：
   - 每组 1 字节标志位（LSB-first）控制后续最多 8 项：`bit=1` 为 1B 字面量，`bit=0` 为 2B 匹配对（低 12 位 `dist - 1` 覆盖 `1..4096`，高 4 位 `len - 3` 覆盖 `3..18`）。
2. **重叠 RLE 展开**：
   - 压缩端与解压端完整支持 `dist < len` 重叠距离匹配，对高度重复数据可实现高效 RLE 压缩与逐字节安全展开。
3. **带 4B 长度头的 Framed 模式与严格边界防护**：
   - `lzssCompressFramed` / `lzssDecompressFramed` 前置 4B 小端原始长度，支持预检缓冲容量与精确长度核验；
   - 解压端严格拒收 `dist > out_pos`、截断 2B 匹配令牌、孤立 Flag 字节及目标缓冲溢出。

---

## 二、 核心 API 速查表 (`lib/lzss/lzss.cp`)

| 符号 / 函数 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `LZSS_WINDOW_SIZE (4096)`, `LZSS_MIN_MATCH (3)`, `LZSS_MAX_MATCH (18)`, `LZSS_FRAME_HEADER_SIZE (4)` |
| `lzssMaxCompressedSize` | `lzssMaxCompressedSize(src_len: usize): usize` — 最坏情况输出字节数 `src_len + (src_len + 7) / 8` |
| `lzssCompress` | `lzssCompress(src: []const u8, dst: []u8): usize?` — 零堆分配 LZSS 压缩 |
| `lzssDecompress` | `lzssDecompress(src: []const u8, dst: []u8): usize?` — 零堆分配 LZSS 解压 |
| `lzssCompressFramed` | `lzssCompressFramed(src: []const u8, dst: []u8): usize?` — 带 4B 小端原始长度头的压缩 |
| `lzssDecompressFramed` | `lzssDecompressFramed(src: []const u8, dst: []u8): usize?` — 校验 4B 长度头的解压 |

---

## 三、 使用示例

```typescript
import { lzssCompressFramed, lzssDecompressFramed } from "cap:lzss";

fn demo() {
    const input = "hello world hello world hello world!";
    let comp: [128]u8 = [0; 128];
    let decomp: [128]u8 = [0; 128];
    if (c_len := lzssCompressFramed(input, comp[0..])) {
        if (d_len := lzssDecompressFramed(comp[0..c_len], decomp[0..])) {
            * = d_len;
        }
    }
}
```
