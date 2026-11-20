# `cap:mysql_wire` — MySQL Client/Server Wire Protocol 标准库

## 概述

`cap:mysql_wire`（`cap.mysql_wire`）是纯 CP 实现的工业级 MySQL Client/Server 二进制协议编解码与增量流式拆包标准库，涵盖 4 字节帧头与流式分帧器、Length-Encoded 原语、`HandshakeV10` / `HandshakeResponse41`、`mysql_native_password` 双 SHA-1 认证响应，以及 `OK` / `ERR` / `EOF` / `COM_*` / `ColumnDef41` / `TextResultRow` 等核心报文。

## 核心类型

| 类型 | 说明 |
| --- | --- |
| `MysqlError` | 协议解析与分帧错误枚举（`UnexpectedEof`、`PacketTooLarge` 等） |
| `LenEncInt` | Length-Encoded Integer 解码结果（`value: u64`, `is_null: bool`） |
| `MysqlPacket` | 单个已拆包的 MySQL 帧（`sequence_id: u8`, `payload: []u8`） |
| `MysqlStreamFramer` | 支持任意切片黏包/半包重组与 `max_packet_bytes` 防护的流式拆包器 |
| `HandshakeV10` | 服务端 Protocol 10 握手包（含 20 字节 scramble 与插件名） |
| `HandshakeResponse41` | 客户端 Protocol 41 认证响应包（含用户名、认证载荷、库名） |
| `OkPacket` / `ErrPacket` / `EofPacket` | 服务端响应状态包（`0x00` / `0xFF` 含 `#SQLSTATE` / `0xFE`） |
| `ComPacket` | 客户端指令包（`COM_QUERY` `0x03`、`COM_PING` `0x0E`、`COM_QUIT` `0x01`） |
| `ColumnDef41` | 结果集列元数据定义（`ColumnDefinition41`） |
| `TextResultRow` | 文本协议结果行（支持 `0xFB` NULL 列） |

## API 速查

- **Length-Encoded 原语**：`readLenEncInt` / `writeLenEncInt` / `writeLenEncNull` / `readLenEncStr` / `writeLenEncStr` / `readNullTermStr`
- **帧头与流式拆包**：`encodePacketFrame(alloc, sequence_id, payload)`、`MysqlStreamFramer.init(max_packet_bytes)`、`.feed(alloc, chunk)`、`.nextPacket(alloc)`、`.pendingBytes()`
- **认证加密**：`computeNativePasswordAuth(alloc, password, scramble_20)`（空密码返回空切片，非空返回 20 字节 `SHA1(pw) XOR SHA1(scramble || SHA1(SHA1(pw)))`）
- **指令编码**：`ComPacket.encodeComQuery(alloc, sql)`、`ComPacket.encodeComPing(alloc)`、`ComPacket.encodeComQuit(alloc)`

## 示例

```cp
import { MysqlStreamFramer, encodePacketFrame, ComPacket } from "cap:mysql_wire";

const query_payload = try ComPacket.encodeComQuery(alloc, "SELECT 1");
defer alloc.free(query_payload);
const frame = try encodePacketFrame(alloc, 0, query_payload);
defer alloc.free(frame);

let framer = MysqlStreamFramer.init(1024 * 1024);
defer framer.deinit(alloc);
try framer.feed(alloc, frame);
if (pkt := try framer.nextPacket(alloc)) {
    let owned = pkt;
    defer owned.deinit(alloc);
}
```
