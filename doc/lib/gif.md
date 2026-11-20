# CP 标准库：GIF87a / GIF89a 动图编解码器 (`cap:gif` / `cap.gif`)

## 一、 架构总览与设计准则

`cap:gif` 是 CP 语言官方内置的纯原生 GIF87a / GIF89a 静态图与多帧动图编解码标准库，直接复用 `cap:lzw`（`LzwBitOrder.LsbFirst`）完成 GIF 子块 LZW 压缩与解压。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，仅使用纯 CP 与 `cap:lzw` / `std` 基础库，支持 `import ... from "cap:gif"` 与 `import cap from "cap"; cap.gif.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/gif/gif.cp` | `// Source: cp/lib/gif/gif.cp (Pure CP GIF87a / GIF89a Animated Image Codec via cap:lzw)` | `GifError`、`GifColor`、`DisposalMethod`、`GifFrame`、`GifIndexedFrame`、`GifImage`、`encodeIndexed`、`decode`、`decodeWithLimits` |
| `lib/gif/index.cp` | `// Source: cp/lib/gif/index.cp (Public Unified Export for cap:gif & cap.gif)` | `cap:gif` 与 `cap.gif` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型

- **`GifError`**：
  - `InvalidSignature`：缺少 `"GIF87a"` 或 `"GIF89a"` 6 字节签名
  - `InvalidHeader`：逻辑屏幕/帧尺寸为 `0`、帧越界或 LZW 最小码长非法
  - `TruncatedData`：文件头、调色板、LZW 子块流截断或缺少 `0x3B` 结束符
  - `UnsupportedBlock`：遇到未知或损坏的块标识
  - `InvalidColorTable`：缺少有效调色板、调色板长度超限（`> 256`）或像素索引越界
  - `ImageTooLarge`：图像宽高超过 `max_w` / `max_h` 上限或解压像素溢出
  - `TooManyFrames`：帧数超过 `max_frames` 限制
  - `OutOfMemory`：分配器内存不足

- **`GifColor`**：`{ r: u8, g: u8, b: u8 }`
- **`DisposalMethod` (`u8`)**：`Unspecified = 0`、`Keep = 1`、`RestoreBg = 2`、`RestorePrev = 3`

### 2. 帧与图像容器

- **`GifFrame`**：
  - 字段：`left: u16`、`top: u16`、`width: u16`、`height: u16`、`delay_cs: u16`、`transparent_index: u8?`、`disposal: DisposalMethod`、`palette: ([]const GifColor)?`、`pixels_rgba: []u8`、`indices: []u8`
  - 解码时自动将调色板与透明色索引展开为标准 `width * height * 4` RGBA8888 缓冲，同时保留 `indices` 以便无损重编码。
  - `deinit(*_, allocator: std.mem.Allocator)`：释放 `pixels_rgba`、`indices` 与可选局部调色板 `palette`。
- **`GifIndexedFrame`**：`encodeIndexed` 的输入帧描述（支持 `local_palette`、`delay_cs`、`transparent_index`、`disposal`）。
- **`GifImage`**：
  - 字段：`width: u16`、`height: u16`、`loop_count: u16`、`global_palette: []const GifColor`、`frames: []GifFrame`
  - `deinit(*_, allocator: std.mem.Allocator)`：释放所有帧及全局调色板。

### 3. 编码与解码函数

- **`encodeIndexed(allocator, width, height, loop_count, global_palette, indexed_frames): ([]u8)!GifError`**
  - 写入 `"GIF89a"` 逻辑屏幕描述符、Netscape `2.0` 循环扩展块（动图或 `loop_count > 0`）、`Graphic Control Extension`、`Image Descriptor`、最大 255 字节 LZW 子块流及 `0x3B` 结束符。
- **`decode(allocator, gif_bytes): GifImage!GifError`**
  - 使用默认安全上限（`65535 x 65535`、`4096` 帧）解码 GIF87a/GIF89a。
- **`decodeWithLimits(allocator, gif_bytes, max_w, max_h, max_frames): GifImage!GifError`**
  - 支持顺序扫描与 `Interlaced`（4-pass 隔行扫描）、局部调色板覆盖及未知扩展块安全跳过，严格拦截超限尺寸与过多帧。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { GifColor, GifIndexedFrame, DisposalMethod, encodeIndexed, decode } from "cap:gif";

pub fn roundtripExample(alloc: std.mem.Allocator): !void {
    const palette: [2]GifColor = [
        GifColor{ r: 255, g: 0, b: 0 },
        GifColor{ r: 0, g: 255, b: 0 },
    ];
    const indices: [4]u8 = [0, 1, 1, 0];
    const frames: [1]GifIndexedFrame = [
        GifIndexedFrame{
            width: 2,
            height: 2,
            delay_cs: 10,
            disposal: DisposalMethod.Keep,
            indices: &indices,
        },
    ];
    const encoded = encodeIndexed(alloc, 2, 2, 0, &palette, &frames)?;
    defer alloc.free(encoded);

    let img = decode(alloc, encoded)?;
    defer img.deinit(alloc);
}
```
