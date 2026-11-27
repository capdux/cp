[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# url-template

Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and validator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-template` ([https://github.com/bramstein/url-template](https://github.com/bramstein/url-template))（原库名 `url-template` → CP 包名 `url-template`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `url-template` |
| 版本 | `0.1.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add url-template
```

```cp
import {
    TemplateVar,
    utIsAlpha,
    utIsDigit,
    utIsHex,
    utHexUpper,
    utIsUnreserved,
    utIsReserved,
    utIsOperator,
} from "url-template";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TemplateVar` | `class` | 字段: `name: string, value: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `utIsAlpha` | `(c: u8): bool` | 调用 `utIsAlpha` 并返回 `bool`。 |
| `utIsDigit` | `(c: u8): bool` | 调用 `utIsDigit` 并返回 `bool`。 |
| `utIsHex` | `(c: u8): bool` | 调用 `utIsHex` 并返回 `bool`。 |
| `utHexUpper` | `(n: u8): u8` | 调用 `utHexUpper` 并返回 `u8`。 |
| `utIsUnreserved` | `(c: u8): bool` | 调用 `utIsUnreserved` 并返回 `bool`。 |
| `utIsReserved` | `(c: u8): bool` | 调用 `utIsReserved` 并返回 `bool`。 |
| `utIsOperator` | `(c: u8): bool` | 调用 `utIsOperator` 并返回 `bool`。 |
| `utIsReservedOp` | `(c: u8): bool` | 调用 `utIsReservedOp` 并返回 `bool`。 |
| `utEqBytes` | `(a: &[]u8, b: &[]u8): bool` | 调用 `utEqBytes` 并返回 `bool`。 |
| `utIsValidVarSpec` | `(spec: &[]u8): bool` | 调用 `utIsValidVarSpec` 并返回 `bool`。 |
| `isValid` | `(tmpl: string): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |
| `countVars` | `(tmpl: string): usize` | 调用 `countVars` 并返回 `usize`。 |
| `extractVarName` | `(tmpl: string, index: usize, out: []u8): usize` | 调用 `extractVarName` 并返回 `usize`。 |
| `pctEncode` | `(input: string, allow_reserved: bool, out: []u8): usize` | 调用 `pctEncode` 并返回 `usize`。 |
| `utTruncateUtf8` | `(val: &[]u8, maxChars: usize): &[]u8` | 调用 `utTruncateUtf8` 并返回 `&[]u8`。 |
| `utWriteSlice` | `(out: []u8, pos: &mut usize, src: &[]u8)` | 就地执行 `utWriteSlice` 操作。 |
| `utWriteByte` | `(out: []u8, pos: &mut usize, b: u8)` | 就地执行 `utWriteByte` 操作。 |
| `expand` | `(tmpl: string, vars: &[]TemplateVar, out: []u8): usize` | 调用 `expand` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isValid } from "url-template";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
