[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# color_quant

Zero-allocation pure CP 256-color palette quantizer and nearest-color mapper (image-rs color_quant parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:color_quant` ([https://github.com/image-rs/color_quant](https://github.com/image-rs/color_quant))

| 元信息 | 值 |
| --- | --- |
| 包名 | `color_quant` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add color_quant
```

```cp
import {
    Rgb8,
    ColorPalette,
    clqtEmptyPalette,
    clqtBucketIndex,
    buildUniformPalette,
    quantizeHistogram,
    mapPixelsToPalette,
} from "color_quant";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rgb8` | `class` | 字段: `r: u8, g: u8, b: u8` |
| `ColorPalette` | `class` | 字段: `colors: [256]Rgb8, len: usize` · 方法: `nearestIndex` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `clqtEmptyPalette` | `(): ColorPalette` | 调用 `clqtEmptyPalette` 并返回 `ColorPalette`。 |
| `clqtBucketIndex` | `(px: Rgb8): usize` | 调用 `clqtBucketIndex` 并返回 `usize`。 |
| `buildUniformPalette` | `(): ColorPalette` | 调用 `buildUniformPalette` 并返回 `ColorPalette`。 |
| `quantizeHistogram` | `(pixels: &[]Rgb8, max_colors: usize): ColorPalette` | 调用 `quantizeHistogram` 并返回 `ColorPalette`。 |
| `mapPixelsToPalette` | `(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize` | 调用 `mapPixelsToPalette` 并返回 `usize`。 |

## 最小调用示例

```cp
import { clqtEmptyPalette } from "color_quant";

pub fn main() {
    let result = clqtEmptyPalette();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
