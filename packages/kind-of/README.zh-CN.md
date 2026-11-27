[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kind-of

Fast zero-heap value & JSON/literal type classifier (clean-room CP implementation of npm kind-of)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kind-of` ([https://github.com/jonschlinkert/kind-of](https://github.com/jonschlinkert/kind-of))

| 元信息 | 值 |
| --- | --- |
| 包名 | `kind-of` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kind-of
```

```cp
import {
    koIsSpace,
    koIsDigit,
    koEq,
    koStartsWith,
    koIsNumber,
    koIsBigInt,
    koIsIsoDate,
    kindOf,
} from "kind-of";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `koIsSpace` | `(c: u8): bool` | 调用 `koIsSpace` 并返回 `bool`。 |
| `koIsDigit` | `(c: u8): bool` | 调用 `koIsDigit` 并返回 `bool`。 |
| `koEq` | `(s: string, start: usize, end: usize, lit: string): bool` | 调用 `koEq` 并返回 `bool`。 |
| `koStartsWith` | `(s: string, start: usize, end: usize, prefix: string): bool` | 调用 `koStartsWith` 并返回 `bool`。 |
| `koIsNumber` | `(s: string, start: usize, end: usize): bool` | 调用 `koIsNumber` 并返回 `bool`。 |
| `koIsBigInt` | `(s: string, start: usize, end: usize): bool` | 调用 `koIsBigInt` 并返回 `bool`。 |
| `koIsIsoDate` | `(s: string, start: usize, end: usize): bool` | 调用 `koIsIsoDate` 并返回 `bool`。 |
| `kindOf` | `(raw: string): u8` | 调用 `kindOf` 并返回 `u8`。 |
| `kindName` | `(kind: u8): string` | 调用 `kindName` 并返回 `string`。 |
| `kindOfName` | `(raw: string): string` | 调用 `kindOfName` 并返回 `string`。 |
| `isPrimitive` | `(raw: string): bool` | 执行 `isPrimitive` 条件判定，返回 `bool`。 |
| `isPlainObject` | `(raw: string): bool` | 执行 `isPlainObject` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isPrimitive } from "kind-of";

pub fn main() {
    let result = isPrimitive("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
