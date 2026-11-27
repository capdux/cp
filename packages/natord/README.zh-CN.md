[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# natord

Zero-heap arbitrary-precision human natural sort comparator, case-insensitive comparator, and in-place sorter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:natord` ([https://github.com/lifthrasiir/rust-natord](https://github.com/lifthrasiir/rust-natord))（原库名 `natord` → CP 包名 `natord`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `natord` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add natord
```

```cp
import {
    compareImpl,
    compareNatural,
    compareNaturalIgnoreCase,
    isSortedNatural,
    sortNatural,
} from "natord";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `compareImpl` | `(a: string, b: string, ignore_case: bool): i32` | 调用 `compareImpl` 并返回 `i32`。 |
| `compareNatural` | `(a: string, b: string): i32` | 调用 `compareNatural` 并返回 `i32`。 |
| `compareNaturalIgnoreCase` | `(a: string, b: string): i32` | 调用 `compareNaturalIgnoreCase` 并返回 `i32`。 |
| `isSortedNatural` | `(items: &[]string): bool` | 执行 `isSortedNatural` 条件判定，返回 `bool`。 |
| `sortNatural` | `(items: []string)` | 就地执行 `sortNatural` 操作。 |

## 最小调用示例

```cp
import { isSortedNatural } from "natord";

pub fn main() {
    let result = isSortedNatural(/* items: &[]string */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
