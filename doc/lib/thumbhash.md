# `cap:thumbhash` — ThumbHash 超紧凑图片占位符编解码器

> 参考来源：/data/dev/bun/src/runtime/image/thumbhash.cp | 吸收日期：2026-11-22

## 概述

`cap:thumbhash` 吸收自 Bun 图像处理子系统（`src/runtime/image/thumbhash.cp`，实现 Evan Wallace 的 public domain ThumbHash LQIP 算法），纯 CP 零外部依赖实现约 21–25 字节低频 DCT 图片占位符编解码、零分配宽高比提取（`approxAspectRatio`）、主色提取（`averageRGBA`），以及带 `~init()` 确定性 RAII 释放的 `DecodedImage`。

## 上游算法特性与行为契约

- **极紧凑低频 DCT 编码（≤25 字节）**：
  - 将尺寸 $\le 100 \times 100$ 的 RGBA 缩略图转换到 **LPQA** 色彩空间（亮度 $L$、两个对立色度平面 $P, Q$ 与可选透明度 $A$），对其低阶离散余弦变换（DCT）系数进行 4-bit 量化打包；
  - 最大哈希长度 `MAX_LEN = 25` 字节：5 字节基础头 + 可选 1 字节 Alpha 头 + 4-bit 压缩 AC 系数（最坏情形为带透明度的正方形图像：$5 + 1 + \lceil(14 + 5 + 5 + 14)/2\rceil = 25$）。
- **防透明黑边伪影**：编码阶段先计算 **Alpha 加权平均色**，再将半透明/全透明像素预混合到该平均色上后执行 DCT，避免透明边缘在低通滤波后产生难看的黑色光晕。
- **高效解码与零分配元信息提取**：
  - `decode(allocator, hash)` 输出宽/高不超过 32px 的平滑模糊占位 RGBA 图像，准确还原平均色、宽高比与大致构图；
  - `approxAspectRatio(hash)` 与 `averageRGBA(hash)` 仅解析哈希前 5–6 字节头部，**零堆分配、零 DCT 运算**，适合列表渲染预占位与背景色填充。
- **性能特征**：上游实测单次编解码在标量 `f32` 下最多仅需 $\le 100^2 \cdot 7^2$ 次乘加，耗时微秒级且无需 SIMD 依赖。

## 导出符号与 API

### 常量与错误集

- `MAX_LEN: usize = 25` — ThumbHash 最大编码字节数。
- `ThumbhashError` — 编码/解码参数非法、输入截断或内存分配失败错误集。

### `DecodedImage`（含 `~init()` RAII）

- 字段：`rgba: []u8`、`w: u32`、`h: u32`
- 析构：`~init()` 确定性释放 `rgba` 像素缓冲。

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `encode(out: *[MAX_LEN]u8, w: u32, h: u32, rgba: []const u8): ([]u8) ! ThumbhashError` | 将 `1..100` 尺寸的 RGBA 像素编码写入栈上 `out`，返回有效切片 |
| `decode(allocator: std.mem.Allocator, hash: []const u8): DecodedImage ! ThumbhashError` | 将 ThumbHash 解码为 $\le 32\text{px}$ 的 RGBA 占位图 |
| `approxAspectRatio(hash: []const u8): f32 ! ThumbhashError` | 零堆分配提取近似宽高比 `width / height` |
| `averageRGBA(hash: []const u8): [4]u8 ! ThumbhashError` | 零堆分配提取图像主色调 RGBA |

## 边界与安全注意事项

1. **输入尺寸限制**：`encode` 要求 `1 <= w <= 100`、`1 <= h <= 100` 且 `rgba.len == w * h * 4`；大图调用前应先缩放到 $\le 100\text{px}$ 边长。
2. **畸形哈希防护**：`decode`、`approxAspectRatio` 与 `averageRGBA` 均校验最小头长度（`< 5` 字节，或 `has_alpha` 时 `< 6` 字节）与 AC 载荷长度，截断输入安全返回错误而绝不越界读取。

## 使用示例

```cp
import std from "std";
import { MAX_LEN, encode, decode, approxAspectRatio, averageRGBA } from "cap:thumbhash";

fn thumbhashDemo(alloc: std.mem.Allocator) {
    let pixels: [4 * 4 * 4]u8 = [120; 64]; // 4x4 RGBA
    let out: [MAX_LEN]u8 = undefined;
    const hash = encode(&out, 4, 4, pixels[0..]) catch return;

    const ratio = approxAspectRatio(hash) catch return;
    const avg = averageRGBA(hash) catch return;
    const img = decode(alloc, hash) catch return;
    * = ratio;
    * = avg;
    * = img;
}
```
