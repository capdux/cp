[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cjs-module-lexer

Zero-allocation CommonJS static named export and re-export lexer (npm cjs-module-lexer parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cjs-module-lexer` ([https://github.com/nodejs/cjs-module-lexer](https://github.com/nodejs/cjs-module-lexer))（原库名 `cjs-module-lexer` → CP 包名 `cjs-module-lexer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `cjs-module-lexer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cjs-module-lexer
```

```cp
import {
    CjsParseResult,
    CjslLexer,
    CjslStrRes,
    cjslIsWs,
    cjslIsIdentStart,
    cjslIsIdentChar,
    cjslStrEq,
    cjslAddExport,
} from "cjs-module-lexer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CjsParseResult` | `class` | 字段: `exports: [32]string, export_count: usize, reexports: [16]string, reexport_count: usize` |
| `CjslLexer` | `class` | 字段: `src: string, pos: usize` |
| `CjslStrRes` | `class` | 字段: `ok: bool, val: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cjslIsWs` | `(ch: u8): bool` | 调用 `cjslIsWs` 并返回 `bool`。 |
| `cjslIsIdentStart` | `(ch: u8): bool` | 调用 `cjslIsIdentStart` 并返回 `bool`。 |
| `cjslIsIdentChar` | `(ch: u8): bool` | 调用 `cjslIsIdentChar` 并返回 `bool`。 |
| `cjslStrEq` | `(a: string, b: string): bool` | 调用 `cjslStrEq` 并返回 `bool`。 |
| `cjslAddExport` | `(res: &mut CjsParseResult, name: string)` | 就地执行 `cjslAddExport` 操作。 |
| `cjslAddReexport` | `(res: &mut CjsParseResult, spec: string)` | 就地执行 `cjslAddReexport` 操作。 |
| `cjslSkipString` | `(lex: &mut CjslLexer)` | 就地执行 `cjslSkipString` 操作。 |
| `cjslSkipWsAndComments` | `(lex: &mut CjslLexer)` | 就地执行 `cjslSkipWsAndComments` 操作。 |
| `cjslReadIdent` | `(lex: &mut CjslLexer): string` | 调用 `cjslReadIdent` 并返回 `string`。 |
| `cjslReadStringLit` | `(lex: &mut CjslLexer): CjslStrRes` | 调用 `cjslReadStringLit` 并返回 `CjslStrRes`。 |
| `cjslMatchRequireCall` | `(lex: &mut CjslLexer): CjslStrRes` | 调用 `cjslMatchRequireCall` 并返回 `CjslStrRes`。 |
| `cjslSkipPropValue` | `(lex: &mut CjslLexer)` | 就地执行 `cjslSkipPropValue` 操作。 |
| `cjslParseObjectExports` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | 就地执行 `cjslParseObjectExports` 操作。 |
| `cjslParseExportsMemberAssign` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | 就地执行 `cjslParseExportsMemberAssign` 操作。 |
| `cjslParseDefineProperty` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | 就地执行 `cjslParseDefineProperty` 操作。 |
| `cjslParseExportStar` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | 就地执行 `cjslParseExportStar` 操作。 |
| `parseCjs` | `(src: string): CjsParseResult` | 通过 `parseCjs` 解析输入数据，返回 `CjsParseResult`。 |

## 最小调用示例

```cp
import { parseCjs } from "cjs-module-lexer";

pub fn main() {
    let result = parseCjs("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
