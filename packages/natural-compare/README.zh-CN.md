[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# natural-compare

Zero-allocation natural / alphanumeric string comparison and in-place sorting (natural-compare / natsort / natord parity, arbitrary-length integer segments without overflow) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:natural-compare` ([https://github.com/litejs/natural-compare-lite](https://github.com/litejs/natural-compare-lite))（原库名 `natural-compare` → CP 包名 `natural-compare`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `natural-compare` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add natural-compare
```

```cp
import {
    compareRawBytes,
    naturalCompareImpl,
    naturalCompare,
    naturalCompareIgnoreCase,
    naturalLess,
    naturalLessIgnoreCase,
    naturalEq,
    comesBefore,
} from "natural-compare";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `compareRawBytes` | `(a: string, b: string): i8` | 调用 `compareRawBytes` 并返回 `i8`。 |
| `naturalCompareImpl` | `(a: string, b: string, ignore_case: bool): i8` | 调用 `naturalCompareImpl` 并返回 `i8`。 |
| `naturalCompare` | `(a: string, b: string): i8` | 调用 `naturalCompare` 并返回 `i8`。 |
| `naturalCompareIgnoreCase` | `(a: string, b: string): i8` | 调用 `naturalCompareIgnoreCase` 并返回 `i8`。 |
| `naturalLess` | `(a: string, b: string): bool` | 调用 `naturalLess` 并返回 `bool`。 |
| `naturalLessIgnoreCase` | `(a: string, b: string): bool` | 调用 `naturalLessIgnoreCase` 并返回 `bool`。 |
| `naturalEq` | `(a: string, b: string): bool` | 调用 `naturalEq` 并返回 `bool`。 |
| `comesBefore` | `(a: string, b: string, desc: bool, ignore_case: bool): bool` | 调用 `comesBefore` 并返回 `bool`。 |
| `siftDown` | `(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool)` | 就地执行 `siftDown` 操作。 |
| `sortInPlace` | `(items: []string, desc: bool, ignore_case: bool)` | 就地执行 `sortInPlace` 操作。 |
| `sortNatural` | `(items: []string)` | 就地执行 `sortNatural` 操作。 |
| `sortNaturalDesc` | `(items: []string)` | 就地执行 `sortNaturalDesc` 操作。 |
| `sortNaturalIgnoreCase` | `(items: []string)` | 就地执行 `sortNaturalIgnoreCase` 操作。 |
| `isSortedNatural` | `(items: []string): bool` | 执行 `isSortedNatural` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isSortedNatural } from "natural-compare";

pub fn main() {
    let result = isSortedNatural(/* items: []string */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
