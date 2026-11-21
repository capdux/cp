# `cap:mysql` — MySQL 协议与 Wire 编解码标准库

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
