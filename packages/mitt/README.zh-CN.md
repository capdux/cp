[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mitt

Zero-allocation tiny functional event emitter with wildcard '*' support in pure CP (npm mitt v3 parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mitt` ([https://github.com/developit/mitt](https://github.com/developit/mitt))

| 元信息 | 值 |
| --- | --- |
| 包名 | `mitt` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mitt
```

```cp
import {
    MittListener,
    MittDispatch,
    Emitter,
    mitStrEq,
    mitIsWildcard,
    mitt,
} from "mitt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MittListener` | `class` | 字段: `active: bool, event_type: string, handler_id: u32, is_wildcard: bool` · 方法: `empty` |
| `MittDispatch` | `class` | 字段: `handler_id: u32, payload: i64, is_wildcard: bool` · 方法: `empty` |
| `Emitter` | `class` | 字段: `listeners: [32]MittListener, count: usize, last_dispatched: [32]MittDispatch, last_dispatch_count: usize, emit_count: usize` · 方法: `new, on, off, emit, listenerCount, totalListeners, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mitStrEq` | `(a: string, b: string): bool` | 调用 `mitStrEq` 并返回 `bool`。 |
| `mitIsWildcard` | `(event_type: string): bool` | 调用 `mitIsWildcard` 并返回 `bool`。 |
| `mitt` | `(): Emitter` | 调用 `mitt` 并返回 `Emitter`。 |

## 最小调用示例

```cp
import { mitStrEq } from "mitt";

pub fn main() {
    let result = mitStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
