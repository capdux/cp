[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# esutils

Zero-allocation ECMAScript keyword, reserved word, identifier, whitespace, and line terminator utilities (clean-room MIT implementation of npm esutils, #201 / 215M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esutils` ([https://github.com/estools/esutils](https://github.com/estools/esutils)) | 许可：`BSD-2-Clause`

| 元信息 | 值 |
| --- | --- |
| 包名 | `esutils` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add esutils
```

```cp
import {
    isDecimalDigit,
    isHexDigit,
    isOctalDigit,
    isWhiteSpace,
    isLineTerminator,
    isIdentifierStartES6,
    isIdentifierPartES6,
    isKeywordES6,
} from "esutils";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isDecimalDigit` | `(cp: u32): bool` | 执行 `isDecimalDigit` 条件判定，返回 `bool`。 |
| `isHexDigit` | `(cp: u32): bool` | 执行 `isHexDigit` 条件判定，返回 `bool`。 |
| `isOctalDigit` | `(cp: u32): bool` | 执行 `isOctalDigit` 条件判定，返回 `bool`。 |
| `isWhiteSpace` | `(cp: u32): bool` | 执行 `isWhiteSpace` 条件判定，返回 `bool`。 |
| `isLineTerminator` | `(cp: u32): bool` | 执行 `isLineTerminator` 条件判定，返回 `bool`。 |
| `isIdentifierStartES6` | `(cp: u32): bool` | 执行 `isIdentifierStartES6` 条件判定，返回 `bool`。 |
| `isIdentifierPartES6` | `(cp: u32): bool` | 执行 `isIdentifierPartES6` 条件判定，返回 `bool`。 |
| `isKeywordES6` | `(id: string, strict: bool): bool` | 执行 `isKeywordES6` 条件判定，返回 `bool`。 |
| `isReservedWordES6` | `(id: string, strict: bool): bool` | 执行 `isReservedWordES6` 条件判定，返回 `bool`。 |
| `isRestrictedWord` | `(id: string): bool` | 执行 `isRestrictedWord` 条件判定，返回 `bool`。 |
| `isIdentifierNameES6` | `(id: string): bool` | 执行 `isIdentifierNameES6` 条件判定，返回 `bool`。 |
| `isIdentifierES6` | `(id: string, strict: bool): bool` | 执行 `isIdentifierES6` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isKeywordES6 } from "esutils";

pub fn main() {
    let result = isKeywordES6("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
