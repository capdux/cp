[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/vorbis

Xiph libvorbis 1.3.7 royalty-free audio codec (self-contained with libogg) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`vorbis`](https://github.com/xiph/vorbis)（`BSD-3-Clause`）（包名 `@cap/vorbis`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/vorbis` |
| 版本 | `1.3.7` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/vorbis
```

```cp
import {
    VorbisError,
    cp_vorbis_version_string,
    cp_vorbis_encoder_test_init,
    versionString,
    encoderTestInit,
} from "@cap/vorbis";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `VorbisError` | `InvalidParams, InitFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_vorbis_version_string` | `(): [*:0]&u8!` | 调用 `cp_vorbis_version_string` 并返回 `[*:0]&u8!`。 |
| `cp_vorbis_encoder_test_init` | `(channels: i32, rate: isize, quality: f32): i32` | 调用 `cp_vorbis_encoder_test_init` 并返回 `i32`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |
| `encoderTestInit` | `(channels: i32, rate: isize, quality: f32): !void` | 通过 `encoderTestInit` 编码或格式化输出，返回 `!void`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/vorbis";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
