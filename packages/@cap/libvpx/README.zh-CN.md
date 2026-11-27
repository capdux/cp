[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/libvpx

Google libvpx 1.15.0 royalty-free VP8/VP9 video codec for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`libvpx`](https://github.com/webmproject/libvpx)（`BSD-3-Clause`）（包名 `@cap/libvpx`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/libvpx` |
| 版本 | `1.15.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/libvpx
```

```cp
import {
    VpxError,
    Vp8DefaultSize,
    cp_vpx_version_str,
    cp_vpx_vp8_enc_cfg_default,
    cp_vpx_vp9_dec_open_close,
    versionStr,
    vp8EncCfgDefault,
    vp9DecOpenClose,
} from "@cap/libvpx";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `VpxError` | `EncConfigFailed, DecoderInitFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Vp8DefaultSize` | `class` | 字段: `width: u32, height: u32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_vpx_version_str` | `(): [*:0]&u8!` | 调用 `cp_vpx_version_str` 并返回 `[*:0]&u8!`。 |
| `cp_vpx_vp8_enc_cfg_default` | `(out_w: *u32!, out_h: *u32!): i32` | 调用 `cp_vpx_vp8_enc_cfg_default` 并返回 `i32`。 |
| `cp_vpx_vp9_dec_open_close` | `(threads: i32): i32` | 调用 `cp_vpx_vp9_dec_open_close` 并返回 `i32`。 |
| `versionStr` | `(): string` | 调用 `versionStr` 并返回 `string`。 |
| `vp8EncCfgDefault` | `(): !Vp8DefaultSize` | 调用 `vp8EncCfgDefault` 并返回 `!Vp8DefaultSize`。 |
| `vp9DecOpenClose` | `(threads: i32): !void` | 调用 `vp9DecOpenClose` 并返回 `!void`。 |

## 最小调用示例

```cp
import { versionStr } from "@cap/libvpx";

pub fn main() {
    let result = versionStr();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
