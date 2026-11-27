[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# string-similarity

Zero-allocation Sorensen-Dice bigram string similarity coefficient and best-match finder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-similarity` ([https://github.com/aceakash/string-similarity](https://github.com/aceakash/string-similarity))（原库名 `string-similarity` → CP 包名 `string-similarity`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `string-similarity` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add string-similarity
```

```cp
import {
    dcToLower,
    dcIsSpace,
    dcNormalize,
    dcEval,
    diceCoefficient,
    diceCaseSensitive,
    bestMatchIndex,
} from "string-similarity";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `dcToLower` | `(c: u8): u8` | 调用 `dcToLower` 并返回 `u8`。 |
| `dcIsSpace` | `(c: u8): bool` | 调用 `dcIsSpace` 并返回 `bool`。 |
| `dcNormalize` | `(src: &[]u8, strip_ws_lower: bool, dst: []u8): usize` | 调用 `dcNormalize` 并返回 `usize`。 |
| `dcEval` | `(a: string, b: string, strip_ws_lower: bool): f64` | 调用 `dcEval` 并返回 `f64`。 |
| `diceCoefficient` | `(a: string, b: string): f64` | 调用 `diceCoefficient` 并返回 `f64`。 |
| `diceCaseSensitive` | `(a: string, b: string): f64` | 调用 `diceCaseSensitive` 并返回 `f64`。 |
| `bestMatchIndex` | `(target: string, candidates: &[]string, min_rating: f64): i64` | 调用 `bestMatchIndex` 并返回 `i64`。 |

## 最小调用示例

```cp
import { dcEval } from "string-similarity";

pub fn main() {
    let result = dcEval("example", "example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
