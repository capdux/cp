[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/miniaudio

Cross-platform audio playback, capture, decoding (WAV/FLAC/MP3) and PCM resampling (miniaudio 0.11.21) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/miniaudio` |
| 版本 | `0.11.21` |
| 许可证 | `MIT-0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/miniaudio
```

```cp
import {
    MiniaudioError,
    ResampleResult,
    DecodedAudio,
    Resampler,
    cp_ma_version_string,
    cp_ma_decoder_from_memory,
    cp_ma_free,
    cp_ma_resampler_create,
} from "@cap/miniaudio";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `MiniaudioError` | `DecodeFailed, ResamplerCreateFailed, ResampleFailed, InvalidArg, InvalidState` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ResampleResult` | `class` | 字段: `in_frames: u64, out_frames: u64` |
| `DecodedAudio` | `class` | 字段: `raw: [*]f32!?, channels: u32, sample_rate: u32, frames: u64` · 方法: `fromMemory, samples, __cp_deinit` |
| `Resampler` | `class` | 字段: `raw: *auto!?, in_rate: u32, out_rate: u32, channels: u32` · 方法: `create, process, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_ma_version_string` | `(): [*:0]&u8!?` | 调用 `cp_ma_version_string` 并返回 `[*:0]&u8!?`。 |
| `cp_ma_decoder_from_memory` | `(data: [*]&u8!, size: usize, out_channels: *u32!?, out_sample_rate: *u32!?, out_frames: *u64!?): [*]f32!?` | 调用 `cp_ma_decoder_from_memory` 并返回 `[*]f32!?`。 |
| `cp_ma_free` | `(ptr: *auto!?)` | 就地执行 `cp_ma_free` 操作。 |
| `cp_ma_resampler_create` | `(in_rate: u32, out_rate: u32, channels: u32): *auto!?` | 调用 `cp_ma_resampler_create` 并返回 `*auto!?`。 |
| `cp_ma_resampler_process` | `(resampler: *auto!?, frames_in: [*]&f32!, frame_count_in: *u64!?, frames_out: [*]f32!, frame_count_out: *u64!?): i32` | 调用 `cp_ma_resampler_process` 并返回 `i32`。 |
| `cp_ma_resampler_free` | `(resampler: *auto!?)` | 就地执行 `cp_ma_resampler_free` 操作。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/miniaudio";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
