# `cap:cypher` — 纯 CP openCypher / Neo4j 图查询零分配词法扫描与只读安全判定库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 概述

`cap:cypher` 提供纯 CP、零外部依赖、零堆分配的 openCypher / Neo4j 图查询语言词法扫描器、子句关键字分类器、`$param` 参数迭代器、括号深度安全校验器与只读查询判定器，专为 AI Agent 知识图谱检索与安全护栏场景设计。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `CYPHER_MAX_DEPTH` | 定界符最大允许嵌套深度常量（`64`） |
| `CypherTokenKind` | 词法记号枚举：`ident`, `param`, `string`, `backtick_ident`, `number`, `arrow_right` (`->`), `arrow_left` (`<-`), `lparen`, `rparen`, `lbracket`, `rbracket`, `lbrace`, `rbrace`, `colon`, `comma`, `dot`, `punct`, `invalid` |
| `CypherClauseKind` | 子句关键字分类枚举：`read`, `write`, `schema`, `call`, `other` |
| `CypherToken` | 零拷贝词法记号：`kind: CypherTokenKind`, `lexeme: []const u8` |
| `CypherIter` | 流式词法扫描器：`init(input)`, `next(*_): CypherToken?`；自动跳过 `//` 与 `/* */` 注释，识别单/双引号转义字符串、反引号转义标识符（支持 `` `` ``）、`$param`、关系箭头 `->`/`<-`、多进制/浮点/科学计数数字及变长路径 `..` |
| `CypherParamIter` | 参数名流式提取器：`init(query)`, `next(*_): ([]const u8)?`；提取所有 `$param` 参数名（不含 `$` 前缀），自动忽略字符串、注释与反引号内文本 |
| `classifyCypherKeyword(word)` | 大小写不敏感分类 openCypher 关键字为 `read` / `write` / `schema` / `call` / `other` |
| `validateCypherDelimiters(query)` | 字符串/反引号/注释感知校验 `()`、`[]`、`{}` 平衡性、词法闭合性及 64 层嵌套深度上限 |
| `isReadOnlyCypher(query)` | 判定查询是否为语法合法的只读图查询：拦截顶层及 `CALL { ... }` 子查询中的 `CREATE`/`MERGE`/`DELETE`/`DETACH`/`SET`/`REMOVE`/`DROP`/`FOREACH` 等写/架构变更关键字，同时避免误判字符串、注释、反引号标识符、属性访问（`n.delete`）及 Map 键（`{ set: 1 }`） |

## 示例

```cp
import { isReadOnlyCypher, validateCypherDelimiters, CypherParamIter, CypherIter } from "cap:compiler";

fn checkGraphQuery(query: []const u8): bool {
    if (!validateCypherDelimiters(query)) return false;
    if (!isReadOnlyCypher(query)) return false;

    let params = CypherParamIter.init(query);
    let count: usize = 0;
    while (p := params.next()) {
        if (p.len > 0) count += 1;
    }
    return count > 0;
}
```
