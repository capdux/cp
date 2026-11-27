[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pluralize

Zero-allocation English pluralization, singularization, irregular/uncountable dictionary, count formatting & ordinal engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pluralize` ([https://github.com/plurals/pluralize](https://github.com/plurals/pluralize))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pluralize` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pluralize
```

```cp
import {
    isVowelLower,
    detectCaseStyle,
    copySlice,
    writeWithCase,
    writeStemAndSuffix,
    inList,
    isUncountable,
    plural,
} from "pluralize";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isVowelLower` | `(c: u8): bool` | 执行 `isVowelLower` 条件判定，返回 `bool`。 |
| `detectCaseStyle` | `(word: &[]u8): u8` | 调用 `detectCaseStyle` 并返回 `u8`。 |
| `copySlice` | `(out: []u8, src: &[]u8): &[]u8` | 调用 `copySlice` 并返回 `&[]u8`。 |
| `writeWithCase` | `(out: []u8, src_lower: &[]u8, style: u8): &[]u8` | 调用 `writeWithCase` 并返回 `&[]u8`。 |
| `writeStemAndSuffix` | `(out: []u8, stem: &[]u8, suffix_lower: &[]u8, all_upper: bool): &[]u8` | 调用 `writeStemAndSuffix` 并返回 `&[]u8`。 |
| `inList` | `(lower: &[]u8, list: &[]&[]u8): bool` | 调用 `inList` 并返回 `bool`。 |
| `isUncountable` | `(word: &[]u8): bool` | 执行 `isUncountable` 条件判定，返回 `bool`。 |
| `plural` | `(word: &[]u8, out: []u8): &[]u8` | 调用 `plural` 并返回 `&[]u8`。 |
| `singular` | `(word: &[]u8, out: []u8): &[]u8` | 调用 `singular` 并返回 `&[]u8`。 |
| `isPlural` | `(word: &[]u8): bool` | 执行 `isPlural` 条件判定，返回 `bool`。 |
| `isSingular` | `(word: &[]u8): bool` | 执行 `isSingular` 条件判定，返回 `bool`。 |
| `pluralizeCount` | `(word: &[]u8, count: i64, out: []u8): &[]u8` | 调用 `pluralizeCount` 并返回 `&[]u8`。 |
| `ordinal` | `(n: i64, out: []u8): &[]u8` | 调用 `ordinal` 并返回 `&[]u8`。 |

## 最小调用示例

```cp
import { isVowelLower } from "pluralize";

pub fn main() {
    let result = isVowelLower(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
