[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/dr_libs

Single-header zero-dependency WAV, MP3, and FLAC audio decoders/encoders (dr_libs) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/dr_libs` |
| 版本 | `0.13.17` |
| 许可证 | `MIT-0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/dr_libs
```

```cp
import {
    AudioCodecError,
    WavBytes,
    DecodedPcm16,
    cp_drwav_encode_pcm16_mem,
    cp_drwav_decode_pcm16_mem,
    cp_drflac_decode_pcm16_mem,
    cp_dr_free,
} from "@cap/dr_libs";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `AudioCodecError` | `EncodeFailed, DecodeFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WavBytes` | `class` | 字段: `raw: [*]u8!?, len: usize` · 方法: `encodePcm16, slice, __cp_deinit` |
| `DecodedPcm16` | `class` | 字段: `raw: [*]i16!?, channels: u32, sample_rate: u32, frame_count: u64` · 方法: `decodeWav, decodeFlac, samples, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_drwav_encode_pcm16_mem` | `(pcm: [*]&i16!, frame_count: u64, channels: u32, sample_rate: u32, out_size: *usize!): [*]u8!?` | 调用 `cp_drwav_encode_pcm16_mem` 并返回 `[*]u8!?`。 |
| `cp_drwav_decode_pcm16_mem` | `(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?` | 调用 `cp_drwav_decode_pcm16_mem` 并返回 `[*]i16!?`。 |
| `cp_drflac_decode_pcm16_mem` | `(data: [*]&u8!, data_size: usize, out_channels: *u32!, out_sample_rate: *u32!, out_frames: *u64!): [*]i16!?` | 调用 `cp_drflac_decode_pcm16_mem` 并返回 `[*]i16!?`。 |
| `cp_dr_free` | `(ptr: *auto!?)` | 就地执行 `cp_dr_free` 操作。 |

## 最小调用示例

```cp
import { cp_drwav_encode_pcm16_mem } from "@cap/dr_libs";

pub fn main() {
    let input = "example";
    let result = cp_drwav_encode_pcm16_mem(input.ptr, 0, 0, 0, /* out_size: *usize! */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
