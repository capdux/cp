[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/sdl3

SDL3 (v3.2.8) cross-platform window, audio, input & GPU abstraction for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`SDL`](https://github.com/libsdl-org/SDL)（`Zlib`）（原库名 `SDL`，CP 包名 `@cap/sdl3`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/sdl3` |
| 版本 | `3.2.8` |
| 许可证 | `Zlib` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/sdl3
```

```cp
import {
    SdlError,
    SdlVersion,
    SdlContext,
    cp_sdl_version,
    cp_sdl_revision,
    cp_sdl_cpu_count,
    cp_sdl_system_ram,
    cp_sdl_init_headless,
} from "@cap/sdl3";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SdlError` | `InitFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SdlVersion` | `class` | 字段: `major: i32, minor: i32, micro: i32` |
| `SdlContext` | `class` | 字段: `active: bool` · 方法: `initHeadless, isActive, ticksNs, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_sdl_version` | `(): i32` | 调用 `cp_sdl_version` 并返回 `i32`。 |
| `cp_sdl_revision` | `(): [*:0]&u8!` | 调用 `cp_sdl_revision` 并返回 `[*:0]&u8!`。 |
| `cp_sdl_cpu_count` | `(): i32` | 调用 `cp_sdl_cpu_count` 并返回 `i32`。 |
| `cp_sdl_system_ram` | `(): i32` | 调用 `cp_sdl_system_ram` 并返回 `i32`。 |
| `cp_sdl_init_headless` | `(): i32` | 调用 `cp_sdl_init_headless` 并返回 `i32`。 |
| `cp_sdl_quit` | `()` | 就地执行 `cp_sdl_quit` 操作。 |
| `cp_sdl_ticks_ns` | `(): u64` | 调用 `cp_sdl_ticks_ns` 并返回 `u64`。 |
| `rawVersion` | `(): i32` | 调用 `rawVersion` 并返回 `i32`。 |
| `version` | `(): SdlVersion` | 调用 `version` 并返回 `SdlVersion`。 |
| `revision` | `(): string` | 调用 `revision` 并返回 `string`。 |
| `cpuCount` | `(): i32` | 调用 `cpuCount` 并返回 `i32`。 |
| `systemRamMB` | `(): i32` | 调用 `systemRamMB` 并返回 `i32`。 |
| `ticksNs` | `(): u64` | 调用 `ticksNs` 并返回 `u64`。 |

## 最小调用示例

```cp
import { version } from "@cap/sdl3";

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
