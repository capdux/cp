# `cap:sql_splitter` — 多语句 SQL 脚本分号切分与语句分类库

> 来源：吸收自 `/data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig`（Multi-Statement SQL Script Splitter）

`cap:sql_splitter` 提供纯 CP、零外部依赖、零堆分配的多语句 SQL 脚本顶层分号切分器、语句类型识别器（含 `WITH` CTE 主语句识别）与只读查询判定工具。

## 核心类型与函数

### `SqlStmtKind`

```cp
pub enum SqlStmtKind: u8 {
    select = 0,
    insert = 1,
    update = 2,
    delete = 3,
    create = 4,
    drop = 5,
    alter = 6,
    begin = 7,
    commit = 8,
    rollback = 9,
    pragma = 10,
    other = 11,

    fn asStr(): string
}
```

### `SqlStmtIter`

```cp
pub struct SqlStmtIter {
    static fn init(script: []const u8): SqlStmtIter
    fn next(*_): ([]const u8)?
}
```

- 自动剥离 UTF-8 BOM，按顶层 `;` 零拷贝切分语句。
- 词法安全跳过 `'...'`（含 `''`）、`"..."`（含 `""`）、`` `...` ``、`-- ...\n`、嵌套 `/* ... */` 与 `$$...$$`/`$tag$...$tag$` 内部的分号。
- 自动裁剪首尾空白，跳过空语句与纯注释块。

### 分类与统计函数

```cp
pub fn classifySqlStmt(stmt: []const u8): SqlStmtKind
pub fn isReadOnlySql(stmt: []const u8): bool
pub fn countSqlStatements(script: []const u8): usize
```

- `classifySqlStmt`：跳过前导空白与注释后大小写不敏感识别主关键字；支持 `WITH ... AS (...) SELECT/INSERT/UPDATE/DELETE` CTE 穿透识别。
- `isReadOnlySql`：判定 `SELECT` / `EXPLAIN` / `SHOW` / `VALUES` 与只读 `WITH ... SELECT`。

## 使用示例

```cp
import { SqlStmtIter, classifySqlStmt, isReadOnlySql } from "cap:sql_splitter";

const script = "BEGIN; INSERT INTO t VALUES ('a;b'); COMMIT;";
let it = SqlStmtIter.init(script);
while (stmt := it.next()) {
    const kind = classifySqlStmt(stmt);
}
```
