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
