[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/opus

Xiph libopus 1.5.2 low-latency audio codec for voice AI agents and WebRTC in CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/opus` |
| 版本 | `1.5.2` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/opus
```

```cp
import {
    OpusError,
    OpusEncoder,
    OpusDecoder,
    opus_get_version_string,
    opus_encoder_create,
    opus_encoder_destroy,
    opus_encode,
    opus_decoder_create,
} from "@cap/opus";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `OpusError` | `BadArg, BufferTooSmall, InternalError, InvalidPacket, Unimplemented, InvalidState, AllocFail, EncodeFailed ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OpusEncoder` | `class` | 字段: `raw: *auto!?, sample_rate: i32, channels: i32` · 方法: `create, encode, __cp_deinit` |
| `OpusDecoder` | `class` | 字段: `raw: *auto!?, sample_rate: i32, channels: i32` · 方法: `create, decode, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `opus_get_version_string` | `(): [*:0]&u8!?` | 调用 `opus_get_version_string` 并返回 `[*:0]&u8!?`。 |
| `opus_encoder_create` | `(fs: i32, channels: i32, application: i32, err_out: *i32!?): *auto!?` | 调用 `opus_encoder_create` 并返回 `*auto!?`。 |
| `opus_encoder_destroy` | `(st: *auto!?)` | 就地执行 `opus_encoder_destroy` 操作。 |
| `opus_encode` | `(st: *auto!?, pcm: [*]&i16!, frame_size: i32, data: [*]u8!, max_data_bytes: i32): i32` | 调用 `opus_encode` 并返回 `i32`。 |
| `opus_decoder_create` | `(fs: i32, channels: i32, err_out: *i32!?): *auto!?` | 调用 `opus_decoder_create` 并返回 `*auto!?`。 |
| `opus_decoder_destroy` | `(st: *auto!?)` | 就地执行 `opus_decoder_destroy` 操作。 |
| `opus_decode` | `(st: *auto!?, data: [*]&u8!, len: i32, pcm: [*]i16!, frame_size: i32, decode_fec: i32): i32` | 调用 `opus_decode` 并返回 `i32`。 |
| `mapOpusError` | `(code: i32): OpusError` | 调用 `mapOpusError` 并返回 `OpusError`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/opus";

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
