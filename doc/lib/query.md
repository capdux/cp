# CP 标准库：防注入参数化 SQL 查询构建器 (`cap:query` / `cap.query`)

## 一、 架构总览与设计准则

`cap:query` 是 CP 语言官方内置的纯原生防注入参数化 SQL 查询构建器标准库，专为 PostgreSQL、MySQL 与 SQLite3 的安全动态 SQL 生成和标识符转义打造。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，严格遵循“结构（SQL 模板）与数据（参数列表）物理分离”的安全契约：

- **参数值绝不拼接**：所有动态值统一封装为强类型 `QueryParam` 并仅收集至 `BuiltQuery.params`，SQL 文本中只生成方言匹配的占位符（PostgreSQL `$1..$N`，MySQL/SQLite `?`）。
- **标识符防注入转义**：`IdentCodec.quote` / `appendQuoted` 支持 `table.col` 点分分段转义，自动拦截空段、`\0` 及控制字符、`;` 分号、`--` / `/*` / `*/` 注释标记，并将内嵌引号字符加倍转义（`""` / `` `` ``）。
- **所有权安全**：Builder 内部对传入标识符与 `Text`/`Bytes` 参数做独立深拷贝，生成的 `BuiltQuery` 同样持有独立副本，调用 `deinit()` 确定性释放全部资源。
- **双通道接入**：支持 `import ... from "cap:query"` 与 `import cap from "cap"; cap.query.*` 自动动态挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/query/types.cp` | `// Source: cp/lib/query/types.cp (SQL Dialect, Parameter Values & Safe Identifier Quoting)` | `QueryError`、`QueryDialect`、`QueryParam`、`BuiltQuery`、`IdentCodec` |
| `lib/query/builder.cp` | `// Source: cp/lib/query/builder.cp (Fluent Parameterized SELECT / INSERT / UPDATE / DELETE Builders)` | `SelectBuilder`、`InsertBuilder`、`UpdateBuilder`、`DeleteBuilder` |
| `lib/query/index.cp` | `// Source: cp/lib/query/index.cp (Unified Exports for cap:query)` | `cap:query` 与 `cap.query` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 基础类型与标识符编解码（`types.cp`）

- **`QueryError`**：`InvalidIdentifier`、`EmptyTable`、`EmptyValues`、`OutOfMemory`
- **`QueryDialect`**：
  - `Postgres`：1 起始编号占位符 `$1, $2, ...`，双引号标识符 `"col"`
  - `MySql`：位置占位符 `?`，反引号标识符 `` `col` ``
  - `Sqlite`：位置占位符 `?`，双引号标识符 `"col"`
  - 辅助方法：`quoteChar(): u8`、`usesNumberedPlaceholders(): bool`
- **`QueryParam`**：`Null`、`Bool: bool`、`Int: i64`、`Float: f64`、`Text: string`、`Bytes: []const u8`
  - 提供 `isNull()`、`asBool()`、`asInt()`、`asFloat()`、`asText()`、`asBytes()`、`clone(alloc)`、`deinit(alloc)`、`eql(other)`
- **`BuiltQuery`**：`sql: string`、`params: []QueryParam`、`deinit(*_)`
- **`IdentCodec`**：
  - `quote(alloc, dialect, ident): ([]u8) ! QueryError`：支持 `table.col` 点分分段加引号，拦截异常控制字符/分号/注释标记，并将内嵌引号加倍转义。

### 2. 流式参数化构建器（`builder.cp`）

- **`SelectBuilder`**：
  - `init(alloc, dialect, table)`、`deinit(*_)`
  - 投影与去重：`columns(cols)`（支持 `*` 与 `alias.*`，默认 `*`）、`distinct()`
  - 关联与过滤：`join(table, on_left, on_right)`、`leftJoin(table, on_left, on_right)`、`whereEq(col, val)`、`whereOp(col, op, val)`（仅允许安全比较/正则操作符白名单）、`whereIn(col, vals)`、`whereNull(col)`、`whereNotNull(col)`、`orWhereEq(col, val)`
  - 分组排序分页：`groupBy(col)`、`orderByAsc(col)`、`orderByDesc(col)`、`limit(n)`、`offset(n)`
  - 生成：`build(): BuiltQuery ! QueryError`
- **`InsertBuilder`**：
  - `init(alloc, dialect, table)`、`deinit(*_)`、`columns(cols)`、`addRow(vals)`（支持多行批量 `VALUES (...), (...)`）、`returning(cols)`、`build(): BuiltQuery ! QueryError`
- **`UpdateBuilder`**：
  - `init(alloc, dialect, table)`、`deinit(*_)`、`set(col, val)`、`whereEq(col, val)`、`whereOp(col, op, val)`、`whereIn(col, vals)`、`whereNull(col)`、`whereNotNull(col)`、`returning(cols)`、`build(): BuiltQuery ! QueryError`
- **`DeleteBuilder`**：
  - `init(alloc, dialect, table)`、`deinit(*_)`、`whereEq(col, val)`、`whereOp(col, op, val)`、`whereIn(col, vals)`、`whereNull(col)`、`whereNotNull(col)`、`returning(cols)`、`build(): BuiltQuery ! QueryError`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    QueryDialect,
    QueryParam,
    SelectBuilder,
    InsertBuilder,
    UpdateBuilder,
    DeleteBuilder,
} from "cap:query";

pub fn demoQuery(): !void {
    const alloc = std.heap.page_allocator;

    // 1. PostgreSQL 流式 SELECT 构建
    let sb = SelectBuilder.init(alloc, .Postgres, "users");
    defer sb.deinit();
    const cols: [3]string = ["users.id", "users.email", "profiles.bio"];
    const statuses: [2]QueryParam = [.Text("active"), .Text("trial")];
    let q = sb.columns(&cols)
        .distinct()
        .leftJoin("profiles", "users.id", "profiles.user_id")
        .whereEq("users.tenant_id", .Int(42))
        .whereIn("users.status", &statuses)
        .whereNotNull("users.verified_at")
        .orderByDesc("users.created_at")
        .limit(20)
        .offset(0)
        .build()?;
    defer q.deinit();
    // q.sql => SELECT DISTINCT "users"."id", "users"."email", "profiles"."bio" FROM "users" LEFT JOIN "profiles" ON "users"."id" = "profiles"."user_id" WHERE "users"."tenant_id" = $1 AND "users"."status" IN ($2, $3) AND "users"."verified_at" IS NOT NULL ORDER BY "users"."created_at" DESC LIMIT 20 OFFSET 0
    // q.params => [.Int(42), .Text("active"), .Text("trial")]

    // 2. cap.query 动态挂载批量 INSERT
    let ib = cap.query.InsertBuilder.init(alloc, .Postgres, "events");
    defer ib.deinit();
    const ev_cols: [2]string = ["kind", "payload"];
    const r1: [2]QueryParam = [.Text("login"), .Bytes("{\"ok\":true}")];
    const ret: [1]string = ["id"];
    let ins = ib.columns(&ev_cols).addRow(&r1).returning(&ret).build()?;
    defer ins.deinit();
}
```
