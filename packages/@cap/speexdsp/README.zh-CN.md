[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/speexdsp

Xiph SpeexDSP 1.2.1 real-time acoustic echo cancellation (AEC), noise suppression, AGC, and VAD for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/speexdsp` |
| 版本 | `1.2.1` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/speexdsp
```

```cp
import {
    SpeexDspError,
    SpeexEchoState,
    SpeexPreprocessState,
    speex_echo_state_init,
    speex_echo_state_destroy,
    speex_echo_cancellation,
    speex_echo_state_reset,
    speex_echo_ctl,
} from "@cap/speexdsp";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SpeexDspError` | `InvalidArg, AllocFail, InvalidState, BufferTooSmall, CtlFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SpeexEchoState` | `class` | 字段: `raw: *auto!?, frame_size: i32, filter_length: i32` · 方法: `create, setSamplingRate, cancel, reset, rawPtr, __cp_deinit` |
| `SpeexPreprocessState` | `class` | 字段: `raw: *auto!?, frame_size: i32, sample_rate: i32` · 方法: `create, setDenoise, setAgc, setVad, setEchoState, run, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `speex_echo_state_init` | `(frame_size: i32, filter_length: i32): *auto!?` | 调用 `speex_echo_state_init` 并返回 `*auto!?`。 |
| `speex_echo_state_destroy` | `(st: *auto!?)` | 就地执行 `speex_echo_state_destroy` 操作。 |
| `speex_echo_cancellation` | `(st: *auto!?, rec: [*]&i16!, play: [*]&i16!, out: [*]i16!)` | 就地执行 `speex_echo_cancellation` 操作。 |
| `speex_echo_state_reset` | `(st: *auto!?)` | 就地执行 `speex_echo_state_reset` 操作。 |
| `speex_echo_ctl` | `(st: *auto!?, request: i32, ptr: *auto!?): i32` | 调用 `speex_echo_ctl` 并返回 `i32`。 |
| `speex_preprocess_state_init` | `(frame_size: i32, sampling_rate: i32): *auto!?` | 调用 `speex_preprocess_state_init` 并返回 `*auto!?`。 |
| `speex_preprocess_state_destroy` | `(st: *auto!?)` | 就地执行 `speex_preprocess_state_destroy` 操作。 |
| `speex_preprocess_run` | `(st: *auto!?, x: [*]i16!): i32` | 调用 `speex_preprocess_run` 并返回 `i32`。 |
| `speex_preprocess_ctl` | `(st: *auto!?, request: i32, ptr: *auto!?): i32` | 调用 `speex_preprocess_ctl` 并返回 `i32`。 |

## 最小调用示例

```cp
import { speex_echo_state_init } from "@cap/speexdsp";

pub fn main() {
    let result = speex_echo_state_init(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
