[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/libyuv

Google libyuv high-performance YUV/RGB pixel conversion, scaling, and rotation for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/libyuv` |
| 版本 | `1.9.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/libyuv
```

```cp
import {
    LibyuvError,
    I420ToARGB,
    ARGBToI420,
    ARGBScale,
    I420Rotate,
    absI32,
    i420ToArgb,
    argbToI420,
} from "@cap/libyuv";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `LibyuvError` | `InvalidDimensions, BufferTooSmall, InvalidRotation, ConvertFailed, ScaleFailed, RotateFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `I420ToARGB` | `(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_argb: [*]u8!, dst_stride_argb: i32, width: i32, height: i32): i32` | 调用 `I420ToARGB` 并返回 `i32`。 |
| `ARGBToI420` | `(src_argb: [*]&u8!, src_stride_argb: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32): i32` | 调用 `ARGBToI420` 并返回 `i32`。 |
| `ARGBScale` | `(src_argb: [*]&u8!, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: [*]u8!, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): i32` | 调用 `ARGBScale` 并返回 `i32`。 |
| `I420Rotate` | `(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32, mode: i32): i32` | 调用 `I420Rotate` 并返回 `i32`。 |
| `absI32` | `(v: i32): i32` | 调用 `absI32` 并返回 `i32`。 |
| `i420ToArgb` | `(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_argb: []u8, dst_stride_argb: i32, width: i32, height: i32): !void` | 调用 `i420ToArgb` 并返回 `!void`。 |
| `argbToI420` | `(src_argb: &[]u8, src_stride_argb: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32): !void` | 调用 `argbToI420` 并返回 `!void`。 |
| `argbScale` | `(src_argb: &[]u8, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: []u8, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): !void` | 调用 `argbScale` 并返回 `!void`。 |
| `i420Rotate` | `(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32, mode: i32): !void` | 调用 `i420Rotate` 并返回 `!void`。 |

## 最小调用示例

```cp
import { absI32 } from "@cap/libyuv";

pub fn main() {
    let result = absI32(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
