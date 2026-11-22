# CP 标准库：统一多方言 SQL 协议引擎与嵌入式 SQLite 手册 (`cap:sql`)

> 参考来源：`/data/dev/bun/docs/runtime/sql.mdx`, `/data/dev/bun/docs/runtime/sqlite.mdx`, `/data/dev/bun/src/sql/` | 吸收日期：2026-11-22

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
| `lib/sql/pgTypes.cp` | `// Source: /data/dev/bun/src/sql/postgres/types/Tag.cp` | `AnyPostgresError`、90+ `PgTypeTag` OID 映射、`PgCommandTag`、`PgErrorClassifier`、`PgBinaryCodec`（int2/int4/int8/float4/float8/bool/bytea/uuid/jsonb/date/timestamp/numeric/int4_array） |
| `lib/sql/pgProtocol.cp` | `// Source: /data/dev/bun/src/sql/postgres/PostgresProtocol.cp` | `PgBufferReader`/`PgBufferWriter`、`PgParameterDescription`、`PgNotificationResponse`、`PgNoticeResponse`、`PgNegotiateProtocolVersion`、`PgCopyResponse`、`PgProtocol`（SSL/Cancel/StartupUTF8/Copy） |
| `lib/sql/mysqlTypes.cp` | `// Source: /data/dev/bun/src/sql/mysql/MySQLTypes.cp` | `AnyMySQLError`、`MySqlAuthMethod`、`MySqlCapabilityFlags`、`MySqlStatusFlags`、`MySqlCharacterSet`、`MySqlFieldType`、`MySqlErrorClassifier`、`MySqlValueDateTime`、`MySqlValueTime` |
| `lib/sql/mysqlProtocol.cp` | `// Source: /data/dev/bun/src/sql/mysql/protocol/Auth.cp` | `MySqlBufferReader`、`MySqlAuthNative` (SHA-1)、`MySqlAuthCachingSha2` (SHA-256)、`MySqlAuthSwitchRequest`、`MySqlSSLRequest`、`MySqlColumnDefinition41`、`MySqlResultSetHeader`、`MySqlPrepareOK`、`MySqlPreparedProtocol`、`MySqlBinaryRowDecoder` |
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
- **MySQL 变长整数与长度前缀字符串编解码 (`mysqlLenenc.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/EncodeInt.cp`)**：
  - 常量与错误集：`MYSQL_LENENC_NULL_MARKER` (`0xfb`)、`MYSQL_LENENC_U16_MARKER` (`0xfc`)、`MYSQL_LENENC_U24_MARKER` (`0xfd`)、`MYSQL_LENENC_U64_MARKER` (`0xfe`)、`MYSQL_LENENC_ERR_MARKER` (`0xff`)、`MySqlLenEncError`（`BufferTooSmall`/`TruncatedInput`/`NullMarker`/`ErrMarker`/`LengthOverflow`）
  - 数据模型与编解码函数：`EncodedLengthInt`、`DecodeLengthIntResult`、`DecodeLengthStringResult`、`encodedLengthIntSize`、`encodeLengthInt`、`encodeLengthIntInto`、`decodeLengthInt`、`decodeLengthIntStrict`、`encodeLengthString`、`decodeLengthString`（零堆分配、1/3/4/9 字节小端编解码与零拷贝字符串切片解码）

```typescript
import { encodeLengthInt, decodeLengthInt, encodeLengthString, decodeLengthString } from "cap:sql";

const enc = encodeLengthInt(65536); // 4-byte 0xfd little-endian encoding
const decoded = decodeLengthInt(enc.slice()) ?? unreachable; // value=65536, bytes_read=4

let wire: [64]u8 = [0; 64];
const n = encodeLengthString("users_table", wire[0..])?;
const str = decodeLengthString(wire[0..n])?; // data="users_table", bytes_read=12
```
- **MySQL `ERR_Packet` 编解码与错误分类 (`mysqlErr.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/ErrorPacket.cp`)**：
  - 常量与错误集：`MYSQL_ERR_PACKET_HEADER` (`0xff`)、`MYSQL_SQL_STATE_MARKER` (`'#'`)、`MySqlErrorPacketError`（`InvalidHeader` / `TruncatedPacket` / `BufferTooSmall` / `InvalidSqlState`）
  - 结构体与方法：`MySqlErrorPacket`（`initWithSqlState` / `initLegacy` / `decode` / `encode` / `encodedSize` / `sqlStateSlice` / `sqlStateClass` / `isRetryable`，兼容 MySQL 4.1+ `#SQLSTATE` 与 4.0 旧版错误包，零拷贝提取错误消息）

```typescript
import { MySqlErrorPacket } from "cap:sql";

const pkt = MySqlErrorPacket.initWithSqlState(1213, "40001", "Deadlock found")?;
let wire: [64]u8 = [0; 64];
const n = pkt.encode(wire[0..])?;
const decoded = MySqlErrorPacket.decode(wire[0..n])?; // error_code=1213, isRetryable()==true
```
- **PostgreSQL 线协议零堆分配二进制帧写入器 (`pgWriter.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/NewWriter.cp`)**：
  - 常量与错误集：`PG_NULL_LENGTH_U32` (`0xffffffff`)、`PG_NULL_LENGTH_I32` (`-1`)、`PgWireWriterError`（`BufferTooSmall` / `TooManyParameters` / `InvalidPatchOffset` / `FrameLengthOverflow`）
  - 结构体与方法：`PgLengthPatch`、`PgWireWriter`（`init` / `offset` / `remaining` / `written` / `reset` / `writeByte` / `writeBytes` / `pwrite` / `writeU16` / `writeShort` / `writeU32` / `writeI32` / `writeI64` / `writeF32` / `writeF64` / `writeCString` / `writeBoolText` / `writeNull` / `writeLengthPrefixed` / `beginLength` / `beginTaggedFrame` / `finishLength` / `finishLengthExcludingSelf`）

```typescript
import { PgWireWriter } from "cap:sql";

let buf: [128]u8 = [0; 128];
let w = PgWireWriter.init(buf[0..]);
const frame = w.beginTaggedFrame('Q')?;
* = w.writeCString("SELECT 1")?;
const frame_len = w.finishLength(frame)?; // 4 + 9 = 13 bytes
const wire = w.written(); // ['Q', 0, 0, 0, 13, 'S', 'E', ..., 0]
```
- **MySQL 线协议零堆分配二进制读取器 (`mysqlReader.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/NewReader.cp`)**：
  - 错误集与结构体：`MySqlWireReaderError`（`ShortRead` / `InvalidEncodedInteger` / `InvalidEncodedLength` / `UnterminatedString` / `InvalidOffset`）、`MySqlWireReader`（`init` / `markMessageStart` / `offset` / `offsetFromStart` / `setOffsetFromStart` / `rewindToMessageStart` / `remaining` / `peek` / `ensureCapacity` / `skip` / `readBytes` / `readByte` / `readZ` / `readU16` / `readU24` / `readU32` / `readU64` / `readI32` / `readI64` / `readF32` / `readF64` / `readLenEncInt` / `readLenEncIntWithSize` / `readLenEncString` / `readLenEncStringOrNull`）

```typescript
import { MySqlWireReader } from "cap:sql";

const wire: [12]u8 = [0x03, 'd', 'e', 'f', 0x05, 'u', 's', 'e', 'r', 's', 0xfb, 0x2a];
let r = MySqlWireReader.init(wire[0..]);
const catalog = r.readLenEncString()?; // "def"
const table = r.readLenEncString()?;   // "users"
const opt = r.readLenEncStringOrNull()?; // null (0xfb)
const tag = r.readByte()?; // 0x2a
```
- **MySQL 线协议零堆分配二进制包写入器 (`mysqlWriter.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/NewWriter.cp`)**：
  - 常量与错误集：`MYSQL_PACKET_HEADER_SIZE` (`4`)、`MYSQL_MAX_PACKET_PAYLOAD` (`0xffffff`)、`MySqlWireWriterError`（`BufferTooSmall` / `InvalidPatchOffset` / `PacketLengthOverflow`）
  - 结构体与方法：`MySqlPacketPatch`、`MySqlWireWriter`（`init` / `offset` / `remaining` / `written` / `reset` / `writeByte` / `writeBytes` / `pwrite` / `writeU16` / `writeU24` / `writeU32` / `writeU64` / `writeI32` / `writeI64` / `writeF32` / `writeF64` / `writeZ` / `writeLenEncInt` / `writeLenEncString` / `writeLenEncNull` / `writeNullBitmap` / `beginPacket` / `finishPacket`）

```typescript
import { MySqlWireWriter, MySqlWireReader } from "cap:sql";

let buf: [64]u8 = [0; 64];
let w = MySqlWireWriter.init(buf[0..]);
const pkt = w.beginPacket(0)?;
* = w.writeByte(0x03)?; // COM_QUERY
* = w.writeBytes("SELECT 1")?;
const payload_len = w.finishPacket(pkt)?; // 9 bytes
let r = MySqlWireReader.init(w.written());
const hdr_len = r.readU24()?; // 9
```
- **PostgreSQL 线协议零堆分配二进制读取器 (`pgReader.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/NewReader.cp`)**：
  - 错误集与结构体：`PgWireReaderError`（`ShortRead` / `InvalidMessage` / `InvalidMessageLength` / `UnterminatedString`）、`PgFrameView`、`PgWireReader`（`init` / `markMessageStart` / `offset` / `offsetFromStart` / `rewindToMessageStart` / `remaining` / `peek` / `ensureCapacity` / `skip` / `readByte` / `readBytes` / `readZ` / `peekU32` / `peekI32` / `readU16` / `readI16` / `readU32` / `readI32` / `readI64` / `readF32` / `readF64` / `readMessageLength` / `readTaggedFrame` / `readLengthPrefixedOrNull` / `eatBytes`）

```typescript
import { PgWireWriter, PgWireReader } from "cap:sql";

let buf: [64]u8 = [0; 64];
let w = PgWireWriter.init(buf[0..]);
const p = w.beginTaggedFrame('Q')?;
* = w.writeCString("SELECT 1")?;
* = w.finishLength(p)?;

let r = PgWireReader.init(w.written());
const frame = r.readTaggedFrame()?; // tag='Q', payload="SELECT 1\0"
let body = PgWireReader.init(frame.payload);
const sql = body.readZ()?; // "SELECT 1"
```
- **MySQL `COM_QUERY` 与 8.0.23+ Query Attributes 零分配编解码器 (`mysqlQuery.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/Query.cp`)**：
  - 常量与类型：`MYSQL_COM_QUERY_CMD` (`0x03`)、`MYSQL_MAX_QUERY_PARAMS` (`64`)、`MySqlQueryCodecError`、`MySqlQueryParamKind`、`MySqlQueryParam`（支持 `nullParam`/`int32Param`/`int64Param`/`uint64Param`/`float64Param`/`stringParam`/`blobParam`，空参数名自动生成 `:p1`、`:p2`…）、`MySqlDecodedQuery`；
  - 编解码函数：`encodeMySqlSimpleQuery`、`encodeMySqlQueryWithAttrs`、`decodeMySqlSimpleQuery`、`decodeMySqlQueryWithAttrs`。

```typescript
import { MySqlQueryParam, encodeMySqlQueryWithAttrs, decodeMySqlQueryWithAttrs } from "cap:sql";

let buf: [256]u8 = [0; 256];
const params: [2]MySqlQueryParam = [
    MySqlQueryParam.int64Param("user_id", 42),
    MySqlQueryParam.stringParam("", "active"), // 自动命名 :p2
];
const wire = encodeMySqlQueryWithAttrs(buf[0..], 0, "SELECT * FROM users WHERE id = ? AND status = ?", params[0..])?;
const decoded = decodeMySqlQueryWithAttrs(wire)?; // decoded.param_count == 2, decoded.query == "SELECT ..."
```
- **PostgreSQL v3 `StartupMessage` / `SSLRequest` / `CancelRequest` 零分配编解码器 (`pgStartup.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/StartupMessage.cp`)**：
  - 常量与类型：`PG_PROTOCOL_V3_0` (`196608`)、`PG_SSL_REQUEST_CODE` (`80877103`)、`PG_CANCEL_REQUEST_CODE` (`80877102`)、`PG_MAX_STARTUP_PARAMS` (`32`)、`PgStartupCodecError`、`PgStartupParam`、`PgStartupConfig`（`database` 为空自动回退为 `user`，`client_encoding` 默认 `"UTF8"`）、`PgStartupParamIter`、`PgDecodedStartup`；
  - 编解码函数：`encodePgStartup`、`decodePgStartup`、`encodePgSslRequestFrame`、`isPgSslRequestFrame`、`encodePgCancelFrame`。

```typescript
import { PgStartupConfig, PgStartupParam, encodePgStartup, decodePgStartup } from "cap:sql";

let buf: [256]u8 = [0; 256];
const extras: [1]PgStartupParam = [PgStartupParam{ key: "TimeZone", value: "UTC" }];
const wire = encodePgStartup(buf[0..], PgStartupConfig{
    user: "postgres",
    application_name: "cap-agent",
    extra_params: extras[0..],
})?;
const decoded = decodePgStartup(wire)?; // database 自动回退为 "postgres", client_encoding == "UTF8"
```
- **PostgreSQL v3 `RowDescription` (`'T'`) 与 `FieldDescription` 零分配流式编解码器 (`pgRowDesc.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/RowDescription.cp` & `FieldDescription.cp`)**：
  - 常量与类型：`PG_ROW_DESCRIPTION_TAG` (`'T'`)、`PG_MAX_ROW_FIELDS` (`1664`)、`PgRowDescError`、`PgFieldFormatCode`（`text = 0` / `binary = 1`）、`PgFieldDescription`（含 `isBinary()` 与匿名数字列识别 `numericColumnIndex()`）、`PgRowDescriptionIter`（零堆流式列元数据迭代器）；
  - 编解码函数：`encodePgRowDescription`、`decodePgRowDescription`。

```typescript
import { PgFieldDescription, PgFieldFormatCode, encodePgRowDescription, decodePgRowDescription } from "cap:sql";

let buf: [256]u8 = [0; 256];
const cols: [2]PgFieldDescription = [
    PgFieldDescription{ name: "id", type_oid: 23, type_size: 4, format_code: PgFieldFormatCode.binary },
    PgFieldDescription{ name: "email", type_oid: 25, type_size: -1, format_code: PgFieldFormatCode.text },
];
const wire = encodePgRowDescription(buf[0..], cols[0..])?;
let it = decodePgRowDescription(wire)?; // it.count() == 2
while (col := it.next()?) { * = col.name; }
```
- **MySQL `OK_Packet` (`0x00`/`0xFE`)、`EOF_Packet` (`0xFE`) 与 16 位服务器状态标志编解码器 (`mysqlOkEof.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/{OKPacket,EOFPacket}.cp` & `StatusFlags.cp`)**：
  - 常量与类型：`MYSQL_OK_HEADER` (`0x00`)、`MYSQL_EOF_HEADER` (`0xFE`)、`MYSQL_FLAG_*`（14 项 16 位状态位常量）、`MySqlOkEofError`、`MySqlServerStatusFlags`（`inTransaction` / `isAutoCommit` / `hasMoreResults` / `wasSlowQuery` / `hasSessionStateChanged`）、`MySqlOkPacketView`、`MySqlEofPacketView`；
  - 编解码函数：`encodeMySqlOkPacket` / `decodeMySqlOkPacket`、`encodeMySqlEofPacket` / `decodeMySqlEofPacket`（严格校验 `EOF_Packet` 载荷长度 `< 9` 字节以区分 `CLIENT_DEPRECATE_EOF` 下的长 `OK_Packet`）。

```typescript
import {
    MYSQL_FLAG_IN_TRANS,
    MYSQL_FLAG_AUTOCOMMIT,
    MySqlServerStatusFlags,
    MySqlOkPacketView,
    encodeMySqlOkPacket,
    decodeMySqlOkPacket,
} from "cap:sql";

let buf: [128]u8 = [0; 128];
const flags = MySqlServerStatusFlags.fromInt(MYSQL_FLAG_IN_TRANS | MYSQL_FLAG_AUTOCOMMIT);
const wire = encodeMySqlOkPacket(buf[0..], MySqlOkPacketView{
    sequence_id: 1,
    affected_rows: 3,
    last_insert_id: 1001,
    status_flags: flags,
    warnings: 0,
    info: "Records: 3",
})?;
const decoded = decodeMySqlOkPacket(wire)?; // decoded.status_flags.inTransaction() == true
```
- **PostgreSQL v3 Extended Query 协议零分配编解码器 (`pgExtended.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp`)**：
  - 常量与类型：`PG_PARSE_TAG` (`'P'`)、`PG_DESCRIBE_TAG` (`'D'`)、`PG_EXECUTE_TAG` (`'E'`)、`PG_CLOSE_TAG` (`'C'`)、`PG_SYNC_TAG` (`'S'`)、`PG_FLUSH_TAG` (`'H'`)、`PG_TARGET_PORTAL` (`'P'`)、`PG_TARGET_STATEMENT` (`'S'`)、`PG_MAX_PARSE_PARAMS` (`65535`)、`PgExtendedError`、`PgTargetKind`、`PgTargetRef`（`portal`/`statement`/`tag`/`isUnnamed`）、`PgParamOidIter`（零堆参数 OID 迭代器）、`PgDecodedParse`、`PgExecuteMsg`；
  - 编解码函数：`encodePgParse` / `decodePgParse`、`encodePgDescribe` / `decodePgDescribe`、`encodePgExecute` / `decodePgExecute`、`encodePgClose` / `decodePgClose`、`encodePgSync` / `decodePgSync`、`encodePgFlush` / `decodePgFlush`。

```typescript
import {
    PgTargetRef,
    encodePgParse,
    decodePgParse,
    encodePgDescribe,
    encodePgExecute,
    encodePgClose,
    encodePgSync,
} from "cap:sql";

let buf: [256]u8 = [0; 256];
const oids: [2]i32 = [23, 25]; // int4, text
const parse_wire = encodePgParse(buf[0..], "stmt_user_by_id", "SELECT * FROM users WHERE id = $1 AND role = $2", oids[0..])?;
let parsed = decodePgParse(parse_wire)?; // parsed.paramCount() == 2
while (oid := parsed.params.next()?) { * = oid; }

const desc_wire = encodePgDescribe(buf[0..], PgTargetRef.statement("stmt_user_by_id"))?;
const exec_wire = encodePgExecute(buf[0..], "", 100)?; // 匿名 Portal，最多返回 100 行
const close_wire = encodePgClose(buf[0..], PgTargetRef.statement("stmt_user_by_id"))?;
const sync_wire = encodePgSync(buf[0..])?; // 5 字节定长同步帧 ['S', 0, 0, 0, 4]
* = desc_wire;
* = exec_wire;
* = close_wire;
* = sync_wire;
```
- **PostgreSQL v3 `ErrorResponse` (`'E'`) 与 `NoticeResponse` (`'N'`) 诊断帧零分配编解码器 (`pgErrNotice.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{ErrorResponse,NoticeResponse,FieldMessage,FieldType}.cp`)**：
  - 常量与类型：`PG_ERROR_RESPONSE_TAG` (`'E'`)、`PG_NOTICE_RESPONSE_TAG` (`'N'`)、`PG_MAX_DIAGNOSTIC_FIELDS` (`64`)、`PgErrorNoticeError`、`PgDiagnosticFieldKind`（完整覆盖 18 种 PostgreSQL 标准诊断字段 `'S'`/`'V'`/`'C'`/`'M'`/`'D'`/`'H'`/`'P'`/`'p'`/`'q'`/`'W'`/`'s'`/`'t'`/`'c'`/`'d'`/`'n'`/`'F'`/`'L'`/`'R'`）、`PgDiagnosticField`、`PgDiagnosticIter`（零堆流式迭代器）、`PgErrorNoticeView`（聚合视图 + `effectiveSeverity`/`isFatal`/`isConstraintViolation`/`isRetryableSqlState` 分类）；
  - 编解码函数：`encodePgErrorResponse`、`encodePgNoticeResponse`、`decodePgErrorNoticeIter`、`decodePgErrorOrNotice`、`decodePgErrorResponse`、`decodePgNoticeResponse`。

```typescript
import {
    PgDiagnosticField,
    PgDiagnosticFieldKind,
    encodePgErrorResponse,
    decodePgErrorResponse,
} from "cap:sql";

let buf: [256]u8 = [0; 256];
const fields: [4]PgDiagnosticField = [
    PgDiagnosticField.of(PgDiagnosticFieldKind.localized_severity, "ERROR"),
    PgDiagnosticField.of(PgDiagnosticFieldKind.code, "23505"),
    PgDiagnosticField.of(PgDiagnosticFieldKind.message, "duplicate key value violates unique constraint"),
    PgDiagnosticField.of(PgDiagnosticFieldKind.constraint, "users_email_key"),
];
const wire = encodePgErrorResponse(buf[0..], fields[0..])?;
const view = decodePgErrorResponse(wire)?; // view.isConstraintViolation() == true, view.constraint == "users_email_key"
* = view;
```
- **PostgreSQL v3 后端生命周期控制帧零分配编解码器 (`pgControl.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{ReadyForQuery,BackendKeyData,CommandComplete,ParameterStatus,TransactionStatusIndicator}.cp`)**：
  - 常量与类型：`PG_READY_FOR_QUERY_TAG` (`'Z'`)、`PG_BACKEND_KEY_DATA_TAG` (`'K'`)、`PG_COMMAND_COMPLETE_TAG` (`'C'`)、`PG_PARAMETER_STATUS_TAG` (`'S'`)、`PG_BACKEND_KEY_DATA_FRAME_LEN` (`12`)、`PgControlError`、`PgTxStatus`（`idle = 'I'` / `in_transaction = 'T'` / `failed_transaction = 'E'`）、`PgBackendKeyData`（`process_id` + `secret_key`）、`PgParameterStatusView`、`PgCommandCompleteView`（含 `rowsAffected()` 行数解析）；
  - 编解码函数：`encodePgReadyForQuery`/`decodePgReadyForQuery`、`encodePgBackendKeyData`/`decodePgBackendKeyData`、`encodePgCommandComplete`/`decodePgCommandComplete`、`encodePgParameterStatus`/`decodePgParameterStatus`。

```typescript
import {
    PgTxStatus,
    PgBackendKeyData,
    encodePgReadyForQuery,
    decodePgReadyForQuery,
    encodePgBackendKeyData,
    decodePgBackendKeyData,
    encodePgCommandComplete,
    decodePgCommandComplete,
} from "cap:sql";

let buf: [64]u8 = [0; 64];
const rfq_wire = encodePgReadyForQuery(buf[0..], PgTxStatus.in_transaction)?;
const tx_status = decodePgReadyForQuery(rfq_wire)?; // tx_status.inTransaction() == true

const key_wire = encodePgBackendKeyData(buf[0..], PgBackendKeyData{ process_id: 12345, secret_key: 0xdeadbeef })?;
const key = decodePgBackendKeyData(key_wire)?; // 用于后续 CancelRequest

const cc_wire = encodePgCommandComplete(buf[0..], "INSERT 0 5")?;
const cc = decodePgCommandComplete(cc_wire)?; // cc.rowsAffected() == 5
* = tx_status;
* = key;
* = cc;
```
- **MySQL 4.1/8.0 命令帧头、`COM_STMT_PREPARE_OK` 与 `ResultSetHeader` 零分配编解码器 (`mysqlStmt.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/{CommandType,PacketHeader,StmtPrepareOKPacket,ResultSetHeader}.cp`)**：
  - 常量与类型：`MYSQL_STMT_PREPARE_OK_STATUS` (`0x00`)、`MYSQL_STMT_PREPARE_OK_MIN_LEN` (`10`)、`MYSQL_STMT_PREPARE_OK_STD_LEN` (`12`)、`MySqlStmtControlError`、`MySqlWireCmd`（完整 31 种 `COM_*` 命令枚举 + `fromByte`/`isPreparedCommand`/`isConnectionControl`）、`MySqlWirePacketHeader`（24 位小端长度 + 16 MiB 分片延续判定 `isContinuationChunk()`）、`MySqlStmtPrepareOkView`、`MySqlResultSetHeaderView`、`MySqlSimpleCommandView`；
  - 编解码函数：`encodeMySqlStmtPrepareOk`/`decodeMySqlStmtPrepareOk`（兼容 10B 与 12B 标准含 `warning_count` 载荷）、`encodeMySqlResultSetHeader`/`decodeMySqlResultSetHeader`（支持 `CLIENT_OPTIONAL_RESULTSET_METADATA` 可选元数据标志）、`encodeMySqlSimpleCommand`/`decodeMySqlSimpleCommand`、`encodeMySqlStmtIdCommand`/`decodeMySqlStmtIdCommand`。

```typescript
import {
    MySqlWireCmd,
    MySqlStmtPrepareOkView,
    encodeMySqlStmtPrepareOk,
    decodeMySqlStmtPrepareOk,
    encodeMySqlResultSetHeader,
    decodeMySqlResultSetHeader,
    encodeMySqlStmtIdCommand,
    decodeMySqlStmtIdCommand,
} from "cap:sql";

let buf: [64]u8 = [0; 64];
const prep_wire = encodeMySqlStmtPrepareOk(buf[0..], MySqlStmtPrepareOkView{
    sequence_id: 1,
    statement_id: 42,
    num_columns: 3,
    num_params: 2,
    warning_count: 0,
})?;
const prep = decodeMySqlStmtPrepareOk(prep_wire)?; // prep.statement_id == 42

const rs_wire = encodeMySqlResultSetHeader(buf[0..], 1, 5, true, false)?;
const rs = decodeMySqlResultSetHeader(rs_wire)?; // rs.field_count == 5, rs.metadata_follows == false

const close_wire = encodeMySqlStmtIdCommand(buf[0..], 0, MySqlWireCmd.com_stmt_close, 42)?;
const closed_id = decodeMySqlStmtIdCommand(close_wire, MySqlWireCmd.com_stmt_close)?; // 42
* = prep;
* = rs;
* = closed_id;
```
- **MySQL 4.1/8.0 `AuthSwitch`、`SSLRequest` 与 `LOCAL_INFILE_Request` 零分配编解码器 (`mysqlAuth.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/{AuthSwitchRequest,AuthSwitchResponse,SSLRequest,LocalInfileRequest}.cp`)**：
  - 常量与类型：`MYSQL_AUTH_SWITCH_HEADER` (`0xFE`)、`MYSQL_LOCAL_INFILE_HEADER` (`0xFB`)、`MYSQL_SSL_REQUEST_PAYLOAD_LEN` (`32`)、`MYSQL_CAP_CLIENT_SSL` (`0x0800`)、`MySqlAuthFrameError`、`MySqlAuthSwitchReqView`、`MySqlAuthSwitchRespView`、`MySqlSslRequestView`、`MySqlLocalInfileReqView`（内置 `isSafeRelativeFilename()` 拦截绝对路径与 `..` 目录穿越）；
  - 编解码函数：`encodeMySqlAuthSwitchRequest`/`decodeMySqlAuthSwitchRequest`、`encodeMySqlAuthSwitchResponse`/`decodeMySqlAuthSwitchResponse`、`encodeMySqlSslRequestFrame`/`decodeMySqlSslRequestFrame`、`encodeMySqlLocalInfileRequest`/`decodeMySqlLocalInfileRequest`。

```typescript
import {
    MYSQL_CAP_CLIENT_SSL,
    MySqlSslRequestView,
    encodeMySqlAuthSwitchRequest,
    decodeMySqlAuthSwitchRequest,
    encodeMySqlSslRequestFrame,
    decodeMySqlSslRequestFrame,
    encodeMySqlLocalInfileRequest,
    decodeMySqlLocalInfileRequest,
} from "cap:sql";

let buf: [128]u8 = [0; 128];
const sw_wire = encodeMySqlAuthSwitchRequest(buf[0..], 2, "caching_sha2_password", "salt1234567890123456")?;
const sw = decodeMySqlAuthSwitchRequest(sw_wire)?; // sw.plugin_name == "caching_sha2_password"

const ssl_wire = encodeMySqlSslRequestFrame(buf[0..], MySqlSslRequestView{
    sequence_id: 1,
    capabilities: MYSQL_CAP_CLIENT_SSL | 0x00000200,
    max_packet_size: 16777216,
    character_set: 45,
})?;
const ssl = decodeMySqlSslRequestFrame(ssl_wire)?; // ssl.hasSslFlag() == true

const li_wire = encodeMySqlLocalInfileRequest(buf[0..], 1, "imports/batch.csv")?;
const li = decodeMySqlLocalInfileRequest(li_wire)?; // li.isSafeRelativeFilename() == true
* = sw;
* = ssl;
* = li;
```
- **PostgreSQL v3 鉴权 (`PasswordMessage`/`SASLInitialResponse`/`SASLResponse`)、`COPY` 流与 `Terminate` 零分配编解码器 (`pgAuthCopy.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{SASLInitialResponse,SASLResponse,PasswordMessage,CopyData,CopyFail}.cp`)**：
  - 常量与类型：`PG_PASSWORD_TAG` (`'p'`)、`PG_COPY_DATA_TAG` (`'d'`)、`PG_COPY_DONE_TAG` (`'c'`)、`PG_COPY_FAIL_TAG` (`'f'`)、`PG_TERMINATE_TAG` (`'X'`)、`PgAuthCopyError`、`PgSaslInitialResponseView`（支持 `data == null` 编码线协议 `-1` 无初始载荷）；
  - 编解码函数：`encodePgPasswordMessage`/`decodePgPasswordMessage`、`encodePgSaslInitialResponse`/`decodePgSaslInitialResponse`、`encodePgSaslResponse`/`decodePgSaslResponse`、`encodePgCopyData`/`decodePgCopyData`、`encodePgCopyDone`/`decodePgCopyDone`、`encodePgCopyFail`/`decodePgCopyFail`、`encodePgTerminate`/`decodePgTerminate`。

```typescript
import {
    encodePgSaslInitialResponse,
    decodePgSaslInitialResponse,
    encodePgCopyData,
    decodePgCopyData,
    encodePgCopyDone,
    encodePgTerminate,
} from "cap:sql";

let buf: [256]u8 = [0; 256];
const sasl_wire = encodePgSaslInitialResponse(buf[0..], "SCRAM-SHA-256", "n,,n=,r=nonce123")?;
const sasl = decodePgSaslInitialResponse(sasl_wire)?; // sasl.mechanism == "SCRAM-SHA-256"

const copy_wire = encodePgCopyData(buf[0..], "1\tAlice\n2\tBob\n")?;
const copy_payload = decodePgCopyData(copy_wire)?;
const done_wire = encodePgCopyDone(buf[0..])?;      // ['c', 0, 0, 0, 4]
const term_wire = encodePgTerminate(buf[0..])?;     // ['X', 0, 0, 0, 4]
* = sasl;
* = copy_payload;
* = done_wire;
* = term_wire;
```

---

## 上游 Bun SQL 行为契约与安全边界注意事项

1. **三方言统一协议与连接串路由（参考 `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - 覆盖 **PostgreSQL v3**（`postgres://` / `postgresql://`，默认回退方言，支持 SCRAM-SHA-256 / MD5 / 明文鉴权及 `PgBinaryCodec` 90+ OID 二进制编解码）、**MySQL 5.7+ / 8.0+**（`mysql://` / `mysql2://`，支持 `mysql_native_password` SHA-1 与 `caching_sha2_password` SHA-256、Query Attributes、二进制预处理协议与查询管线化）以及 **嵌入式 SQLite3**（`sqlite://` / `file://` / `:memory:` / `SqliteDatabase`，无协议前缀的裸文件名须显式指定 SQLite 适配器以避免与 PostgreSQL 产生歧义）。
2. **PostgreSQL Simple vs Extended Query 与预处理语句最佳实践（参考 `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - **Simple Query (`'Q'`) vs Extended Query (`'P'`/`'B'`/`'D'`/`'E'`/`'C'`/`'S'`)**：Simple Query 支持单次提交多条分号分隔语句但不支持参数绑定；Extended Query 是默认推荐路径，支持二进制参数绑定与查询管线化（Pipelining），但单条 `Parse` 仅允许包含一条 SQL 语句。
   - **命名 vs 匿名预处理语句（Named vs Unnamed Prepared Statements）**：对静态高频查询使用命名语句（`name != ""`）可复用服务端执行计划并启用管线化；在 **PGBouncer 事务池模式（Transaction Pooling）** 或低频动态查询场景下，应使用匿名语句（`name = ""`，对应 Bun `prepare: false`），其生命周期仅持续至下一次匿名 `Parse`，避免跨连接复用引发 `ERR_POSTGRES_EXPECTED_STATEMENT`。
   - **管线化同步边界**：在多帧管线化序列末尾发送 `Sync` (`'S'`) 以建立事务恢复点并触发服务端 `ReadyForQuery`；若需在不断开管线的前提下提前拉取服务端响应，使用 `Flush` (`'H'`)。
3. **PostgreSQL / SQLite 结构化错误分类与重试策略（参考 `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - PostgreSQL 后端诊断帧同时携带非本地化严重度 `'V'`（PostgreSQL 9.6+ 推荐用于程序化判定 `ERROR`/`FATAL`/`PANIC`）与 5 位 `SQLSTATE` `'C'`；应用层应通过 `PgErrorNoticeView.isRetryableSqlState()`（`40001` 序列化冲突、`40P01` 死锁、`08xxx` 连接中断）驱动指数退避重试，通过 `isConstraintViolation()`（`23xxx`）精准映射业务冲突，并在 `isFatal()` 时主动摘除连接池中的失效连接。
   - SQLite 错误码以 `SQLITE_CONSTRAINT` (`errno=19`)、`SQLITE_BUSY` (`errno=5`)、`SQLITE_LOCKED` (`errno=6`) 与 `SQLITE_READONLY` (`errno=8`) 为主，高并发写入场景应结合 WAL 模式与 `SQLITE_BUSY` 退避重试。
4. **参数化绑定与防 SQL 注入**：
   - 严禁通过字符串拼接构造含外部输入的 SQL；始终通过 `SqlStatement` / `SqliteDatabase` 参数绑定或 `MySqlPreparedProtocol` / `encodeMySqlQueryWithAttrs` / PostgreSQL Extended Query 参数帧分离语句与数据；审计或读写分离路由可结合 `parseSqlWithMaxDepth`、`isReadOnlyQuery` / `isReadOnlySql` 与 `normalizeSqlFingerprint` 实现安全校验与查询脱敏。
5. **连接状态机与事务边界（参考 `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - PostgreSQL 事务内一旦单条语句出错，`ReadyForQuery` (`'Z'`) 返回 `PgTxStatus.failed_transaction` (`'E'`)，须执行 `ROLLBACK`（或回滚至 `SAVEPOINT` 以实现局部错误恢复）待状态恢复为 `idle` (`'I'`) / `in_transaction` (`'T'`) 后方可继续新查询；`SqlTransactionStateMachine`、`PgBackendKeyData`（配合 `CancelRequest`）与 `SqliteDatabase`（`begin`/`commit`/`rollback`）配合 `defer` 可确保异常路径下确定性回滚，防止连接池归还脏事务连接；跨会话分布式事务在 PostgreSQL 走 Prepared Transactions (2PC)，在 MySQL 走 XA Transactions。
5. **15 字节小对象优化（`SqlData`）**：
   - 继承自 Bun `src/sql/shared/Data.cp`，短列值直接内联存储于 15 字节槽位，显著降低宽表批量扫描时的堆分配压力。
6. **MySQL 预处理二进制协议执行帧编解码（`lib/sql/mysqlExec.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/PreparedStatement.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - **二进制参数绑定防注入与性能优势**：对齐 `Bun.SQL` MySQL 5.7+/8.0+ 预处理管线（`COM_STMT_PREPARE` -> `COM_STMT_EXECUTE` `0x17` -> `COM_STMT_CLOSE`），通过独立的 NULL 位图（`(param_count + 7) / 8` 字节，无偏移）与类型元数据头（`field_type` + `0x80` 无符号标志）传递参数，避免文本 SQL 拼接与转义开销；
   - **全类型覆盖与零拷贝往返解码**：支持 `Tiny`/`Short`/`Long`/`LongLong`（有符号/无符号小端）、IEEE-754 `Float`/`Double`、`VarString`/`String`/`Blob`/`Json`/`NewDecimal`（Length-Encoded String）与 `Null`；`encodeMySqlStmtExecute` / `decodeMySqlStmtExecuteFrame` 自动封装与校验 4 字节 MySQL 包头，严格拦截截断报文、非法命令字节、非法 `new_params_bind_flag` 与尾随垃圾字节。

```cp
import {
    MySqlExecParam,
    MySqlStmtExecuteRequest,
    encodeMySqlStmtExecute,
    decodeMySqlStmtExecuteFrame,
} from "cap:sql";

const req = MySqlStmtExecuteRequest{ statement_id: 42, iteration_count: 1, new_params_bind_flag: true };
const params: [3]MySqlExecParam = [
    MySqlExecParam.longUnsigned(1001),
    MySqlExecParam.nullParam(),
    MySqlExecParam.text("Alice"),
];
let wire: [128]u8 = [0; 128];
const n = encodeMySqlStmtExecute(req, params[0..], 0, wire[0..])?;
const decoded = decodeMySqlStmtExecuteFrame(wire[0..n], 3)?;
// decoded.statement_id == 42, decoded.isParamNull(1) == true, decoded.params[2].bytes_val == "Alice"
* = decoded;
```

7. **PostgreSQL 后端认证质询与 SCRAM-SHA-256 握手编解码（`lib/sql/pgAuthChal.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/Authentication.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - **完整覆盖 `'R'` 认证子码**：与客户端上行帧 `pgAuthCopy.cp` 对称，实现后端下行的 `AuthenticationOk` (0)、`Cleartext` (3)、`MD5Password` (5, 4 字节 salt)、`GSS`/`SSPI`、`AuthenticationSASL` (10, NUL 分隔机制列表迭代器 `PgSaslMechanismIter`)、`AuthenticationSASLContinue` (11) 与 `AuthenticationSASLFinal` (12) 零拷贝编解码；
   - **SCRAM-SHA-256 质询严格校验**：`parsePgSaslContinue` 强制要求 `r=`、`s=`、`i=` 三元组齐全且迭代次数 `i` 为严格正 `u32`；`parsePgSaslFinal` 解析服务端校验签名 `v=<base64>` 或 RFC 5802 错误 `e=<reason>`；`decodePgAuthChallenge` 强制验证各类认证码对应的消息长度不变量与零尾随字节。

```cp
import {
    PgAuthChallengeKind,
    encodePgAuthSaslContinue,
    decodePgAuthChallenge,
} from "cap:sql";

let wire: [128]u8 = [0; 128];
const n = encodePgAuthSaslContinue("r=clientnonce+servernonce,s=c2FsdA==,i=4096", wire[0..])?;
const view = decodePgAuthChallenge(wire[0..n])?;
// view.kind == PgAuthChallengeKind.SASLContinue
// view.sasl_continue.iterations == 4096, view.sasl_continue.salt_b64 == "c2FsdA=="
* = view;
```

8. **PostgreSQL 后端数据行（`'D'` DataRow）零拷贝流式编解码（`lib/sql/pgDataRow.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/DataRow.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - **精确区分 `SQL NULL` 与空字符串 `""`**：PostgreSQL `'D'` 帧使用 `0xffff_ffff`（`-1`）哨兵表示 `SQL NULL`，长度 `0` 表示非空零字节值（空串/空 `bytea`）；`decodePgDataRowIter` 返回零拷贝流式迭代器 `PgDataRowIter`（支持任意列数宽表与 `columnAt` 随机访问），`decodePgDataRowFrame` 提供最多 64 列的定长视图；
   - **严格帧安全校验**：自动拦截除 `-1` 以外的任意负列长（`InvalidColumnLength`）、列数据截断（`TruncatedFrame`）与行尾垃圾字节（`TrailingBytes`）。

```cp
import {
    PgDataRowColumn,
    encodePgDataRowFrame,
    decodePgDataRowFrame,
} from "cap:sql";

const cols: [3]PgDataRowColumn = [
    PgDataRowColumn.bytesCol("42"),
    PgDataRowColumn.nullCol(),
    PgDataRowColumn.emptyCol(),
];
let wire: [64]u8 = [0; 64];
const n = encodePgDataRowFrame(cols[0..], wire[0..])?;
const row = decodePgDataRowFrame(wire[0..n])?;
// row.column_count == 3, row.columns[1].is_null == true, !row.columns[2].is_null && row.columns[2].bytes.len == 0
* = row;
```

9. **MySQL 5.7+/8.0+ 完整字符集与 Collation 注册表（`lib/sql/mysqlCharset.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/CharacterSet.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - **完整 1..255+ Collation ID 查找与编码族分类**：零堆分配 O(1) 映射 Bun `CharacterSet` 全部 Collation ID 与 MySQL 8.0+ `utf8mb4_0900_*`（`255`/`278`/`305`/`309`），提供 `maxBytesPerChar()`（1/2/3/4 字节宽度）、`isUtf8Compatible()` 与大小写无关名称/别名解析（`lookupMySqlCollationByName`）；
   - **MySQL 协议握手与二进制列边界契约**：HandshakeResponse41 仅含 1 字节 `character_set` 字段，对于 `id > 255`（如 `309` `utf8mb4_0900_bin`）须通过 `handshakeByte()` 回落为 `255` 并在连接后按需设置 collation；判定 `ColumnDefinition41` 列是否为原始字节流（`BLOB`/`VARBINARY`）时必须用 `isMySqlBinaryColumn(id)` / `isRawBinary()`（仅 `63`），切勿与文本二进制排序 `utf8mb4_bin` (`46`) 混淆。

```cp
import {
    MYSQL_CHARSET_DEFAULT_ID,
    MYSQL_CHARSET_BINARY_ID,
    lookupMySqlCollation,
    lookupMySqlCollationByName,
    isMySqlBinaryColumn,
} from "cap:sql";

const info = lookupMySqlCollation(MYSQL_CHARSET_DEFAULT_ID); // utf8mb4_general_ci, maxBytesPerChar() == 4
const is_bin = isMySqlBinaryColumn(MYSQL_CHARSET_BINARY_ID); // true
const by_name = lookupMySqlCollationByName("utf8mb4_0900_bin"); // id == 309, handshakeByte() == 255
* = info;
* = is_bin;
* = by_name;
```

10. **MySQL 5.7+/8.0+ 会话握手、能力协商与 TLS/认证状态机（`lib/sql/mysqlSession.cp`，吸收自 `/data/dev/bun/src/sql/mysql/{ConnectionState,Capabilities,StatusFlags,SSLMode,AuthMethod}.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
    - **协议握手与能力协商**：`negotiateMySqlCapabilities()` 强制要求服务端支持 `CLIENT_PROTOCOL_41`，自动按 `MYSQL_SESSION_CAP_REJECT_MASK` 剔除 13 个不安全或客户端不支持的能力位（如 `CLIENT_LOCAL_FILES`/`CLIENT_COMPRESS`/`CLIENT_ZSTD_COMPRESSION_ALGORITHM`），并与默认客户端能力集求交集；
    - **TLS 升级与认证插件路由（参考 `/data/dev/bun/docs/runtime/sql.mdx`）**：支持 `Disable` / `Prefer` / `Require` / `VerifyCa` / `VerifyFull` 五种 TLS 模式（`Require` 及以上在服务端不支持 SSL 时立即返回 `TlsRequiredByClient`），完整覆盖 `mysql_native_password`（20B scramble）、`caching_sha2_password`（32B）与 `sha256_password`（32B），并在明文模式下支持 `AwaitingPublicKey` RSA 公钥交换及 `AuthSwitchRequest` 插件切换；
    - **服务器状态标志追踪**：通过 `MySqlSessionServerStatus` 实时感知事务态（`inTransaction()` / `isReadOnlyTransaction()`）、`isAutoCommit()`、`hasMoreResults()`、`isQuerySlow()` 与 `isNoBackslashEscapes()`。

```cp
import {
    MYSQL_SESSION_CAP_PROTOCOL_41,
    MYSQL_SESSION_CAP_SSL,
    MYSQL_SESSION_CAP_PLUGIN_AUTH,
    MYSQL_SESSION_CAP_SECURE_CONNECTION,
    MYSQL_SESSION_CAP_DEPRECATE_EOF,
    MYSQL_SESSION_STATUS_AUTOCOMMIT,
    MySqlSessionSslMode,
    MySqlSessionMachine,
} from "cap:sql";

let sess = MySqlSessionMachine.init(MySqlSessionSslMode.Require, true);
* = sess.beginConnect()?;
* = sess.onTcpConnected()?;
const caps = MYSQL_SESSION_CAP_PROTOCOL_41 | MYSQL_SESSION_CAP_SSL | MYSQL_SESSION_CAP_PLUGIN_AUTH | MYSQL_SESSION_CAP_SECURE_CONNECTION | MYSQL_SESSION_CAP_DEPRECATE_EOF;
const use_tls = sess.onServerGreeting(caps, "caching_sha2_password", MYSQL_SESSION_STATUS_AUTOCOMMIT)?;
if (use_tls) {
    * = sess.onTlsUpgraded()?;
}
* = sess.onAuthOk(MYSQL_SESSION_STATUS_AUTOCOMMIT)?;
// sess.isReadyForQuery() == true, sess.usesDeprecateEof() == true
```

11. **PostgreSQL v3.0 会话状态机、SSLRequest 协商与 CommandTag 解析（`lib/sql/pgSession.cp`，吸收自 `/data/dev/bun/src/sql/postgres/{Status,SSLMode,TLSStatus,AnyPostgresError,CommandTag}.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
    - **SSLRequest 协商与严格顺序守卫**：`decidePgTlsResponse()` 严格处理 PostgreSQL 单字节回复（`'S'`/`'N'`），`Require`/`VerifyCa`/`VerifyFull` 在服务端拒绝 SSL 时立即返回 `TlsNotAvailable`；状态机强制要求 `Pending` TLS 握手完成后才允许 `onStartupSent()`；
    - **事务块与取消密钥追踪**：记录 `BackendKeyData`（`pid > 0` 与 `secret_key`），通过 `ReadyForQuery`（`'I'`/`'T'`/`'E'`）实时追踪 `inTransaction()` 与 `needsRollback()`（失败事务块 `'E'` 必须显式 `ROLLBACK`）；
    - **CommandComplete 标签解析**：`parsePgSessionCommandTag()` 精准处理 `INSERT <oid> <rows>`（跳过 OID 提取行数）及 `DELETE`/`UPDATE`/`MERGE`/`SELECT`/`MOVE`/`FETCH`/`COPY` 等标准标签。

```cp
import {
    PgSessionSslMode,
    PgSessionCmdKind,
    PgSessionMachine,
} from "cap:sql";

let pg = PgSessionMachine.init(PgSessionSslMode.Require);
* = pg.beginConnect()?;
const use_tls = pg.onSslNegotiated('S')?;
if (use_tls) {
    * = pg.onTlsHandshakeComplete()?;
}
* = pg.onStartupSent()?;
* = pg.onBackendKeyData(4242, 987654)?;
* = pg.onReadyForQuery('I')?;
const cmd = pg.onCommandComplete("INSERT 0 5")?;
// cmd.kind == PgSessionCmdKind.Insert, cmd.row_count == 5
* = cmd;
```

12. **零分配 SQL 结果集列标识符分类与重复列阴影追踪（`lib/sql/columnId.cp`，吸收自 `/data/dev/bun/src/sql/shared/ColumnIdentifier.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
    - **导出符号**：`SQL_COLUMN_ID_MAX_COLS` (`64`)、`SQL_COLUMN_ID_MAX_INDEX` (`4294967294`)、`SqlColumnIdError`、`SqlColumnIdKind`（`name`/`index`/`duplicate`）、`SqlColumnId`（`parse`/`fromIndex`/`duplicateOf`/`isName`/`isIndex`/`isDuplicate`/`sameKey`/`eql`）、`SqlColumnIdMap`（`init`/`addColumn`/`count`/`duplicateCount`/`activeCount`/`hasDuplicates`/`get`/`rawNameAt`/`lookupByName`/`lookupByIndex`/`formatHeaderList`）；
    - **Bun SQL 结果集格式与重复列实战实践（吸收自 `runtime/sql.mdx`）**：
      - Bun SQL 客户端（PostgreSQL / MySQL / SQLite 共享层）默认以对象数组返回结果行，此时重复列名（如 `SELECT a.id, b.id FROM a JOIN b`）遵循**后列覆盖前列（last-column-wins）**规则；当存在重复列名或需要更高性能二进制输出时，应使用 `.values()`（按位置数组返回，完整保留所有重复列）或 `.raw()`（返回原始字节缓冲数组）；
      - `SqlColumnId.parse` 精确对齐 Bun 规则：将 1..10 位且值 `<= 4294967294`（`< u32::MAX`）的纯数字列名归类为 `.index` 快速数组索引键，其余（含空名、前导符号或 `>= 4294967295`）保留为零拷贝 `.name`；`SqlColumnIdMap` 在无堆分配前提下自动标记被覆写前列为 `.duplicate` 并通过 `hasDuplicates()` 提示调用方切换位置访问模式。

```cp
import { SqlColumnIdMap } from "cap:sql";

let cols = SqlColumnIdMap.init();
* = cols.addColumn("id")?;
* = cols.addColumn("name")?;
* = cols.addColumn("id")?; // 阴影覆盖第 0 列，对象模式保留第 2 列
// cols.hasDuplicates() == true, cols.lookupByName("id") == 2, cols.activeCount() == 2
```
