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
- **MySQL 变长整数与长度前缀字符串编解码 (`mysql_lenenc.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/EncodeInt.cp`)**：
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
- **MySQL `ERR_Packet` 编解码与错误分类 (`mysql_error_packet.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/ErrorPacket.cp`)**：
  - 常量与错误集：`MYSQL_ERR_PACKET_HEADER` (`0xff`)、`MYSQL_SQL_STATE_MARKER` (`'#'`)、`MySqlErrorPacketError`（`InvalidHeader` / `TruncatedPacket` / `BufferTooSmall` / `InvalidSqlState`）
  - 结构体与方法：`MySqlErrorPacket`（`initWithSqlState` / `initLegacy` / `decode` / `encode` / `encodedSize` / `sqlStateSlice` / `sqlStateClass` / `isRetryable`，兼容 MySQL 4.1+ `#SQLSTATE` 与 4.0 旧版错误包，零拷贝提取错误消息）

```typescript
import { MySqlErrorPacket } from "cap:sql";

const pkt = MySqlErrorPacket.initWithSqlState(1213, "40001", "Deadlock found")?;
let wire: [64]u8 = [0; 64];
const n = pkt.encode(wire[0..])?;
const decoded = MySqlErrorPacket.decode(wire[0..n])?; // error_code=1213, isRetryable()==true
```
- **PostgreSQL 线协议零堆分配二进制帧写入器 (`pg_wire_writer.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/NewWriter.cp`)**：
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
- **MySQL 线协议零堆分配二进制读取器 (`mysql_wire_reader.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/NewReader.cp`)**：
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
- **MySQL 线协议零堆分配二进制包写入器 (`mysql_wire_writer.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/NewWriter.cp`)**：
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
- **PostgreSQL 线协议零堆分配二进制读取器 (`pg_wire_reader.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/NewReader.cp`)**：
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
- **MySQL `COM_QUERY` 与 8.0.23+ Query Attributes 零分配编解码器 (`mysql_query.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/Query.cp`)**：
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
- **PostgreSQL v3 `StartupMessage` / `SSLRequest` / `CancelRequest` 零分配编解码器 (`pg_startup.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/StartupMessage.cp`)**：
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
- **PostgreSQL v3 `RowDescription` (`'T'`) 与 `FieldDescription` 零分配流式编解码器 (`pg_row_desc.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/RowDescription.cp` & `FieldDescription.cp`)**：
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
- **MySQL `OK_Packet` (`0x00`/`0xFE`)、`EOF_Packet` (`0xFE`) 与 16 位服务器状态标志编解码器 (`mysql_ok_eof.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/{OKPacket,EOFPacket}.cp` & `StatusFlags.cp`)**：
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
- **PostgreSQL v3 Extended Query 协议零分配编解码器 (`pg_extended.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp`)**：
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
- **PostgreSQL v3 `ErrorResponse` (`'E'`) 与 `NoticeResponse` (`'N'`) 诊断帧零分配编解码器 (`pg_error_notice.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{ErrorResponse,NoticeResponse,FieldMessage,FieldType}.cp`)**：
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
- **PostgreSQL v3 后端生命周期控制帧零分配编解码器 (`pg_control.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/{ReadyForQuery,BackendKeyData,CommandComplete,ParameterStatus,TransactionStatusIndicator}.cp`)**：
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
