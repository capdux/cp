# `cap:sql_ast` (`cap.sql_ast`) — SQL 词法分词、AST 解析与安全脱敏标准库

## 1. 概述

`cap:sql_ast` 是纯 CP 实现的工业级 SQL 解析与安全分析标准库，提供零外部依赖的：
- **SQL 词法分词器**：支持单引号字符串（`''` 转义）、双引号/反引号标识符、整数与小数字面量、`?` / `$1` 占位符、`--` 与 `/* ... */` 注释、多字符操作符（`<=`, `>=`, `<>`, `!=`, `||`, `::`），并严格拦截未闭合字符串、未闭合块注释及非法字符。
- **递归下降 DML/DDL AST 解析器**：支持完整运算符优先级的表达式树（`OR` < `AND` < 比较/`LIKE`/`IN`/`IS NULL` < `+`/`-`/`||` < `*`/`/`/`%` < 一元运算），以及 `SELECT`、`INSERT`、`UPDATE`、`DELETE`、`CREATE TABLE` 语句解析，内置 `max_depth = 32` 防栈溢出保护。
- **安全校验与指纹脱敏**：提供慢查询指纹规范化（`normalizeSqlFingerprint`）、关联表名提取（`extractTableNames`）以及防多语句堆叠注入的只读查询校验（`isReadOnlyQuery`）。

## 2. 核心类型

- `SqlError`：词法与语法错误联合（`UnterminatedString`, `UnterminatedComment`, `InvalidCharacter`, `InvalidNumber`, `UnexpectedToken`, `UnexpectedEof`, `MaxDepthExceeded`, `OutOfMemory`）。
- `SqlTokenKind` / `SqlToken`：词法 Token 类型与结构（`kind`, `lexeme`, `line`, `col`）。
- `SqlExprKind` / `SqlExpr`：表达式 AST 节点（`Literal`, `ColumnRef`, `Placeholder`, `UnaryOp`, `BinaryOp`, `FuncCall`, `IsNull`, `InList`, `Paren`），提供 `deinit(alloc)` 递归释放。
- `SqlStmtKind` / `SqlStmt`：语句 AST 顶层包装，包含 `SelectStmt`、`InsertStmt`、`UpdateStmt`、`DeleteStmt`、`CreateTableStmt`，提供 `deinit(alloc)` 统一释放。

## 3. API 速查

| 函数签名 | 说明 |
| --- | --- |
| `tokenizeSql(alloc, sql): ![]SqlToken` | 将 SQL 文本分词为 Token 切片（需 `alloc.free(tokens)` 释放） |
| `parseSql(alloc, sql): !SqlStmt` | 解析单条 DML/DDL SQL 为 AST（默认 `max_depth = 32`，需 `stmt.deinit(alloc)` 释放） |
| `parseSqlWithMaxDepth(alloc, sql, max_depth): !SqlStmt` | 指定最大递归深度解析 SQL |
| `normalizeSqlFingerprint(alloc, sql): !string` | 将字面量与占位符脱敏为 `?`，规整空白与关键字大小写 |
| `extractTableNames(alloc, sql): ![]string` | 提取 SQL 中引用的去重表名列表（需 `alloc.free(tables)` 释放） |
| `isReadOnlyQuery(sql): bool` | 严格判定是否为单条只读 `SELECT`（拒绝 `; DROP ...` 等堆叠注入） |

## 4. 示例

```cp
import { parseSql, normalizeSqlFingerprint, isReadOnlyQuery } from "cap:sql_ast";

const sql = "SELECT id, name FROM users WHERE age >= 18 AND status = 'active'";
if (isReadOnlyQuery(sql)) {
    let stmt = try parseSql(alloc, sql);
    defer stmt.deinit(alloc);

    const fp = try normalizeSqlFingerprint(alloc, sql);
    defer alloc.free(fp);
    // fp => "SELECT id, name FROM users WHERE age >= ? AND status = ?"
}
```
