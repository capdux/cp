[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/whisper

whisper.cpp (v1.9.6) local speech-to-text (ASR) engine for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`whisper.cpp`](https://github.com/ggml-org/whisper.cpp)（`MIT`）（原库名 `whisper.cpp`，CP 包名 `@cap/whisper`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/whisper` |
| 版本 | `1.9.6` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/whisper
```

```cp
import {
    WhisperError,
    WhisperContext,
    cp_whisper_version,
    cp_whisper_print_system_info,
    cp_whisper_lang_max_id,
    cp_whisper_lang_id,
    cp_whisper_lang_str,
    cp_whisper_init_from_file,
} from "@cap/whisper";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `WhisperError` | `InitFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WhisperContext` | `class` | 字段: `ctx_ptr: *auto!?` · 方法: `initFromFile, isValid, nVocab, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_whisper_version` | `(): [*:0]&u8!` | 调用 `cp_whisper_version` 并返回 `[*:0]&u8!`。 |
| `cp_whisper_print_system_info` | `(): [*:0]&u8!` | 调用 `cp_whisper_print_system_info` 并返回 `[*:0]&u8!`。 |
| `cp_whisper_lang_max_id` | `(): i32` | 调用 `cp_whisper_lang_max_id` 并返回 `i32`。 |
| `cp_whisper_lang_id` | `(lang: [*:0]&u8!): i32` | 调用 `cp_whisper_lang_id` 并返回 `i32`。 |
| `cp_whisper_lang_str` | `(id: i32): [*:0]&u8!` | 调用 `cp_whisper_lang_str` 并返回 `[*:0]&u8!`。 |
| `cp_whisper_init_from_file` | `(path: [*:0]&u8!): *auto!?` | 调用 `cp_whisper_init_from_file` 并返回 `*auto!?`。 |
| `cp_whisper_free` | `(ctx: *auto!?)` | 就地执行 `cp_whisper_free` 操作。 |
| `cp_whisper_n_vocab` | `(ctx: *auto!?): i32` | 调用 `cp_whisper_n_vocab` 并返回 `i32`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |
| `systemInfo` | `(): string` | 调用 `systemInfo` 并返回 `string`。 |
| `langMaxId` | `(): i32` | 调用 `langMaxId` 并返回 `i32`。 |
| `langId` | `(code: string): i32` | 调用 `langId` 并返回 `i32`。 |
| `langStr` | `(id: i32): string` | 调用 `langStr` 并返回 `string`。 |

## 最小调用示例

```cp
import { langId } from "@cap/whisper";

pub fn main() {
    let result = langId("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
