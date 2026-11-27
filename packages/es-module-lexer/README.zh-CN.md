[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# es-module-lexer

Zero-allocation ES module static/dynamic import and export lexer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:es-module-lexer` ([https://github.com/guybedford/es-module-lexer](https://github.com/guybedford/es-module-lexer))

| 元信息 | 值 |
| --- | --- |
| 包名 | `es-module-lexer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add es-module-lexer
```

```cp
import {
    ImportKind,
    ImportSpecifier,
    ExportSpecifier,
    EsmParseResult,
    EsmlSpan,
    esmlIsWs,
    esmlIsIdentStart,
    esmlIsIdentPart,
} from "es-module-lexer";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ImportKind` | `Static, Dynamic, ImportMeta` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ImportSpecifier` | `class` | 字段: `kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize` · 方法: `specifier` |
| `ExportSpecifier` | `class` | 字段: `name_start: usize, name_end: usize, local_start: usize, local_end: usize` · 方法: `name, localName` |
| `EsmParseResult` | `class` | 字段: `import_count: usize, export_count: usize, facade: bool, has_module_syntax: bool` |
| `EsmlSpan` | `class` | 字段: `start: usize, end: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `esmlIsWs` | `(c: u8): bool` | 调用 `esmlIsWs` 并返回 `bool`。 |
| `esmlIsIdentStart` | `(c: u8): bool` | 调用 `esmlIsIdentStart` 并返回 `bool`。 |
| `esmlIsIdentPart` | `(c: u8): bool` | 调用 `esmlIsIdentPart` 并返回 `bool`。 |
| `esmlStrEq` | `(a: string, b: string): bool` | 调用 `esmlStrEq` 并返回 `bool`。 |
| `esmlSkipWsAndComments` | `(src: string, start: usize): usize` | 调用 `esmlSkipWsAndComments` 并返回 `usize`。 |
| `esmlScanString` | `(src: string, start: usize): EsmlSpan` | 调用 `esmlScanString` 并返回 `EsmlSpan`。 |
| `esmlSkipTemplate` | `(src: string, start: usize): usize` | 调用 `esmlSkipTemplate` 并返回 `usize`。 |
| `esmlScanIdent` | `(src: string, start: usize): EsmlSpan` | 调用 `esmlScanIdent` 并返回 `EsmlSpan`。 |
| `esmlPrevNonWsChar` | `(src: string, pos: usize): u8` | 调用 `esmlPrevNonWsChar` 并返回 `u8`。 |
| `esmlPushImport` | `(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize` | 调用 `esmlPushImport` 并返回 `usize`。 |
| `esmlPushExport` | `(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize` | 调用 `esmlPushExport` 并返回 `usize`。 |
| `parseEsm` | `(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult` | 通过 `parseEsm` 解析输入数据，返回 `EsmParseResult`。 |
| `isFacade` | `(src: string): bool` | 执行 `isFacade` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseEsm } from "es-module-lexer";

pub fn main() {
    let result = parseEsm("example", /* imports_out: []ImportSpecifier */, /* exports_out: []ExportSpecifier */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
