# `cap:mysql` — MySQL 协议与 Wire 编解码标准库

> 参考来源：`/data/dev/bun/src/sql/mysql/`, `/data/dev/bun/docs/runtime/sql.mdx` | 吸收日期：2026-11-22

`cap:mysql` 统一收敛了 MySQL 客户端/服务端协议栈的全部纯 CP 能力，涵盖认证加盐、能力位与状态标志、控制报文解析、字段类型与列定义、握手响应、数据包帧读写以及文本结果集编解码。

## 目录结构

- `lib/mysql/auth.cp`：MySQL 认证插件与加盐（`mysql_native_password`、`caching_sha2_password`、明文密码、XOR nonce）及 `SSLRequest` 帧编码
- `lib/mysql/caps.cp`：零堆分配 32-bit 能力位集合（`CapabilityFlag` / `CapsCapabilities`）与 16-bit 状态标志（`StatusFlag` / `CapsStatusFlags`）
- `lib/mysql/control.cp`：零分配控制报文解析与命令类型（`CommandType`、`HandshakeV10`、`OKPacket`、`ErrorPacket`、`EOFPacket`、`AuthSwitchRequest`）
- `lib/mysql/fieldtypes.cp`：MySQL 字段类型（`FieldType`）、列标志（`ColumnFlags`）、长度编码整数/字符串及 `ColumnDefinition41` 编解码
- `lib/mysql/handshake.cp`：`HandshakeResponse41` 与连接属性（`ConnectAttr`）完整编解码器
- `lib/mysql/packet.cp`：MySQL 4 字节包头与流式包读写器（`PacketHeader`、`MySqlPacketReader`、`MySqlPacketWriter`）
- `lib/mysql/proto.cp`：32-bit 协议能力常量（`CAP_CLIENT_*`）、结构化 `Capabilities`、`StatusFlags` 与完整 `CharacterSet` Collation 枚举
- `lib/mysql/wire.cp`：带分配器的全功能 Wire 帧组装（`MysqlStreamFramer`、`WireHandshakeV10`、`WireHandshakeResponse41`、`OkPacket`/`ErrPacket`/`EofPacket`/`ComPacket`、`ColumnDef41`、`TextResultRow`）
- `lib/mysql/localInfile.cp`：零分配 `LOCAL INFILE Request`（`0xFB`）编解码与客户端路径穿越策略防护
- `lib/mysql/stackReader.cp`：零拷贝有界协议游标与原子半包回滚帧读取器（`StackReader`、`MysqlFrameSlice`）
- `lib/mysql/stmtPrepareOk.cp`：零分配 `COM_STMT_PREPARE_OK` 预处理响应编解码与预处理语句生命周期帧编码（`StmtPrepareOKPacket`、`StmtCommandCodec`）
- `lib/mysql/mysqlError.cp`：统一 MySQL 协议错误集 `AnyMySqlError`、错误分类器（`MySqlErrorKind` / `MySqlErrorCategory`）、重试与连接重置策略及服务端 `errno` 映射
- `lib/mysql/index.cp`：`cap:mysql` 统一聚合导出入口

## 核心导出摘要

| 领域 | 核心类型 / 函数 / 常量 |
| :--- | :--- |
| 认证 & SSL (`auth.cp`) | `AuthMethod`, `AuthFastStatus`, `AuthError`, `SSLRequest`, `scrambleNativePassword`, `scrambleCachingSha2Password`, `encodeCleartextPassword`, `xorPasswordWithNonce` |
| 能力位 & 状态位 (`caps.cp`) | `CapabilityFlag`, `StatusFlag`, `CapsCapabilities`, `CapsStatusFlags` |
| 控制报文 (`control.cp`) | `ControlError`, `CommandType`, `HandshakeV10`, `OKPacket`, `ErrorPacket`, `EOFPacket`, `AuthSwitchRequest` |
| 字段 & 列定义 (`fieldtypes.cp`) | `FieldType`, `fieldTypeFromByte`, `ColumnFlags`, `MySqlWireError`, `MySqlWireBuffer`, `ColumnDefinition41`, `encodeColumnDefinition41`, `decodeColumnDefinition41`, `encodeLenEncInt`/`decodeLenEncInt` |
| 握手响应 (`handshake.cp`) | `HandshakeError`, `ConnectAttr`, `HandshakeResponse41`, `EncodedHandshake`, `DecodedHandshake41`, `encodeHandshakeResponse41`, `decodeHandshakeResponse41` |
| 包读写器 (`packet.cp`) | `PACKET_HEADER_SIZE`, `MAX_PACKET_PAYLOAD`, `MySqlPacketError`, `PacketHeader`, `DecodedLenInt`, `decodeLenInt`, `MySqlPacketReader`, `PacketHandle`, `MySqlPacketWriter` |
| 协议常量 & 字符集 (`proto.cp`) | `CAP_CLIENT_*` (32 项), `Capabilities`, `StatusFlags`, `CharacterSet` |
| Wire 帧 & 结果集 (`wire.cp`) | `MysqlStreamFramer`, `MysqlPacket`, `MysqlError`, `encodePacketFrame`, `WireHandshakeV10`, `WireHandshakeResponse41`, `computeNativePasswordAuth`, `OkPacket`, `ErrPacket`, `EofPacket`, `ComPacket`, `ColumnDef41`, `CellValue`, `TextResultRow` |
| LOCAL INFILE (`localInfile.cp`) | `LOCAL_INFILE_HEADER`, `LOCAL_INFILE_MAX_FILENAME`, `LocalInfileError`, `LocalInfileRequest` |
| 有界游标 (`stackReader.cp`) | `StackReaderError`, `MysqlFrameSlice`, `StackReader` |
| 预处理语句响应 (`stmtPrepareOk.cp`) | `STMT_PREPARE_OK_STATUS`, `STMT_PREPARE_OK_MIN_LEN`, `STMT_PREPARE_OK_STD_LEN`, `STMT_PREPARE_OK_META_LEN`, `COM_STMT_PREPARE_CMD`, `COM_STMT_CLOSE_CMD`, `COM_STMT_RESET_CMD`, `StmtPrepareOkError`, `StmtPrepareOKPacket`, `StmtCommandCodec` |
| 统一错误分类 (`mysqlError.cp`) | `AnyMySqlError`, `MySqlErrorCategory`, `MySqlErrorKind` |

## 使用示例

```cp
import {
    Capabilities,
    CAP_CLIENT_SSL,
    AuthMethod,
    SSLRequest,
    HandshakeV10,
    FieldType,
    MySqlPacketWriter,
} from "cap:mysql";

fn demo() {
    const caps = Capabilities.defaultClient(true, true);
    const has_ssl = caps.hasFlag(CAP_CLIENT_SSL);
    const method = AuthMethod.fromPluginName("caching_sha2_password");
    * = has_ssl;
    * = method;
}
```

---

## 上游 Bun MySQL 协议契约与安全边界

1. **MySQL 8.0 `caching_sha2_password` 与 `AuthSwitchRequest` 握手**：
   - `HandshakeV10` 返回服务端 20 字节认证盐（`MIN_SALT_LEN`）与默认插件名；若服务端要求切换插件则返回 `AuthSwitchRequest`，客户端需按目标插件选用 `scrambleCachingSha2Password`（SHA-256）或 `scrambleNativePassword`（SHA-1），明文传输仅应在 `SSLRequest` TLS 升级完成后进行。
2. **16 MiB 单包上限与序号回绕（`MAX_PACKET_PAYLOAD`）**：
   - MySQL 4 字节包头采用 24-bit 小端长度（最大 `0xFFFFFF = 16 MiB - 1`）+ 8-bit 递增序号 `seq_id`；单条载荷超过 `MAX_PACKET_PAYLOAD` 时须分多包发送并以不足 16 MiB 的末包收尾。
3. **长度编码整数与 NULL 哨兵（`decodeLenEncInt`）**：
   - 在文本结果集 `TextResultRow` 中，`0xFB` 表示 SQL `NULL`（区别于空字符串 `0x00`），`0xFC`/`0xFD`/`0xFE` 分别引导 2/3/8 字节无符号长度。
4. **`LOCAL INFILE Request`（`0xFB`）客户端路径策略防护（`localInfile.cp`）**：
   - 吸收自 `/data/dev/bun/src/sql/mysql/protocol/LocalInfileRequest.cp`（参考 `/data/dev/bun/docs/runtime/sql.mdx` MySQL 协议安全边界）：在 `COM_QUERY` 响应阶段，首字节 `0xFB` 表示服务端请求客户端上传指定路径文件；
   - **安全铁律**：默认禁用 `CLIENT_LOCAL_FILES`（`allow_local_files = false`），即使显式启用也必须先调用 `req.checkPolicy(true, false)` 拦截 `..` 路径遍历、绝对路径与内嵌 NUL 字节，数据流发送完毕后使用 `LocalInfileRequest.encodeEmptyTerminatorFrame(seq_id, out)` 发送 4 字节空帧结束传输。

```cp
import { LocalInfileRequest } from "cap:mysql";

const req = LocalInfileRequest.init("imports/users.csv", 1);
* = req.checkPolicy(true, false)?;
let frame: [64]u8 = [0; 64];
const n = req.encodeFrame(frame[0..])?;
const decoded = LocalInfileRequest.decodeFrame(frame[0..n])?;
* = decoded.isSafeRelativeFilename(); // true
```

5. **有界零分配协议游标（`stackReader.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/StackReader.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - 面向 MySQL 5.7+ / 8.0+ 线缆流式解包场景提供零拷贝 `StackReader`：支持消息起点书签（`markMessageStart`/`rewindMessage`/`setOffsetFromStart`）、双向饱和跳过（`skip`）、NUL 终止字符串（`readZ`）、小端定长整数（`readU8`..`readU64Le`）以及 Length-Encoded 整数/字符串解码；
   - `readPacketFrame()` 自动执行原子半包回滚：若网络流缓冲区尚未收齐完整载荷，自动回退游标至 `message_start` 并返回 `StackReaderError.ShortRead`，等待下一次网络读就绪后无损续解。

```cp
import { StackReader } from "cap:mysql";

const wire: [7]u8 = [3, 0, 0, 1, 'O', 'K', '!'];
let r = StackReader.init(wire[0..]);
const frame = r.readPacketFrame()?; // payload_len=3, sequence_id=1, payload="OK!"
* = frame;
```

6. **MySQL 预处理语句二进制协议与流水线（`stmtPrepareOk.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/StmtPrepareOKPacket.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - 依据 Bun SQL 文档（`/data/dev/bun/docs/runtime/sql.mdx`）的 MySQL 最佳实践：参数化查询自动走 **Prepared Statements + Binary Protocol**，可同时获得语句缓存、精确二进制类型编码以及多语句 **Query Pipelining** 吞吐优势；
   - `StmtPrepareOKPacket` 完整兼容 10 字节传统包（无 `warning_count`）、12 字节标准包以及 MySQL 8.0+ 13 字节可选元数据标志包（`CLIENT_OPTIONAL_RESULTSET_METADATA` / `metadata_follows`）；
   - 通过 `pkt.expectedFollowupPackets(deprecate_eof)` 可零分配计算出服务端紧随其后发送的参数定义包、列定义包与可选 EOF 包总数，配合 `StmtCommandCodec.encodePrepareFrame` / `encodeCloseFrame` / `encodeResetFrame` 管理完整预处理语句生命周期（注意：不再复用的语句应及时发送 `COM_STMT_CLOSE` 释放服务端句柄，避免触发 `max_prepared_stmt_count` 上限）。

```cp
import { StmtPrepareOKPacket, StmtCommandCodec } from "cap:mysql";

let req_buf: [128]u8 = [0; 128];
const req_n = StmtCommandCodec.encodePrepareFrame("SELECT id FROM users WHERE id = ?", 0, req_buf[0..])?;
* = req_n;

const ok = StmtPrepareOKPacket.initWithMetadata(42, 1, 1, 0, true);
const followup = ok.expectedFollowupPackets(true); // CLIENT_DEPRECATE_EOF: 1 param + 1 col = 2
* = followup;
```

7. **统一 MySQL 错误分类、重试与连接重置策略（`mysqlError.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/AnyMySQLError.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - `AnyMySqlError` 收敛 31 项纯协议/驱动错误；`MySqlErrorKind` 提供标准化错误码（`ERR_MYSQL_*`）、可读诊断消息、7 大错误类别（`Connection`/`Timeout`/`Auth`/`Protocol`/`Query`/`Resource`/`State`）及 `fromServerErrno(errno_code)` 服务端错误码映射；
   - **连接池自愈准则**：`isRetryable()` 仅对断连/超时/半包返回 `true`；若触发 `requiresConnectionReset()`（如畸形包、`CR_COMMANDS_OUT_OF_SYNC` 等线缆失步错误），必须立即销毁当前连接而非归还连接池。

```cp
import { MySqlErrorKind, MySqlErrorCategory } from "cap:mysql";

const k = MySqlErrorKind.fromServerErrno(1045);
// k == MySqlErrorKind.AuthenticationFailed, k.category() == MySqlErrorCategory.Auth
* = k.code();
* = MySqlErrorCategory.Auth;
```

8. **结果集头、查询响应首包判别与认证切换响应帧（`resultSetHeader.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/ResultSetHeader.cp`）**：
   - `classifyQueryResponse(payload)` 严格按 MySQL 线协议规则区分 `Ok`（`0x00` 且 `len >= 7`）、`Error`（`0xFF`）、`LocalInfile`（`0xFB`）、`Eof`（`0xFE` 且 `len < 9`）与 `ResultSetHeader`；
   - `MySqlResultSetHeader` 零堆分配编解码 1/3/4/9 字节 `lenenc-int` 列计数，并完整支持 MySQL 8.0+ `CLIENT_OPTIONAL_RESULTSET_METADATA`（`metadata_follows = 0 | 1`），通过 `expectedColumnDefPackets()` 计算后续 `ColumnDefinition41` 包数；
   - `AuthSwitchResponseCodec` 零堆分配编解码客户端 `AuthSwitchResponse` 4 字节帧头 + 认证响应负载。

```cp
import {
    classifyQueryResponse,
    MySqlResponsePacketKind,
    MySqlResultSetHeader,
    AuthSwitchResponseCodec,
} from "cap:mysql";

let buf: [64]u8 = [0; 64];
const hdr = MySqlResultSetHeader.initWithMetadata(12, true);
const n = hdr.encodeFrame(1, buf[0..])?;
const decoded = MySqlResultSetHeader.decodeFrame(buf[0..n], true)?;
* = decoded.expectedColumnDefPackets(); // 12

const kind = classifyQueryResponse(buf[4..n])?;
* = kind == MySqlResponsePacketKind.ResultSetHeader;
const auth_n = AuthSwitchResponseCodec.encodeFrame("scrambled_20_bytes!!", 3, buf[0..])?;
* = auth_n;
```

9. **`COM_QUERY`（简单与参数化 NULL-bitmap 绑定）与单字节会话命令帧（`queryPacket.cp`，吸收自 `/data/dev/bun/src/sql/mysql/protocol/Query.cp`）**：
   - `MySqlQueryPacket` 支持无参 `COM_QUERY` 与携带最多 32 个 `MySqlQueryParam`（`nullParam` / `varString` / `fixedBinary`）的零堆分配参数化帧编码，自动计算并写入 NULL-bitmap（`mysqlNullBitmapLen` / `isNullInBitmap`）、参数类型+无符号标志位（`0x80`）、`:p1`..`:p32` 命名属性以及固定二进制 / `lenenc-str` 参数值；
   - 配套 `decodeSimpleFrame`、`encodeInitDbFrame`（`COM_INIT_DB`）与 `encodeSingleCmdFrame`（`COM_PING` / `COM_QUIT`）。

```cp
import {
    MySqlQueryPacket,
    MySqlQueryParam,
    MYSQL_FTYPE_LONG,
    MYSQL_CMD_PING,
} from "cap:mysql";

let buf: [256]u8 = [0; 256];
let q = MySqlQueryPacket.init("SELECT * FROM users WHERE id = ? AND email = ?");
const id_le: [4]u8 = [42, 0, 0, 0];
* = q.addParam(MySqlQueryParam.fixedBinary(MYSQL_FTYPE_LONG, true, id_le[0..]))?;
* = q.addParam(MySqlQueryParam.varString("alice@example.com"))?;
const n = q.encodeFrame(0, buf[0..])?;
const n_ping = MySqlQueryPacket.encodeSingleCmdFrame(MYSQL_CMD_PING, 0, buf[0..])?;
* = n; * = n_ping;
```
