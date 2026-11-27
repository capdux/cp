[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# regex

PCRE2 10.44 Perl-compatible regular expression engine with SLJIT native JIT acceleration for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `regex` |
| 版本 | `10.44.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add regex
```

```cp
import {
    Pcre2Error,
    Pcre2Match,
    Pcre2Regex,
    cp_pcre2_compile,
    cp_pcre2_match,
    cp_pcre2_free,
    cp_pcre2_is_jit,
    cp_pcre2_version,
} from "regex";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `Pcre2Error` | `CompileFailed, MatchFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pcre2Match` | `class` | 字段: `start: usize, end: usize` · 方法: `slice` |
| `Pcre2Regex` | `class` | 字段: `raw: *auto!?` · 方法: `compile, compileJit, compileWithOptions, versionString, isJitEnabled, find, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_pcre2_compile` | `(pattern: [*]&u8!, pat_len: usize, opts: u32, jit: i32): *auto!?` | 调用 `cp_pcre2_compile` 并返回 `*auto!?`。 |
| `cp_pcre2_match` | `(re: *auto!?, subj: [*]&u8!, subj_len: usize, start_off: usize, out_start: *usize!?, out_end: *usize!?): i32` | 调用 `cp_pcre2_match` 并返回 `i32`。 |
| `cp_pcre2_free` | `(re: *auto!?)` | 就地执行 `cp_pcre2_free` 操作。 |
| `cp_pcre2_is_jit` | `(re: *auto!?): i32` | 调用 `cp_pcre2_is_jit` 并返回 `i32`。 |
| `cp_pcre2_version` | `(): [*:0]&u8!?` | 调用 `cp_pcre2_version` 并返回 `[*:0]&u8!?`。 |

## 最小调用示例

```cp
import { cp_pcre2_version } from "regex";

pub fn main() {
    let result = cp_pcre2_version();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
