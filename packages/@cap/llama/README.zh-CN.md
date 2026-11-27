[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/llama

llama.cpp (b12676) local LLM inference engine for CP via official prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`llama.cpp`](https://github.com/ggml-org/llama.cpp)（`MIT`）（原库名 `llama.cpp`，CP 包名 `@cap/llama`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/llama` |
| 版本 | `1.0.12676` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/llama
```

```cp
import {
    LlamaError,
    LlamaBackend,
    LlamaModel,
    cp_llama_backend_init,
    cp_llama_backend_free,
    cp_llama_max_devices,
    cp_llama_supports_mmap,
    cp_llama_supports_mlock,
} from "@cap/llama";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `LlamaError` | `LoadModelFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LlamaBackend` | `class` | 字段: `active: bool` · 方法: `init, isActive, __cp_deinit` |
| `LlamaModel` | `class` | 字段: `model_ptr: *auto!?` · 方法: `load, isValid, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_llama_backend_init` | `()` | 就地执行 `cp_llama_backend_init` 操作。 |
| `cp_llama_backend_free` | `()` | 就地执行 `cp_llama_backend_free` 操作。 |
| `cp_llama_max_devices` | `(): usize` | 调用 `cp_llama_max_devices` 并返回 `usize`。 |
| `cp_llama_supports_mmap` | `(): i32` | 调用 `cp_llama_supports_mmap` 并返回 `i32`。 |
| `cp_llama_supports_mlock` | `(): i32` | 调用 `cp_llama_supports_mlock` 并返回 `i32`。 |
| `cp_llama_supports_gpu_offload` | `(): i32` | 调用 `cp_llama_supports_gpu_offload` 并返回 `i32`。 |
| `cp_llama_time_us` | `(): i64` | 调用 `cp_llama_time_us` 并返回 `i64`。 |
| `cp_llama_print_system_info` | `(): [*:0]&u8!` | 调用 `cp_llama_print_system_info` 并返回 `[*:0]&u8!`。 |
| `cp_llama_load_model` | `(path: [*:0]&u8!, n_gpu_layers: i32): *auto!?` | 调用 `cp_llama_load_model` 并返回 `*auto!?`。 |
| `cp_llama_free_model` | `(m: *auto!?)` | 就地执行 `cp_llama_free_model` 操作。 |
| `systemInfo` | `(): string` | 调用 `systemInfo` 并返回 `string`。 |
| `supportsMmap` | `(): bool` | 调用 `supportsMmap` 并返回 `bool`。 |
| `supportsMlock` | `(): bool` | 调用 `supportsMlock` 并返回 `bool`。 |
| `supportsGpuOffload` | `(): bool` | 调用 `supportsGpuOffload` 并返回 `bool`。 |
| `maxDevices` | `(): usize` | 调用 `maxDevices` 并返回 `usize`。 |
| `timeUs` | `(): i64` | 调用 `timeUs` 并返回 `i64`。 |

## 最小调用示例

```cp
import { cp_llama_max_devices } from "@cap/llama";

pub fn main() {
    let result = cp_llama_max_devices();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
