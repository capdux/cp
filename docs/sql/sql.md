# CP 标准库：统一多方言 SQL 协议引擎与嵌入式 SQLite 手册 (`cap:sql`)

## 一、 架构总览与设计准则

`cap:sql` 是 CP 语言官方内置的多方言关系型数据库协议与嵌入式存储标准库，全面吸收自 Bun 原生 SQL 驱动与 `bun:sqlite` 引擎（`/data/dev/bun/src/sql/`）。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                          │
├──────────────────────────┬─────────────────────────┬───────────────────────┤
│ 统一参数/连接池/事务状态机 │  PostgreSQL v3 协议引擎 │   MySQL 4.1+ 协议引擎 │
│ SqlValue / SqlStatement  │  Startup/MD5/SCRAM-256  │   MySqlPacketHeader   │
│ SqlConnectionPool        │  Extended Query (P/B/D/E)│  LenEnc / LenEncStr   │
│ SqlTransactionStateMachine│ S/K/T/D/C/Z/E 后端帧解 │  HandshakeV10 / COM_* │
├──────────────────────────┴─────────────────────────┴───────────────────────┤
│              嵌入式原生数据库引擎: SqliteDatabase / SqliteRows              │
│              (支持 :memory: 内存库与持久化文件库、零拷贝游标迭代)            │
└────────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/sql/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sql/protocol.cp` | `// Source: /data/dev/bun/src/sql/` | `SqlDialect`、`SqlType`、`SqlValue`、`SqlParam`、`SqlStatement`、`PostgresWire`（Startup/SimpleQuery/Extended Query/SASL/MD5）、`MySqlPacketHeader`、`SqlConnectionPool`、`SqlTransactionStateMachine` |
| `lib/sql/postgres.cp` | `// Source: /data/dev/bun/src/sql/` | `PostgresBackendTag`、`PostgresBackendFrame`、`PostgresAuthChallenge`、`ScramSha256Client`、`PostgresParameterStatus`、`PostgresBackendKeyData`、`PostgresRowDescription`、`PostgresDataRow`、`PostgresCommandComplete`、`PostgresReadyForQuery`、`PostgresErrorResponse` |
| `lib/sql/mysql.cp` | `// Source: /data/dev/bun/src/sql/` | `MySqlLenEnc`、`MySqlLenEncStr`、`MySqlCapabilities`、`MySqlHandshakeV10`、`MySqlCommandType`、`MySqlCommandEncoder`、`MySqlOkPacket`、`MySqlErrPacket`、`MySqlEofPacket` |
| `lib/sql/sqlite.cp` | `// Source: /data/dev/bun/src/sql/` | `SqliteDatabase` 与 `SqliteRows` 原生 SQLite3 C-ABI 封装（支持 `:memory:` 与磁盘事务、`columnDouble`/`columnCount`/`changes`/`lastInsertRowId`） |
| `lib/sql/index.cp` | `// Source: /data/dev/bun/src/sql/` | `cap:sql` 标准库对外统一导出入口 |

---

## 二、 PostgreSQL v3 与 MySQL 4.1+ 完整二进制协议引擎

### 1. PostgreSQL Frontend/Backend v3 协议帧结构

1. **启动与认证（`StartupMessage` / `MD5` / `SCRAM-SHA-256`）**：
   - `PostgresWire.encodeStartupMessage(out, user, database)`：构造 `0x00030000` 握手启动帧；
   - `PostgresWire.encodeMD5Password(out, user, password, salt)`：基于 `cap:crypto` 计算 `"md5" + md5(md5(pw + user) + salt)`；
   - `ScramSha256Client` + `PostgresWire.encodeSASLInitialResponse` / `encodeSASLResponse`：完整实现 RFC 5802 / RFC 7677 SCRAM-SHA-256 握手（PBKDF2-HMAC-SHA-256、`ClientProof` 与 `ServerSignature` 双向校验）。
2. **扩展查询协议（Extended Query Protocol）**：
   - `PostgresWire.encodeParse` (`'P'`)：发送预编译语句名、SQL 文本与参数 OID 数组；
   - `PostgresWire.encodeBind` (`'B'`)：绑定 `SqlParam`（支持 `Null` `-1`、整型、浮点、布尔、文本、二进制）；
   - `PostgresWire.encodeDescribe` (`'D'`)、`encodeExecute` (`'E'`)、`encodeClose` (`'C'`)、`encodeSync` (`'S'`)、`encodeFlush` (`'H'`)、`encodeTerminate` (`'X'`)。
3. **后端响应帧解码（`PostgresBackendFrame.decode`）**：
   - `'R'` (`Authentication`)：`PostgresAuthChallenge.parse`；
   - `'S'` (`ParameterStatus`)：`PostgresParameterStatus.parse`；
   - `'K'` (`BackendKeyData`)：`PostgresBackendKeyData.parse`；
   - `'T'` (`RowDescription`)：`PostgresRowDescription.parse`（解析列元数据、OID、格式码，支持 `findField(name)`）；
   - `'D'` (`DataRow`)：`PostgresDataRow.init` + `.nextColumn()` 零拷贝字段切片解码；
   - `'C'` (`CommandComplete`)：`PostgresCommandComplete.parse`；
   - `'Z'` (`ReadyForQuery`)：`PostgresReadyForQuery.parse` 提取 `'I'`/`'T'`/`'E'` 事务状态；
   - `'E'` (`ErrorResponse`)：`PostgresErrorResponse.parse` 提取 `severity`、`sql_state`、`message`、`detail`、`hint`、`constraint`。

### 2. MySQL 4.1+ 线协议核心编解码

1. **4 字节报文头与变长编解码**：
   - `MySqlPacketHeader.encode/decode`（3 字节小端序长度 + 1 字节 `sequence_id`）；
   - `MySqlLenEnc` 与 `MySqlLenEncStr`（支持 `0xFB` NULL 字符串与 1/3/4/9 字节变长编码）。
2. **握手与命令帧**：
   - `MySqlHandshakeV10.parse`：解析 `server_version`、`connection_id`、能力掩码、插件名及 20 字节 scramble；
   - `MySqlCommandEncoder`：`encodeQuery` (`COM_QUERY`)、`encodePing` (`COM_PING`)、`encodeQuit` (`COM_QUIT`)、`encodeInitDb` (`COM_INIT_DB`)、`encodeHandshakeResponse41`；
   - `MySqlOkPacket.parse` / `MySqlErrPacket.parse` / `MySqlEofPacket.parse`。

### 3. 通用连接池 (`SqlConnectionPool`) 与事务状态机 (`SqlTransactionStateMachine`)

- **`SqlConnectionPool`**：管理多方言连接槽位（`Empty`/`Idle`/`InUse`/`Closed`），提供 `registerConnection`、`acquire`、`releaseConnection`、`evictIdle`、`closeConnection` 与 `SqlPoolStats`。
- **`SqlTransactionStateMachine`**：强制维护 `Idle` -> `InTransaction` -> `Failed` 状态迁移；生成方言敏感的 `BEGIN [ISOLATION LEVEL ...] [READ ONLY]` / `START TRANSACTION` / `BEGIN IMMEDIATE` / `COMMIT` / `ROLLBACK` / `SAVEPOINT` / `ROLLBACK TO SAVEPOINT` / `RELEASE SAVEPOINT`，支持从 PostgreSQL `ReadyForQuery` 自动同步状态。

---

## 三、 代码示例

```typescript
import {
    SqlDialect,
    SqlStatement,
    PostgresWire,
    PostgresBackendFrame,
    PostgresRowDescription,
    PostgresDataRow,
    PostgresReadyForQuery,
    PostgresErrorResponse,
    ScramSha256Client,
    MySqlHandshakeV10,
    MySqlCommandEncoder,
    SqlConnectionPool,
    SqlTransactionStateMachine,
    SqlIsolationLevel,
    SqliteDatabase,
} from "cap:sql";

pub fn demoSqlEngines(): !void {
    // 1. PostgreSQL Extended Query + SCRAM-SHA-256
    let wire: [512]u8 = undefined;
    const oids = [23 as u32];
    const p_len = PostgresWire.encodeParse(&wire, "s1", "SELECT $1::int4", &oids);
    * = p_len;

    let scram = ScramSha256Client.init();
    let cf_buf: [128]u8 = undefined;
    const cf = try scram.buildClientFirst("postgres", "nonce123", &cf_buf);
    * = PostgresWire.encodeSASLInitialResponse(&wire, "SCRAM-SHA-256", cf);

    // 2. 通用连接池与事务状态机
    let pool = SqlConnectionPool.init(SqlDialect.Postgres, 16, 60000);
    * = pool.registerConnection(1, 1000, false);
    if (cid := pool.acquire(2000)) {
        defer * = pool.releaseConnection(cid, 2500, true);
    }

    let tx = SqlTransactionStateMachine.init(SqlDialect.Postgres);
    let sql_buf: [128]u8 = undefined;
    const begin_sql = try tx.begin(SqlIsolationLevel.Serializable, false, &sql_buf);
    const sp_sql = try tx.savepoint("sp1", &sql_buf);
    const commit_sql = try tx.commit(&sql_buf);
    * = begin_sql;
    * = sp_sql;
    * = commit_sql;
}
```

---

## 四、 与 `Bun.sql` / `bun:sqlite` API 对照表

| 功能特性 | Bun (`Bun.sql` / `bun:sqlite`) | CP 标准库 (`cap:sql`) |
| :--- | :--- | :--- |
| **参数化语句防注入** | `` sql`SELECT * FROM t WHERE id = ${id}` `` | `SqlStatement.prepare` -> `.bindInt` / `.bindFloat` / `.bindText` / `.bindBlob` / `.bindBool` |
| **Postgres 启动与认证** | `StartupMessage` / `SCRAM-SHA-256` | `PostgresWire.encodeStartupMessage` / `encodeMD5Password` / `ScramSha256Client` |
| **Postgres 扩展查询** | `Parse` / `Bind` / `Describe` / `Execute` / `Sync` | `PostgresWire.encodeParse` / `encodeBind` / `encodeDescribe` / `encodeExecute` / `encodeSync` |
| **Postgres 后端帧解码** | `RowDescription` / `DataRow` / `ErrorResponse` / `ReadyForQuery` | `PostgresBackendFrame.decode` + 全套强类型结构体解析器 |
| **MySQL 握手与命令** | `HandshakeV10` / `COM_QUERY` / `OK` / `ERR` / `EOF` | `MySqlHandshakeV10.parse` / `MySqlCommandEncoder` / `MySqlOkPacket` / `MySqlErrPacket` / `MySqlEofPacket` |
| **连接池与事务状态机** | 内置连接池 & `sql.begin` / `savepoint` | `SqlConnectionPool` & `SqlTransactionStateMachine` |
| **SQLite 嵌入式引擎** | `new Database(":memory:")` | `SqliteDatabase.open(":memory:")` -> `exec` / `query` / `changes` / `lastInsertRowId` |
