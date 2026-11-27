[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# spdx

Zero-allocation SPDX 2.3 license expression parser, validator, and compatibility evaluator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`spdx`](https://github.com/EmbarkStudios/spdx)（`MIT OR Apache-2.0`）（包名 `spdx`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `spdx` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add spdx
```

```cp
import {
    LicenseCategory,
    EvalMode,
    LicenseInfo,
    SpdxResult,
    Parser,
    isIdChar,
    isPermissiveCategory,
    lookupBaseLicense,
} from "spdx";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `LicenseCategory` | `Permissive, WeakCopyleft, StrongCopyleft, NetworkCopyleft, PublicDomain, Unknown` |
| `enum` | `EvalMode` | `ValidateOnly, PermissiveOnly, Satisfies` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LicenseInfo` | `class` | 字段: `id: string, category: LicenseCategory, isOsiApproved: bool` |
| `SpdxResult` | `class` | 字段: `valid: bool, allKnown: bool, errorOffset: usize, licenses: [MAX_LICENSES]string, licenseCount: usize` · 方法: `getLicense` |
| `Parser` | `class` | 字段: `src: string, pos: usize, depth: usize, failed: bool, errorOffset: usize, allKnown: bool, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isIdChar` | `(c: u8): bool` | 执行 `isIdChar` 条件判定，返回 `bool`。 |
| `isPermissiveCategory` | `(cat: LicenseCategory): bool` | 执行 `isPermissiveCategory` 条件判定，返回 `bool`。 |
| `lookupBaseLicense` | `(id: string): LicenseInfo?` | 查询或读取目标数据，返回 `LicenseInfo?`。 |
| `lookupLicense` | `(id: string): LicenseInfo?` | 查询或读取目标数据，返回 `LicenseInfo?`。 |
| `initParser` | `(src: string, mode: EvalMode, allowed: &[]string): Parser` | 创建或初始化实例，返回 `Parser`。 |
| `skipSpaces` | `(p: &mut Parser)` | 就地执行 `skipSpaces` 操作。 |
| `failAt` | `(p: &mut Parser, offset: usize): bool` | 调用 `failAt` 并返回 `bool`。 |
| `isKeyword` | `(tok: string): bool` | 执行 `isKeyword` 条件判定，返回 `bool`。 |
| `peekKeyword` | `(p: &mut Parser, kw: string): bool` | 调用 `peekKeyword` 并返回 `bool`。 |
| `consumeKeyword` | `(p: &mut Parser, kw: string): bool` | 调用 `consumeKeyword` 并返回 `bool`。 |
| `recordLicense` | `(p: &mut Parser, lic: string)` | 就地执行 `recordLicense` 操作。 |
| `isValidRefId` | `(s: string): bool` | 执行 `isValidRefId` 条件判定，返回 `bool`。 |
| `licenseTokenMatches` | `(licBase: string, hasPlus: bool, allowedLic: string): bool` | 调用 `licenseTokenMatches` 并返回 `bool`。 |
| `matchAllowedEntry` | `(licBase: string, hasPlus: bool, hasExc: bool, exc: string, entry: string): bool` | 调用 `matchAllowedEntry` 并返回 `bool`。 |
| `evalSimple` | `(p: &mut Parser, licBase: string, hasPlus: bool, hasExc: bool, exc: string, fullLic: string): bool` | 调用 `evalSimple` 并返回 `bool`。 |
| `parseSimple` | `(p: &mut Parser): bool` | 通过 `parseSimple` 解析输入数据，返回 `bool`。 |
| `parsePrimary` | `(p: &mut Parser): bool` | 通过 `parsePrimary` 解析输入数据，返回 `bool`。 |
| `parseAnd` | `(p: &mut Parser): bool` | 通过 `parseAnd` 解析输入数据，返回 `bool`。 |
| `parseOr` | `(p: &mut Parser): bool` | 通过 `parseOr` 解析输入数据，返回 `bool`。 |
| `runParse` | `(p: &mut Parser): bool` | 调用 `runParse` 并返回 `bool`。 |
| `validateExpression` | `(expr: string): SpdxResult` | 校验输入合法性，返回 `SpdxResult`。 |
| `isPermissiveOnly` | `(expr: string): bool` | 执行 `isPermissiveOnly` 条件判定，返回 `bool`。 |
| `satisfies` | `(expr: string, allowed: &[]string): bool` | 调用 `satisfies` 并返回 `bool`。 |

## 最小调用示例

```cp
import { isKeyword } from "spdx";

pub fn main() {
    let result = isKeyword("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
