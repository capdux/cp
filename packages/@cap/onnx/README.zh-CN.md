[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/onnx

ONNX Runtime (v1.32.0) high-performance ML inference engine for CP via official prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`onnxruntime`](https://github.com/microsoft/onnxruntime)（`MIT`）（原库名 `onnxruntime`，CP 包名 `@cap/onnx`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/onnx` |
| 版本 | `1.32.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/onnx
```

```cp
import {
    OrtError,
    OrtEnv,
    OrtSessionOptions,
    cp_ort_version,
    cp_ort_create_env,
    cp_ort_release_env,
    cp_ort_create_session_options,
    cp_ort_set_intra_op_threads,
} from "@cap/onnx";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `OrtError` | `CreateEnvFailed, CreateSessionOptionsFailed, SetThreadsFailed, ClosedHandle, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OrtEnv` | `class` | 字段: `env_ptr: *auto!?` · 方法: `create, isValid, __cp_deinit` |
| `OrtSessionOptions` | `class` | 字段: `opts_ptr: *auto!?` · 方法: `create, setIntraOpThreads, isValid, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_ort_version` | `(): [*:0]&u8!` | 调用 `cp_ort_version` 并返回 `[*:0]&u8!`。 |
| `cp_ort_create_env` | `(log_id: [*:0]&u8!): *auto!?` | 调用 `cp_ort_create_env` 并返回 `*auto!?`。 |
| `cp_ort_release_env` | `(env: *auto!?)` | 就地执行 `cp_ort_release_env` 操作。 |
| `cp_ort_create_session_options` | `(): *auto!?` | 调用 `cp_ort_create_session_options` 并返回 `*auto!?`。 |
| `cp_ort_set_intra_op_threads` | `(opts: *auto!?, n: i32): i32` | 调用 `cp_ort_set_intra_op_threads` 并返回 `i32`。 |
| `cp_ort_release_session_options` | `(opts: *auto!?)` | 就地执行 `cp_ort_release_session_options` 操作。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/onnx";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
