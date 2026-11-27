[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/flac

Xiph libFLAC 1.4.3 lossless audio encoder and decoder engine for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/flac` |
| 版本 | `1.4.3` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/flac
```

```cp
import {
    FlacError,
    cp_flac_version,
    cp_flac_vendor,
    cp_flac_encode_pcm16_test,
    version,
    vendor,
    encodePcm16Test,
} from "@cap/flac";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `FlacError` | `InvalidInput, InitFailed, EncodeFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_flac_version` | `(): [*:0]&u8!?` | 调用 `cp_flac_version` 并返回 `[*:0]&u8!?`。 |
| `cp_flac_vendor` | `(): [*:0]&u8!?` | 调用 `cp_flac_vendor` 并返回 `[*:0]&u8!?`。 |
| `cp_flac_encode_pcm16_test` | `(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32` | 调用 `cp_flac_encode_pcm16_test` 并返回 `i32`。 |
| `version` | `(): string` | 调用 `version` 并返回 `string`。 |
| `vendor` | `(): string` | 调用 `vendor` 并返回 `string`。 |
| `encodePcm16Test` | `(pcm: &[]i16, sample_rate: u32): !u64` | 通过 `encodePcm16Test` 编码或格式化输出，返回 `!u64`。 |

## 最小调用示例

```cp
import { version } from "@cap/flac";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
