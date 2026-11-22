# `cap:xpath` — 纯 CP W3C XPath 零分配词法扫描与路径步进解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:web`，导入请使用 `import { ... } from "cap:web"`。

## 概述

`cap:xpath` 提供纯 CP、零外部依赖、零堆分配的 W3C XPath 表达式词法扫描器（支持单/双引号字符串、`$var` 变量引用、`//`/`..`/`::`/`!=`/`<=`/`>=` 多字符记号、XML 命名空间前缀 `ns:tag` / `ns:*` 与数字）、位置路径步进迭代器（自动跳过谓词内部 `/` 与字符串）及 64 层深度上限的括号平衡校验器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `XPathTokenKind` | 词法单元类型：`slash`, `dslash`, `at`, `axis`, `dot`, `ddot`, `star`, `pipe`, `lbracket`, `rbracket`, `lparen`, `rparen`, `comma`, `op`, `ident`, `string`, `number`, `var_ref`, `invalid` |
| `XPathToken` | 零拷贝词法单元：`kind`, `lexeme` |
| `XPathIter` | 流式词法扫描器：`init(expr)`、`next(*_): XPathToken?` |
| `XPathStep` | 位置路径步进：`is_descendant: bool`, `axis: ([]const u8)?`, `node_test: []const u8`, `predicate: ([]const u8)?` |
| `XPathStepIter` | 位置路径步进迭代器：`init(expr)`、`next(*_): XPathStep?` |
| `validateXPathDelimiters(expr)` | 字符串感知校验 `[]`/`()` 平衡、无未闭合引号与 64 层嵌套深度上限 |

## 示例

```cp
import { XPathStepIter, validateXPathDelimiters } from "cap:web";

fn inspectPath(expr: []const u8): bool {
    if (!validateXPathDelimiters(expr)) return false;
    let steps = XPathStepIter.init(expr);
    return steps.next() != null;
}
```
