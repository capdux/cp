[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/rnnoise

Xiph RNNoise recurrent neural network speech denoising and VAD for CP voice AI agents

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/rnnoise` |
| 版本 | `0.1.1` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/rnnoise
```

```cp
import {
    RnnoiseError,
    DenoiseState,
    rnnoise_get_size,
    rnnoise_get_frame_size,
    rnnoise_create,
    rnnoise_destroy,
    rnnoise_process_frame,
    queryFrameSize,
} from "@cap/rnnoise";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `RnnoiseError` | `AllocFail, InvalidState, BufferTooSmall` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DenoiseState` | `class` | 字段: `raw: *auto!?` · 方法: `frameSize, create, processFrame, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rnnoise_get_size` | `(): i32` | 调用 `rnnoise_get_size` 并返回 `i32`。 |
| `rnnoise_get_frame_size` | `(): i32` | 调用 `rnnoise_get_frame_size` 并返回 `i32`。 |
| `rnnoise_create` | `(model: *auto!?): *auto!?` | 调用 `rnnoise_create` 并返回 `*auto!?`。 |
| `rnnoise_destroy` | `(st: *auto!?)` | 就地执行 `rnnoise_destroy` 操作。 |
| `rnnoise_process_frame` | `(st: *auto!?, out: [*]f32!, input: [*]&f32!): f32` | 调用 `rnnoise_process_frame` 并返回 `f32`。 |
| `queryFrameSize` | `(): usize` | 调用 `queryFrameSize` 并返回 `usize`。 |
| `frameSize` | `(): usize` | 调用 `frameSize` 并返回 `usize`。 |

## 最小调用示例

```cp
import { rnnoise_get_size } from "@cap/rnnoise";

pub fn main() {
    let result = rnnoise_get_size();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
