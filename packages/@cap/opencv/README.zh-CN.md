[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/opencv

OpenCV (v4.10.0) core + imgproc computer vision library for CP via prebuilt dynamic libraries

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`opencv`](https://github.com/opencv/opencv)（`Apache-2.0`）（包名 `@cap/opencv`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/opencv` |
| 版本 | `4.10.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/opencv
```

```cp
import {
    OpenCvError,
    cp_cv_version,
    cp_cv_gaussian_blur_u8,
    cp_cv_canny_u8,
    cp_cv_resize_u8,
    version,
    gaussianBlurU8,
    cannyU8,
} from "@cap/opencv";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `OpenCvError` | `InvalidDimensions, InvalidKernelSize, InvalidThreshold, BufferTooSmall, OperationFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_cv_version` | `(): [*:0]&u8!` | 调用 `cp_cv_version` 并返回 `[*:0]&u8!`。 |
| `cp_cv_gaussian_blur_u8` | `(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32` | 调用 `cp_cv_gaussian_blur_u8` 并返回 `i32`。 |
| `cp_cv_canny_u8` | `(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32` | 调用 `cp_cv_canny_u8` 并返回 `i32`。 |
| `cp_cv_resize_u8` | `(src: [*]&u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32` | 调用 `cp_cv_resize_u8` 并返回 `i32`。 |
| `version` | `(): string` | 调用 `version` 并返回 `string`。 |
| `gaussianBlurU8` | `(src: &[]u8, dst: []u8, w: i32, h: i32, ksize: i32): !void` | 调用 `gaussianBlurU8` 并返回 `!void`。 |
| `cannyU8` | `(src: &[]u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): !void` | 执行 `cannyU8` 条件判定，返回 `bool`。 |
| `resizeU8` | `(src: &[]u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): !void` | 调用 `resizeU8` 并返回 `!void`。 |

## 最小调用示例

```cp
import { version } from "@cap/opencv";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
