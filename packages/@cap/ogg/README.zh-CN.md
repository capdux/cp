[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/ogg

Xiph libogg 1.3.5 bitstream container framing and page muxing engine for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/ogg` |
| 版本 | `1.3.5` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/ogg
```

```cp
import {
    OggError,
    OggStream,
    cp_ogg_stream_init,
    cp_ogg_stream_packetin,
    cp_ogg_stream_pageout,
    cp_ogg_stream_flush,
    cp_ogg_stream_clear,
} from "@cap/ogg";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `OggError` | `InitFailed, InvalidState, PacketFailed, BufferTooSmall, PageFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OggStream` | `class` | 字段: `raw: *auto!?, serialno: i32` · 方法: `init, packetIn, pageOut, flush, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_ogg_stream_init` | `(serialno: i32): *auto!?` | 调用 `cp_ogg_stream_init` 并返回 `*auto!?`。 |
| `cp_ogg_stream_packetin` | `(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32` | 调用 `cp_ogg_stream_packetin` 并返回 `i32`。 |
| `cp_ogg_stream_pageout` | `(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32` | 调用 `cp_ogg_stream_pageout` 并返回 `i32`。 |
| `cp_ogg_stream_flush` | `(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32` | 调用 `cp_ogg_stream_flush` 并返回 `i32`。 |
| `cp_ogg_stream_clear` | `(os: *auto!?)` | 就地执行 `cp_ogg_stream_clear` 操作。 |

## 最小调用示例

```cp
import { cp_ogg_stream_init } from "@cap/ogg";

pub fn main() {
    let result = cp_ogg_stream_init(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
