# `cap:sparql` — 纯 CP W3C SPARQL 1.1 零分配词法扫描与查询分析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 概述

`cap:sparql` 提供纯 CP、零外部依赖、零堆分配的 W3C SPARQL 1.1 知识图谱查询语言词法扫描器（支持 `#` 行注释、`<iri>` IRI 引用、`?var`/`$var` 变量、单/双引号及三引号转义字符串、数值与运算符）、`PREFIX alias: <iri>` 前缀声明流式提取器、查询/更新形式判定器、只读安全守卫与 64 层深度上限的分隔符平衡校验器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SparqlQueryForm` | 查询形式枚举：`select`, `ask`, `construct`, `describe`, `update`, `unknown` |
| `SparqlPrefix` | 前缀声明元数据：`alias`（不含 `:`，可为空表示默认前缀）、`iri`（不含 `<`/`>`） |
| `SparqlTokenKind` | 词法单元类型：`ident`, `var_name`, `iri`, `string`, `number`, `lbrace`, `rbrace`, `lparen`, `rparen`, `lbracket`, `rbracket`, `dot`, `semicolon`, `comma`, `punct`, `invalid` |
| `SparqlToken` | 零拷贝词法单元：`kind`, `lexeme` |
| `SparqlIter` | 流式词法扫描器：`init(input)`、`next(*_): SparqlToken?` |
| `SparqlPrefixIter` | 前缀声明提取器：`init(query)`、`next(*_): SparqlPrefix?`（大小写不敏感） |
| `detectSparqlQueryForm(query)` | 跳过前导 `BASE`/`PREFIX` 后判定 `SELECT`/`ASK`/`CONSTRUCT`/`DESCRIBE` 或 `INSERT`/`DELETE`/`LOAD`/`CLEAR`/`DROP`/`CREATE` 等更新形式 |
| `isReadOnlySparql(query)` | 判定查询是否为严格只读（拒绝更新语句、未闭合词法与顶层多语句写拼接） |
| `validateSparqlDelimiters(query)` | 字符串/IRI/注释感知校验 `{}`/`()`/`[]` 平衡与 64 层嵌套深度上限 |

## 示例

```cp
import {
    SparqlPrefixIter,
    detectSparqlQueryForm,
    isReadOnlySparql,
    validateSparqlDelimiters,
} from "cap:compiler";

fn checkQuery(query: []const u8): bool {
    if (!validateSparqlDelimiters(query)) return false;
    if (!isReadOnlySparql(query)) return false;
    let prefixes = SparqlPrefixIter.init(query);
    return prefixes.next() != null;
}
```
