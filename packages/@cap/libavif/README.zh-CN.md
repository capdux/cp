[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/libavif

AOMediaCodec libavif (v1.2.1) AVIF/HEIF image container & pixel format library for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/libavif` |
| 版本 | `1.2.1` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/libavif
```

```cp
import {
    AvifError,
    AvifImage,
    cp_avif_version,
    cp_avif_pixel_format_to_string,
    cp_avif_image_create,
    cp_avif_image_destroy,
    cp_avif_image_width,
    cp_avif_image_height,
} from "@cap/libavif";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `AvifError` | `InvalidArgument, AllocFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AvifImage` | `class` | 字段: `raw: *auto!?` · 方法: `create, width, height, depth, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_avif_version` | `(): [*:0]&u8!?` | 调用 `cp_avif_version` 并返回 `[*:0]&u8!?`。 |
| `cp_avif_pixel_format_to_string` | `(fmt: i32): [*:0]&u8!?` | 调用 `cp_avif_pixel_format_to_string` 并返回 `[*:0]&u8!?`。 |
| `cp_avif_image_create` | `(width: u32, height: u32, depth: u32, yuv_format: i32): *auto!?` | 调用 `cp_avif_image_create` 并返回 `*auto!?`。 |
| `cp_avif_image_destroy` | `(img: *auto!?)` | 就地执行 `cp_avif_image_destroy` 操作。 |
| `cp_avif_image_width` | `(img: *auto!?): u32` | 调用 `cp_avif_image_width` 并返回 `u32`。 |
| `cp_avif_image_height` | `(img: *auto!?): u32` | 调用 `cp_avif_image_height` 并返回 `u32`。 |
| `cp_avif_image_depth` | `(img: *auto!?): u32` | 调用 `cp_avif_image_depth` 并返回 `u32`。 |
| `cp_avif_image_create_and_destroy` | `(width: u32, height: u32, depth: u32): i32` | 调用 `cp_avif_image_create_and_destroy` 并返回 `i32`。 |
| `version` | `(): string` | 调用 `version` 并返回 `string`。 |
| `pixelFormatName` | `(fmt: i32): string` | 调用 `pixelFormatName` 并返回 `string`。 |
| `imageCreateAndDestroy` | `(width: u32, height: u32, depth: u32): !void` | 调用 `imageCreateAndDestroy` 并返回 `!void`。 |

## 最小调用示例

```cp
import { version } from "@cap/libavif";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
