# CP 标准库：统一多方言 SQL 协议引擎与嵌入式 SQLite 手册 (`cap:sql`)

## 一、 架构总览与设计准则

`cap:sql` 是 CP 语言官方内置的多方言关系型数据库协议与嵌入式存储标准库，全面吸收自 Bun 原生 SQL 驱动与 `bun:sqlite` 引擎（`/data/dev/bun/src/sql/`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├──────────────────────┬─────────────────────────┬───────────────────────┤
│ 统一参数绑定防注入层   │  PostgreSQL v3 协议引擎 │   MySQL 4.1+ 协议引擎 │
│ SqlStatement         │  PostgresWire (Startup/ │   MySqlPacketHeader   │
│ bindInt / bindText   │  Password/SimpleQuery)  │   MySqlLenEnc         │
│ bindBool / bindNull  │  PostgresAuthChallenge  │   MySqlCapabilities   │
│ SqlDialect (3方言)   │  PostgresDataRow (零拷贝)│   MySqlOk/ErrPacket   │
├──────────────────────┴─────────────────────────┴───────────────────────┤
│              嵌入式原生数据库引擎: SqliteDatabase / SqliteRows          │
│              (支持 :memory: 内存库与持久化文件库、零拷贝游标迭代)        │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/sql/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sql/protocol.cp` | `// Source: /data/dev/bun/src/sql/` | `SqlDialect`、`SqlType`、`SqlParam`、`SqlStatement` 参数化语句构建器、`PostgresWire` 报文编码器、`MySqlPacketHeader` 4 字节帧头 |
| `lib/sql/postgres.cp` | `// Source: /data/dev/bun/src/sql/` | `PostgresAuthKind`、`PostgresAuthChallenge` (MD5/SASL 认证挑战解析)、`PostgresDataRow` 零拷贝列迭代器、`PostgresCommandComplete` |
| `lib/sql/mysql.cp` | `// Source: /data/dev/bun/src/sql/` | `MySqlLenEnc` 变长整数编解码、`MySqlCapabilities` 能力协商掩码、`MySqlOkPacket` 与 `MySqlErrPacket` 解析 |
| `lib/sql/sqlite.cp` | `// Source: /data/dev/bun/src/sql/` | `SqliteDatabase` 与 `SqliteRows` 原生 SQLite3 C-ABI 封装（支持 `:memory:` 与磁盘事务） |
| `lib/sql/index.cp` | `// Source: /data/dev/bun/src/sql/` | `cap:sql` 标准库对外统一导出入口 |

---

## 二、 PostgreSQL v3 与 MySQL 4.1+ 原生二进制协议帧格式

### 1. PostgreSQL Frontend/Backend v3 协议帧结构

1. **`StartupMessage`（握手启动帧，无单字节标签前缀）**：
   ```text
   ┌──────────────────────┬──────────────────────┬──────────────────────────┬──────┐
   │ Total Length (4B BE) │ Version 0x00030000   │ "user\0<u_name>\0..."    │ '\0' │
   └──────────────────────┴──────────────────────┴──────────────────────────┴──────┘
   ```
   由 `PostgresWire.encodeStartupMessage(out, user, database)` 直接在栈缓冲区生成。
2. **常规消息帧（1 字节 Tag + 4 字节大端序 Length + Payload）**：
   - `'Q'` (`SimpleQuery`)：`PostgresWire.encodeSimpleQuery(out, sql)`；
   - `'p'` (`PasswordMessage`)：`PostgresWire.encodePasswordMessage(out, password)`；
   - `'R'` (`Authentication`)：`PostgresAuthChallenge.parse(payload)` 自动识别 `Ok(0)`、`CleartextPassword(3)`、`MD5Password(5)`（提取 4 字节 `md5_salt`）与 `SASL(10)`（提取 `"SCRAM-SHA-256"` 机制名）；
   - `'D'` (`DataRow`)：`PostgresDataRow.init(payload)` -> `.nextColumn()` 零拷贝遍历每列切片（自动识别 `0xFFFFFFFF` 即 `-1` 为 SQL `NULL`）；
   - `'C'` (`CommandComplete`)：`PostgresCommandComplete.parse(raw_tag)` 提取受影响行数 `rows_affected`。

### 2. MySQL 4.1+ 客户端/服务端协议帧结构

1. **4 字节报文头 (`MySqlPacketHeader`)**：
   - `[0..3]`：3 字节小端序（Little-Endian）载荷长度（最大 `16MB - 1`）；
   - `[3]`：1 字节序列号 `sequence_id`。
2. **变长编码整数 (`MySqlLenEnc`)**：
   - `< 251`：单字节直接表示；
   - `251..65535`：`0xFC` + 2 字节小端序 `u16`；
   - `65536..16777215`：`0xFD` + 3 字节小端序 `u24`；
   - `>= 16777216`：`0xFE` + 8 字节小端序 `u64`。
3. **响应包解析 (`MySqlOkPacket` & `MySqlErrPacket`)**：
   - `0x00` / `0xFE` (`OK_Packet`)：自动提取 `affected_rows`、`last_insert_id`、`status_flags`、`warnings`；
   - `0xFF` (`ERR_Packet`)：自动提取 16 位 `error_code`、5 字节 `#SQLSTATE`（如 `"28000"`）与错误文本 `message`。

---

## 三、 SQL 参数化绑定防注入与 SQLite 原生引擎代码示例

```typescript
import {
    SqlDialect,
    SqlType,
    SqlStatement,
    PostgresWire,
    PostgresAuthChallenge,
    PostgresDataRow,
    PostgresCommandComplete,
    MySqlPacketHeader,
    MySqlLenEnc,
    MySqlOkPacket,
    MySqlErrPacket,
    SqliteDatabase,
} from "cap:sql";

pub fn demoSqlAllDialects(): !void {
    // =========================================================================
    // 1. 类型安全参数化绑定 (防止 SQL 注入)
    // =========================================================================
    let stmt = SqlStatement.prepare(
        "SELECT id, role FROM users WHERE tenant_id = $1 AND username = $2 AND active = $3",
        SqlDialect.Postgres
    );
    * = stmt.bindInt(1024);
    * = stmt.bindText("admin' OR '1'='1"); // 作为纯文本参数隔离绑定，绝不拼接 SQL 语法树
    * = stmt.bindBool(true);

    // =========================================================================
    // 2. PostgreSQL v3 握手、查询编码与 DataRow 零拷贝读取
    // =========================================================================
    let pg_buf: [256]u8 = undefined;
    const startup_len = PostgresWire.encodeStartupMessage(&pg_buf, "postgres", "production_db");
    const query_len = PostgresWire.encodeSimpleQuery(&pg_buf, "SELECT name, email FROM users;");

    // 解析后端返回的 DataRow ('D' 载荷: 2 列 -> "Alice", NULL)
    let row = PostgresDataRow.init("\x00\x02\x00\x00\x00\x05Alice\xFF\xFF\xFF\xFF").?;
    const col0 = row.nextColumn().?; // col0.data == "Alice", col0.is_null == false
    const col1 = row.nextColumn().?; // col1.is_null == true

    // =========================================================================
    // 3. 嵌入式 SQLite3 内存数据库与游标查询
    // =========================================================================
    let db = SqliteDatabase.open(":memory:")?;
    defer db.close();

    db.exec("CREATE TABLE agents (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);")?;
    db.exec("INSERT INTO agents (name, score) VALUES ('Alpha', 98), ('Beta', 100);")?;

    let rows = db.query("SELECT id, name, score FROM agents ORDER BY score DESC;")?;
    defer rows.close();

    while (rows.next()) {
        const id = rows.columnInt(0);
        const name = rows.columnText(1);
        const score = rows.columnInt(2);
    }
}
```

---

## 四、 与 `Bun.sql` / `bun:sqlite` API 对照表

| 功能特性 | Bun (`Bun.sql` / `bun:sqlite`) | CP 标准库 (`cap:sql`) |
| :--- | :--- | :--- |
| **参数化语句防注入** | `` sql`SELECT * FROM t WHERE id = ${id}` `` | `SqlStatement.prepare(sql, dialect)` -> `.bindInt` / `.bindText` / `.bindBool` |
| **Postgres 启动握手帧** | ` postgres/protocol/StartupMessage ` | `PostgresWire.encodeStartupMessage(out, user, db)` |
| **Postgres 查询与密码帧** | ` postgres/protocol/Query ` | `PostgresWire.encodeSimpleQuery` / `encodePasswordMessage` |
| **Postgres 认证与数据行** | ` postgres/protocol/DataRow ` | `PostgresAuthChallenge.parse` / `PostgresDataRow.init(payload)` |
| **MySQL 报文头与变长整数** | ` mysql/protocol/Packet ` | `MySqlPacketHeader.encode/decode` / `MySqlLenEnc.encode/decode` |
| **MySQL OK / ERR 包解析** | ` mysql/protocol/OKPacket ` | `MySqlOkPacket.parse(payload)` / `MySqlErrPacket.parse(payload)` |
| **SQLite 打开数据库** | `new Database(":memory:")` | `SqliteDatabase.open(":memory:")` |
| **SQLite 执行 DDL/DML** | `db.exec(sql)` / `db.run(sql)` | `db.exec(sql)` |
| **SQLite 查询与遍历** | `db.query(sql).all()` | `db.query(sql)` -> `rows.next()` -> `rows.columnText(i)` / `rows.columnInt(i)` |
