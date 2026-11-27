[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-rational

Zero-allocation exact rational number (Ratio64) arithmetic library with canonical reduction via num-integer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-rational` ([https://github.com/rust-num/num-rational](https://github.com/rust-num/num-rational))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-rational` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-rational
```

```cp
import { Ratio64, nratAbsU64, nratReduce } from "num-rational";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ratio64` | `class` | 字段: `numer: i64, denom: i64` · 方法: `new, fromInt, zero, one, isInteger, isZero, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nratAbsU64` | `(v: i64): u64` | 调用 `nratAbsU64` 并返回 `u64`。 |
| `nratReduce` | `(numer: i64, denom: i64): Ratio64` | 调用 `nratReduce` 并返回 `Ratio64`。 |

## 最小调用示例

```cp
import { nratAbsU64 } from "num-rational";

pub fn main() {
    let result = nratAbsU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
