[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-number

Zero-heap strict numeric string validator (integers, decimals, scientific, hex) matching npm is-number in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-number` ([https://github.com/jonschlinkert/is-number](https://github.com/jonschlinkert/is-number))

| 元信息 | 值 |
| --- | --- |
| 包名 | `is-number` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add is-number
```

```cp
import { isNumber, isIntegerStr, isFiniteF64 } from "is-number";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isNumber` | `(s: string): bool` | 执行 `isNumber` 条件判定，返回 `bool`。 |
| `isIntegerStr` | `(s: string): bool` | 执行 `isIntegerStr` 条件判定，返回 `bool`。 |
| `isFiniteF64` | `(x: f64): bool` | 执行 `isFiniteF64` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isNumber } from "is-number";

pub fn main() {
    let result = isNumber("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
