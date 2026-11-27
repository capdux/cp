[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# accept-header

Zero-heap RFC 7231 HTTP Accept header parser and media-type negotiator with quality-factor & specificity ranking (clean-room port of Rust accept-header)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:accept-header` ([https://github.com/djc/accept-header](https://github.com/djc/accept-header))

| 元信息 | 值 |
| --- | --- |
| 包名 | `accept-header` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add accept-header
```

```cp
import {
    AcceptEntry,
    AcceptHeader,
    acphFindSlash,
    acphIsStar,
    acphMediaSpecificity,
    acphMatchesRange,
    acphEmptyEntry,
    acphEmptySpec,
} from "accept-header";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AcceptEntry` | `class` | 字段: `media_range: string, quality_milli: u16, specificity: u8` |
| `AcceptHeader` | `class` | 字段: `entries: [16]AcceptEntry, count: usize` · 方法: `parse, len, accepts, bestMatch` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `acphFindSlash` | `(s: string): usize` | 调用 `acphFindSlash` 并返回 `usize`。 |
| `acphIsStar` | `(s: string): bool` | 调用 `acphIsStar` 并返回 `bool`。 |
| `acphMediaSpecificity` | `(range: string): u8` | 调用 `acphMediaSpecificity` 并返回 `u8`。 |
| `acphMatchesRange` | `(range: string, mime: string): bool` | 调用 `acphMatchesRange` 并返回 `bool`。 |
| `acphEmptyEntry` | `(): AcceptEntry` | 调用 `acphEmptyEntry` 并返回 `AcceptEntry`。 |
| `acphEmptySpec` | `(): QualitySpec` | 调用 `acphEmptySpec` 并返回 `QualitySpec`。 |
| `negotiateMediaType` | `(header: string, available: []string): string?` | 调用 `negotiateMediaType` 并返回 `string?`。 |

## 最小调用示例

```cp
import { acphFindSlash } from "accept-header";

pub fn main() {
    let result = acphFindSlash("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
