[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# multimap

Zero-allocation insertion-ordered one-to-many string multimap (capacity 128) for HTTP headers, URL query params, tag indexes, and event routing (Rust multimap parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:multimap` ([https://github.com/havarnov/multimap](https://github.com/havarnov/multimap))

| 元信息 | 值 |
| --- | --- |
| 包名 | `multimap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add multimap
```

```cp
import { MultiEntry, MultiMapStr, strEq } from "multimap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MultiEntry` | `class` | 字段: `key: string, value: string` |
| `MultiMapStr` | `class` | 字段: `entries: [MULTIMAP_CAP]MultiEntry, count: usize` · 方法: `init, totalValues, isEmpty, isFull, clear, insert, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | 调用 `strEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { strEq } from "multimap";

pub fn main() {
    let result = strEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
