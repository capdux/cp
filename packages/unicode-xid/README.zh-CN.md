[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-xid

Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classification and identifier scanner for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-xid` ([https://github.com/unicode-rs/unicode-xid](https://github.com/unicode-rs/unicode-xid))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-xid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-xid
```

```cp
import {
    UxidRune,
    uxidDecodeUtf8Strict,
    uxidIsExtraStart,
    uxidBsearchStart,
    uxidBsearchCont,
    uxidIsAsciiAlpha,
    uxidIsAsciiDigit,
    isXidStart,
} from "unicode-xid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UxidRune` | `class` | 字段: `cp: u32, len: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uxidDecodeUtf8Strict` | `(s: string, offset: usize): UxidRune` | 调用 `uxidDecodeUtf8Strict` 并返回 `UxidRune`。 |
| `uxidIsExtraStart` | `(cp: u32): bool` | 调用 `uxidIsExtraStart` 并返回 `bool`。 |
| `uxidBsearchStart` | `(cp: u32): bool` | 调用 `uxidBsearchStart` 并返回 `bool`。 |
| `uxidBsearchCont` | `(cp: u32): bool` | 调用 `uxidBsearchCont` 并返回 `bool`。 |
| `uxidIsAsciiAlpha` | `(b: u8): bool` | 调用 `uxidIsAsciiAlpha` 并返回 `bool`。 |
| `uxidIsAsciiDigit` | `(b: u8): bool` | 调用 `uxidIsAsciiDigit` 并返回 `bool`。 |
| `isXidStart` | `(cp: u32): bool` | 执行 `isXidStart` 条件判定，返回 `bool`。 |
| `isXidContinue` | `(cp: u32): bool` | 执行 `isXidContinue` 条件判定，返回 `bool`。 |
| `isIdentStart` | `(cp: u32): bool` | 执行 `isIdentStart` 条件判定，返回 `bool`。 |
| `isAsciiIdent` | `(s: string): bool` | 执行 `isAsciiIdent` 条件判定，返回 `bool`。 |
| `isValidIdent` | `(s: string): bool` | 执行 `isValidIdent` 条件判定，返回 `bool`。 |
| `scanIdentLen` | `(s: string, start: usize): usize` | 调用 `scanIdentLen` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isAsciiIdent } from "unicode-xid";

pub fn main() {
    let result = isAsciiIdent("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
