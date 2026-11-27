[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/dav1d

VideoLAN dav1d (v1.5.4) high-performance royalty-free AV1 video decoder for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`dav1d`](https://code.videolan.org/videolan/dav1d)（`BSD-2-Clause`）（包名 `@cap/dav1d`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/dav1d` |
| 版本 | `1.5.4` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/dav1d
```

```cp
import {
    Dav1dError,
    Dav1dDecoder,
    cp_dav1d_version,
    cp_dav1d_version_api,
    cp_dav1d_open,
    cp_dav1d_flush,
    cp_dav1d_close,
    version,
} from "@cap/dav1d";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `Dav1dError` | `OpenFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Dav1dDecoder` | `class` | 字段: `raw: *auto!?` · 方法: `open, isOpen, flush, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_dav1d_version` | `(): [*:0]&u8!` | 调用 `cp_dav1d_version` 并返回 `[*:0]&u8!`。 |
| `cp_dav1d_version_api` | `(): u32` | 调用 `cp_dav1d_version_api` 并返回 `u32`。 |
| `cp_dav1d_open` | `(n_threads: i32, max_frame_delay: i32): *auto!?` | 调用 `cp_dav1d_open` 并返回 `*auto!?`。 |
| `cp_dav1d_flush` | `(ctx: *auto!?)` | 就地执行 `cp_dav1d_flush` 操作。 |
| `cp_dav1d_close` | `(ctx: *auto!?)` | 就地执行 `cp_dav1d_close` 操作。 |
| `version` | `(): string` | 调用 `version` 并返回 `string`。 |
| `versionApi` | `(): u32` | 调用 `versionApi` 并返回 `u32`。 |

## 最小调用示例

```cp
import { version } from "@cap/dav1d";

pub fn main() {
    let result = version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
