# CP 标准库：统一多方言 SQL 协议引擎与嵌入式 SQLite 手册 (`cap:sql`)

## 一、 架构总览与设计准则

`cap:sql` 是 CP 语言官方内置的多方言关系型数据库协议与嵌入式存储标准库，直接原版移植自 Bun 原生 SQL 驱动与 `bun:sqlite` 引擎（`/data/dev/bun/src/sql/`，10 个文件 4,718 行）。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                          CP 用户层应用 (Application)                           │
├─────────────────────────────┬──────────────────────────┬─────────────────────┤
│  共享数据与 PostgreSQL v3    │   MySQL 4.1+ / 8.0 协议   │   嵌入式 SQLite3     │
│  SqlData (15B SSO) / flags  │  Caching-SHA2 / Native   │  SqliteDatabase     │
│  PgTypeTag (90+ OIDs)       │  AuthSwitch / SSLRequest │  SqliteRows 游标     │
│  PgBinaryCodec (numeric/arr)│  COM_STMT_PREPARE/EXECUTE│  参数绑定 & 事务控制 │
│  PgProtocol (Startup/Copy)  │  MySqlBinaryRowDecoder   │  begin/commit/rollb │
└─────────────────────────────┴──────────────────────────┴─────────────────────┘
```

### 1. 模块文件拓扑与谱系来源（共 10 文件，4,718 行）

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sql/index.cp` | `// Source: /data/dev/bun/src/sql/` | `cap:sql` 统一公开导出入口 |
| `lib/sql/shared.cp` | `// Source: /data/dev/bun/src/sql/shared/Data.cp` | `SQLQueryResultMode`、`ConnectionFlags`、15B 小对象优化 `SqlData`、`ColumnIdentifier` |
| `lib/sql/pg_types.cp` | `// Source: /data/dev/bun/src/sql/postgres/types/Tag.cp` | `AnyPostgresError`、90+ `PgTypeTag` OID 映射、`PgCommandTag`、`PgErrorClassifier`、`PgBinaryCodec`（int2/int4/int8/float4/float8/bool/bytea/uuid/jsonb/date/timestamp/numeric/int4_array） |
| `lib/sql/pg_protocol.cp` | `// Source: /data/dev/bun/src/sql/postgres/PostgresProtocol.cp` | `PgBufferReader`/`PgBufferWriter`、`PgParameterDescription`、`PgNotificationResponse`、`PgNoticeResponse`、`PgNegotiateProtocolVersion`、`PgCopyResponse`、`PgProtocol`（SSL/Cancel/StartupUTF8/Copy） |
| `lib/sql/mysql_types.cp` | `// Source: /data/dev/bun/src/sql/mysql/MySQLTypes.cp` | `AnyMySQLError`、`MySqlAuthMethod`、`MySqlCapabilityFlags`、`MySqlStatusFlags`、`MySqlCharacterSet`、`MySqlFieldType`、`MySqlErrorClassifier`、`MySqlValueDateTime`、`MySqlValueTime` |
| `lib/sql/mysql_protocol.cp` | `// Source: /data/dev/bun/src/sql/mysql/protocol/Auth.cp` | `MySqlBufferReader`、`MySqlAuthNative` (SHA-1)、`MySqlAuthCachingSha2` (SHA-256)、`MySqlAuthSwitchRequest`、`MySqlSSLRequest`、`MySqlColumnDefinition41`、`MySqlResultSetHeader`、`MySqlPrepareOK`、`MySqlPreparedProtocol`、`MySqlBinaryRowDecoder` |
| `lib/sql/protocol.cp` | `// Source: /data/dev/bun/src/sql/` | 向后兼容：`SqlDialect`、`SqlType`、`SqlValue`、`SqlParam`、`SqlStatement`、`PostgresWire`、`MySqlPacketHeader`、`SqlConnectionPool`、`SqlTransactionStateMachine` |
| `lib/sql/postgres.cp` | `// Source: /data/dev/bun/src/sql/` | 向后兼容：`PostgresBackendTag`、`PostgresBackendFrame`、`PostgresAuthChallenge`、`ScramSha256Client`、`PostgresRowDescription`、`PostgresDataRow`、`PostgresErrorResponse` 等 |
| `lib/sql/mysql.cp` | `// Source: /data/dev/bun/src/sql/` | 向后兼容：`MySqlLenEnc`、`MySqlLenEncStr`、`MySqlCapabilities`、`MySqlHandshakeV10`、`MySqlCommandEncoder`、`MySqlOkPacket`、`MySqlErrPacket`、`MySqlEofPacket` |
| `lib/sql/sqlite.cp` | `// Source: /data/dev/bun/src/sql/` | `SqliteDatabase` 与 `SqliteRows`（含 `bindNull`/`bindInt`/`bindDouble`/`bindText`/`bindBlob`/`columnName`/`columnBlob`/`reset`/`clearBindings`/`errMsg`/`begin`/`commit`/`rollback`） |

---

## 二、 核心新增工业协议能力详解

### 1. PostgreSQL 类型映射与二进制编解码 (`PgTypeTag` / `PgBinaryCodec`)

- **`PgTypeTag`**：覆盖 90+ 核心 OID（`pg_bool = 16`, `bytea = 17`, `int8 = 20`, `int2 = 21`, `int4 = 23`, `text = 25`, `json = 114`, `float4 = 700`, `float8 = 701`, `date = 1082`, `timestamp = 1114`, `timestamptz = 1184`, `numeric = 1700`, `uuid = 2950`, `jsonb = 3802` 及各类 `*_array`），提供 `fromOid()`、`isBinaryFormatSupported()`、`formatCode()`、`isArray()`。
- **`PgBinaryCodec`**：
  - 标量大端编解码：`encodeInt2`/`decodeInt2`、`encodeInt4`/`decodeInt4`、`encodeInt8`/`decodeInt8`、`encodeFloat4`/`decodeFloat4`、`encodeFloat8`/`decodeFloat8`、`encodeBool`/`decodeBool`；
  - `bytea` `\x` 十六进制编解码：`encodeByteaHex` / `decodeByteaText`；
  - `UUID` 16B 二进制与 36B 连字符文本互转：`parseUuid` / `formatUuid`；
  - `JSONB` 版本头（`0x01`）编解码：`encodeJsonb` / `decodeJsonb`；
  - `Date` 与 `Timestamp`（2000-01-01 Postgres 纪元 <-> Unix 毫秒/天）；
  - `NUMERIC` base-10000 二进制全精度文本解码：`decodeNumericText`（支持负数、`NaN`、`Infinity`、小数缩放 `dscale`）；
  - 单维二进制数组头及元素解码：`decodeArrayHeader` / `decodeInt4Array`。
- **`PgErrorClassifier` & `PgCommandTag`**：SQLSTATE 类别分类（`IntegrityConstraintViolation`、`TransactionRollback` 等）与可重试判断；`INSERT 0 N` / `UPDATE N` 等行数解析。

### 2. MySQL 8.0 认证、预编译与二进制行解码 (`MySqlAuthCachingSha2` / `MySqlPreparedProtocol` / `MySqlBinaryRowDecoder`)

- **`MySqlAuthNative` & `MySqlAuthCachingSha2`**：完整 SHA-1 scramble（`mysql_native_password`）与 SHA-256 scramble（`caching_sha2_password`）、FastAuth 响应解析（`0x03` 成功 / `0x04` 继续）、`MySqlAuthSwitchRequest` 解析/响应与 `MySqlSSLRequest` 编码；
- **`MySqlPreparedProtocol`**：`encodePrepare` (`COM_STMT_PREPARE`)、`encodeExecute` (`COM_STMT_EXECUTE` 含 NULL 位图与二进制参数载荷)、`encodeClose` (`COM_STMT_CLOSE`)；
- **`MySqlBinaryRowDecoder.decodeRow`**：依据 `MySqlColumnDefinition41` 列表解析 MySQL 二进制结果行（NULL 位图 `(N+9)/8` 偏移、有符号/无符号整型、IEEE 754 浮点、`MySqlValueDateTime`、`MySqlValueTime`、BIT 与字符串/BLOB）。

### 3. SQLite 事务与参数化绑定 (`SqliteDatabase` / `SqliteRows`)

- `SqliteRows` 支持 `bindInt` / `bindDouble` / `bindText` / `bindBlob` / `bindNull` / `reset` / `clearBindings` 与 `columnName` / `columnBlob`；
- `SqliteDatabase` 支持 `begin()` / `commit()` / `rollback()` 与 `errMsg()`。

---

## 三、 代码示例

```typescript
const std = @import("std");
import {
    PgTypeTag,
    PgBinaryCodec,
    MySqlAuthCachingSha2,
    MySqlPreparedProtocol,
    MySqlBinaryRowDecoder,
    MySqlColumnDefinition41,
    MySqlDecodedCell,
    SqlParam,
    SqliteDatabase,
} from "cap:sql";

pub fn demoSqlCodecs(): ! {
    // 1. PostgreSQL NUMERIC 二进制解码与 UUID 编解码
    let buf: [64]u8 = [0; 64];
    const num_raw: [12]u8 = [0, 2, 0, 0, 0x40, 0x00, 0, 4, 0, 123, 0x11, 0x94];
    const dec = PgBinaryCodec.decodeNumericText(num_raw[0..12], buf[0..64])?;
    * = dec;
    * = PgTypeTag.fromOid(3802).isBinaryFormatSupported();

    // 2. MySQL caching_sha2_password & COM_STMT_EXECUTE
    const scramble = MySqlAuthCachingSha2.scramble("secret", "12345678901234567890")?;
    * = scramble;
    let pkt: [128]u8 = [0; 128];
    const params: [1]SqlParam = [SqlParam{ kind: .Int64, int_val: 42 }];
    * = MySqlPreparedProtocol.encodeExecute(pkt[0..128], 1, 0, params[0..1], 0);

    // 3. SQLite 参数化绑定与事务
    let db = SqliteDatabase.open(":memory:")?;
    defer db.close();
    db.exec("CREATE TABLE kv (k INTEGER PRIMARY KEY, v TEXT);")?;
    db.begin()?;
    let st = db.query("INSERT INTO kv (k, v) VALUES (?, ?);")?;
    defer st.close();
    st.bindInt(1, 1)?;
    st.bindText(2, "ok")?;
    * = st.next();
    db.commit()?;
}
```

---

## 扩展 SQL 工具链子模块（收敛自原 `sql_*` 系列）

- **SQL 词法 / AST 解析与静态分析 (`lexer.cp`, `ast.cp`, `parser.cp`, `analyzer.cp`)**：
  - `tokenizeSql`、`parseSql`、`parseSqlWithMaxDepth`、`normalizeSqlFingerprint`、`extractTableNames`、`isReadOnlyQuery` 及完整 AST 节点（`SelectStmt` / `InsertStmt` / `UpdateStmt` / `DeleteStmt` / `CreateTableStmt` / `SqlStmt` / `SqlExpr`）。
- **数据库连接 URI 与命令标签解析 (`uri.cp`)**：
  - `SqlConnUri`、`SqlUriDialect`、`SslMode`、`UriPgCommandKind`、`UriPgCommandTag`。
- **标识符 / 字面量 / LIKE 转义 (`escape.cp`)**：
  - `escapeAnsiIdent`、`escapeMysqlIdent`、`escapeQualifiedAnsiIdent`、`escapeSqlStringAnsi`、`escapeSqlStringMysql`、`escapeLikePattern`。
- **参数占位符扫描与方言重写 (`params.cp`)**：
  - `countQuestionMarks`、`maxDollarParam`、`rewriteQuestionToDollar`、`NamedParamIter`。
- **多语句脚本切分与分类 (`splitter.cp`)**：
  - `SqlSplitStmtKind`、`SqlStmtIter`、`classifySqlStmt`、`isReadOnlySql`、`countSqlStatements`。
