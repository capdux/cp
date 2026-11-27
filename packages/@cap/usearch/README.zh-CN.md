[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/usearch

USearch (v2.26.6) high-performance HNSW vector search engine for CP AI Agents

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/usearch` |
| 版本 | `2.26.6` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/usearch
```

```cp
import {
    UsearchError,
    UsearchInitOptions,
    VectorIndex,
    usearch_version,
    usearch_init,
    usearch_free,
    usearch_reserve,
    usearch_add,
} from "@cap/usearch";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `UsearchError` | `InvalidDimensions, InitFailed, ReserveFailed, AddFailed, SearchFailed, RemoveFailed, IndexClosed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UsearchInitOptions` | `class` | 字段: `metric_kind: u32, metric: *auto!?, quantization: u32, dimensions: usize, connectivity: usize, expansion_add: usize, +2 项` |
| `VectorIndex` | `class` | 字段: `handle: *auto!?, dims: usize` · 方法: `initWithMetric, initCosine, initL2, reserve, addF32, searchF32, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `usearch_version` | `(): [*:0]&u8!` | 调用 `usearch_version` 并返回 `[*:0]&u8!`。 |
| `usearch_init` | `(options: *UsearchInitOptions!, err: *[*:0]&u8!?!): *auto!?` | 调用 `usearch_init` 并返回 `*auto!?`。 |
| `usearch_free` | `(index: *auto!?, err: *[*:0]&u8!?!)` | 就地执行 `usearch_free` 操作。 |
| `usearch_reserve` | `(index: *auto!?, capacity: usize, err: *[*:0]&u8!?!)` | 就地执行 `usearch_reserve` 操作。 |
| `usearch_add` | `(index: *auto!?, key: u64, vector: *&anyopaque!, vector_kind: u32, err: *[*:0]&u8!?!)` | 就地执行 `usearch_add` 操作。 |
| `usearch_search` | `(index: *auto!?, query_vector: *&anyopaque!, query_kind: u32, count: usize, keys: [*]u64!, distances: [*]f32!, err: *[*:0]&u8!?!): usize` | 调用 `usearch_search` 并返回 `usize`。 |
| `usearch_size` | `(index: *auto!?, err: *[*:0]&u8!?!): usize` | 调用 `usearch_size` 并返回 `usize`。 |
| `usearch_dimensions` | `(index: *auto!?, err: *[*:0]&u8!?!): usize` | 调用 `usearch_dimensions` 并返回 `usize`。 |
| `usearch_contains` | `(index: *auto!?, key: u64, err: *[*:0]&u8!?!): bool` | 调用 `usearch_contains` 并返回 `bool`。 |
| `usearch_remove` | `(index: *auto!?, key: u64, err: *[*:0]&u8!?!): usize` | 调用 `usearch_remove` 并返回 `usize`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/usearch";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
