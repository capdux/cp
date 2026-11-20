# CP 标准库：确定性数据库迁移与防篡改校验引擎手册 (`cap:migrate`)

## 一、 架构总览与设计准则

`cap:migrate` 是 CP 语言官方内置的纯 CP **确定性 Schema/数据库迁移规划与防篡改校验引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。它基于 `cap:crypto` 的 SHA-256 为迁移脚本提供版本确定性排序、重复拦截、双段 SQL 脚本解析、防事后篡改校验和比对、`planUp`/`planDown` 执行计划生成，并内置基于 `cap:sqlite` 的事务级 `SqliteMigrator` 执行器。既可通过 `import { ... } from "cap:migrate"` 导入，也支持 `import cap from "cap"; cap.migrate.*` 动态懒挂载调用。

### 1. 规范 SHA-256 校验和 (`CP-MIGRATE-V1`)

每条迁移脚本 `MigrationSpec` 的 `checksum` 均按无歧义规范编码计算 SHA-256（64 位小写十六进制）：
- 域分隔前缀：`"CP-MIGRATE-V1\x00"`
- 8 字节大端序 `version`
- 8 字节大端序长度前缀 + 原始字节：`name`、规范化后的 `up_sql`（统一 `\n` 并去除首尾空白）、规范化后的 `down_sql`

已应用到数据库的迁移脚本一旦在本地被修改（或被删除），`verify` / `planUp` / `planDown` / `SqliteMigrator` 将立即拦截并抛出 `ChecksumMismatch` 或 `MissingAppliedMigration`。

### 2. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/migrate/migrator.cp` | `// Source: cp/lib/migrate/migrator.cp (...)` | `MigrateError`、`MigrationDirection`、`MigrationStatus`、`MigrationSpec`、`AppliedMigration`、`MigrationPlanItem`、`MigrationStatusEntry`、`MigrationRegistry`、`SqliteMigrator` |
| `lib/migrate/index.cp` | `// Source: cp/lib/migrate/index.cp (Unified Exports for cap:migrate)` | `cap:migrate` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与枚举类型

- **`MigrateError`**：
  - `InvalidVersion`：版本号为 `0`。
  - `DuplicateVersion`：注册了重复的版本号。
  - `ChecksumMismatch`：已应用迁移脚本的本地 SHA-256 与数据库记录不一致（疑似事后篡改）。
  - `MissingAppliedMigration`：已应用记录在本地注册表中缺失对应脚本。
  - `NonTransactionalNotSupported`：遇到 `-- +migrate NO TRANSACTION` 等非事务指令。
  - `InvalidMigrationFormat`：脚本缺少 `Up` 段、存在重复段标记或名称/内容为空。
  - `SqlExecutionFailed`：SQLite 执行或事务提交失败。
  - `OutOfMemory`：内存分配失败。
- **`MigrationDirection`**：`Up`、`Down`（提供 `toString()`）。
- **`MigrationStatus`**：`Pending`、`Applied`、`DriftDetected`（提供 `toString()`）。

### 2. 数据结构

- **`MigrationSpec`**：`version: u64`、`name: string`、`up_sql: string`、`down_sql: string`、`checksum: string`。
- **`AppliedMigration`**：`version: u64`、`name: string`、`checksum: string`、`applied_at_ms: u64`。
- **`MigrationPlanItem`**：`version: u64`、`name: string`、`direction: MigrationDirection`、`sql: string`、`checksum: string`。
- **`MigrationStatusEntry`**：`version: u64`、`name: string`、`status: MigrationStatus`、`local_checksum: string`、`applied_checksum: string`。

### 3. `MigrationRegistry` 核心方法

- **`MigrationRegistry.init(alloc)` / `deinit()`**：初始化与确定性释放所有内部托管字符串与列表。
- **`register(version, name, up_sql, down_sql): MigrationSpec ! MigrateError`**：校验 `version > 0` 与唯一性，按 `version` 升序插入并计算 SHA-256 `checksum`。
- **`parseSqlFile(version, name, content): MigrationSpec ! MigrateError`**：解析 `-- +migrate Up` / `-- +migrate Down`（或 `-- migrate:up` / `-- migrate:down`）单文件双段格式并注册。
- **`verify(applied: []const AppliedMigration): !MigrateError`**：校验已应用记录的完整性与未篡改性。
- **`planUp(alloc, applied): []MigrationPlanItem ! MigrateError`**：校验后按 `version` 升序返回所有未应用的 `Pending` 迁移计划。
- **`planDown(alloc, applied, steps): []MigrationPlanItem ! MigrateError`**：校验后按 `version` 降序返回最近 `steps` 步回滚计划。
- **`statusReport(alloc, applied): []MigrationStatusEntry ! MigrateError`**：输出每个版本的 `Pending` / `Applied` / `DriftDetected` 状态列表。

### 4. `SqliteMigrator` 闭环执行器（基于 `cap:sqlite`）

- **`ensureTable(db): !MigrateError`**：自动创建 `_cap_migrations(version INTEGER PRIMARY KEY, name TEXT NOT NULL, checksum TEXT NOT NULL, applied_at_ms INTEGER NOT NULL)`。
- **`loadApplied(alloc, db): []AppliedMigration ! MigrateError`**：从 `_cap_migrations` 读取已应用记录（配套 `freeApplied(alloc, items)` 释放）。
- **`migrateUp(alloc, db, registry, now_ms): usize ! MigrateError`**：在 SQLite 事务中按序执行全部 `Pending` 迁移并写入 `_cap_migrations`，返回本次应用的迁移数。
- **`rollback(alloc, db, registry, steps): usize ! MigrateError`**：在 SQLite 事务中按逆序执行 `down_sql` 并删除对应 `_cap_migrations` 记录，返回本次回滚的步数。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Database } from "cap:sqlite";
import { MigrationRegistry, SqliteMigrator } from "cap:migrate";
import cap from "cap";

pub fn runMigrations(): ! {
    const alloc = std.heap.page_allocator;
    let reg = cap.migrate.MigrationRegistry.init(alloc);
    defer reg.deinit();

    * = reg.parseSqlFile(1, "init_users",
        "-- +migrate Up\nCREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);\n" ++
        "-- +migrate Down\nDROP TABLE users;"
    )?;

    const db = new Database(":memory:");
    const applied_count = SqliteMigrator.migrateUp(alloc, db, reg, 1700000000000)?;
    * = applied_count;
}
```
