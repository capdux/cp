[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# balanced-match

Zero-allocation nested balanced delimiter matcher (npm balanced-match v3 parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`balanced-match`](https://github.com/juliangruber/balanced-match)（`MIT`）（包名 `balanced-match`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `balanced-match` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add balanced-match
```

```cp
import {
    BalancedResult,
    bmMakeRange,
    bmEmptyResult,
    balancedRange,
    balanced,
    balancedAll,
} from "balanced-match";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BalancedResult` | `class` | 字段: `found: bool, start: usize, end: usize, pre: string, body: string, post: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bmMakeRange` | `(s: usize, e: usize): [usize, usize]` | 调用 `bmMakeRange` 并返回 `[usize, usize]`。 |
| `bmEmptyResult` | `(): BalancedResult` | 调用 `bmEmptyResult` 并返回 `BalancedResult`。 |
| `balancedRange` | `(a: string, b: string, input: string): [usize, usize]?` | 调用 `balancedRange` 并返回 `[usize, usize]?`。 |
| `balanced` | `(a: string, b: string, input: string): BalancedResult` | 调用 `balanced` 并返回 `BalancedResult`。 |
| `balancedAll` | `(a: string, b: string, input: string, out: []BalancedResult): usize` | 调用 `balancedAll` 并返回 `usize`。 |

## 最小调用示例

```cp
import { balancedRange } from "balanced-match";

pub fn main() {
    let result = balancedRange("example", "example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
