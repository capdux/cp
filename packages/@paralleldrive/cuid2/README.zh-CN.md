[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @paralleldrive/cuid2

Zero-heap collision-resistant, URL-safe, lowercase CUID2 ID generator and validator (clean-room port of npm @paralleldrive/cuid2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@paralleldrive/cuid2` ([https://github.com/paralleldrive/cuid2](https://github.com/paralleldrive/cuid2))（原库名 `@paralleldrive/cuid2` → CP 包名 `@paralleldrive/cuid2`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@paralleldrive/cuid2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @paralleldrive/cuid2
```

```cp
import {
    Cuid2Generator,
    c2Mix,
    c2Base36Char,
    c2LetterChar,
    isCuid2,
    isCuid2WithBounds,
} from "@paralleldrive/cuid2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Cuid2Generator` | `class` | 字段: `state: u64, counter: u64, fingerprint_hash: u64, id_len: usize` · 方法: `new, length, next` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `c2Mix` | `(x: u64): u64` | 调用 `c2Mix` 并返回 `u64`。 |
| `c2Base36Char` | `(v: u64): u8` | 调用 `c2Base36Char` 并返回 `u8`。 |
| `c2LetterChar` | `(v: u64): u8` | 调用 `c2LetterChar` 并返回 `u8`。 |
| `isCuid2` | `(id: string): bool` | 执行 `isCuid2` 条件判定，返回 `bool`。 |
| `isCuid2WithBounds` | `(id: string, min_len: usize, max_len: usize): bool` | 执行 `isCuid2WithBounds` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isCuid2 } from "@paralleldrive/cuid2";

pub fn main() {
    let result = isCuid2("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
