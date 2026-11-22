# `cap:quic_header` — 纯 CP QUIC 报文头编解码、QPACK 头分类与 Socket 类型枚举库

> 参考来源：/data/dev/bun/src/uws_sys/quic/Header.cp, /data/dev/bun/src/uws_sys/SocketKind.cp | 吸收日期：2026-11-22

## 概述

`cap:quic_header` 吸收自 Bun 上游 HTTP/3 与传输层基础模块（`src/uws_sys/quic/Header.cp` 与 `src/uws_sys/SocketKind.cp`），并完整补全了 RFC 9000 QUIC 传输层长/短头编解码与变长整数（VarInt）引擎。整个模块零堆分配、强边界校验，适用于 QUIC 数据包路由、HTTP/3 QPACK 静态表头分类以及多协议传输 Socket 类型分派。

## 上游架构与行为契约

- **RFC 9114 HTTP/3 逐跳头拦截（吸收自 Bun `Header.cp`）**：Bun 在构建 HTTP/3 请求/响应头时，通过 `Qpack.classify`（大小写不敏感匹配）将头部划分为三类：
  - **禁止转发的逐跳头（`forbidden`）**：`connection`、`keep-alive`、`proxy-connection`、`transfer-encoding`、`upgrade`（HTTP/3 中携带此类字段属于协议错误，必须在发送前剔除或拒绝）；
  - **主机头（`host`）**：HTTP/3 中 `Host` 需映射或协同 `:authority` 伪头部（QPACK 静态索引 `0`）处理；
  - **静态表索引头（`indexed`）**：覆盖 29 个高频 HTTP/3 静态表字段（`accept`、`accept-encoding`、`authorization`、`cache-control`、`content-type`、`cookie`、`etag`、`set-cookie`、`user-agent`、`x-forwarded-for` 等），直接命中 RFC 9204 静态表索引以压缩线上传输开销。
- **传输层 `SocketKind` 分类（吸收自 Bun `SocketKind.cp`）**：保留 Bun 统一的 23 种底层 Socket 标识枚举，涵盖通用 TCP/TLS、HTTP/WebSocket 客户端与服务端、以及 PostgreSQL / MySQL / Valkey 数据库连接与 `spawn_ipc`，支持 `O(1)` 识别 TLS、监听态、HTTP、WebSocket 与数据库通道。
- **RFC 9000 变长整数与报文头边界**：
  - 变长整数采用高 2 位编码长度前缀（`00`=1B，`01`=2B，`10`=4B，`11`=8B），最大合法值为 $2^{62} - 1$（`QUIC_VARINT_MAX = 4611686018427387903`），超限返回 `QuicHeaderError.VarIntTooLarge`；
  - 长头（`Initial`/`ZeroRTT`/`Handshake`/`Retry`/`VersionNegotiation`）与 1-RTT 短头均校验 QUIC Fixed Bit（第 2 高位必须为 1，Version Negotiation 除外），不合法时返回 `QuicHeaderError.InvalidFixedBit`；
  - DCID / SCID 长度严格限制在 `<= QUIC_MAX_CID_LEN`（20 字节），超长返回 `QuicHeaderError.InvalidCidLength`；
  - 短头不含 DCID 长度字段，解码时必须由调用方显式传入连接上下文的 `short_dcid_len`。

## 核心类型与 API

### QUIC 常量与错误集

- `QUIC_VERSION_1: u32 = 0x00000001`（RFC 9000）
- `QUIC_VERSION_2: u32 = 0x6b3343cf`（RFC 9369）
- `QUIC_VERSION_NEGOTIATION: u32 = 0x00000000`
- `QUIC_MAX_CID_LEN: usize = 20`
- `QUIC_VARINT_MAX: u64 = 4611686018427387903`
- `QuicHeaderError`：`UnexpectedEof` / `InvalidFixedBit` / `InvalidCidLength` / `VarIntTooLarge` / `BufferTooSmall` / `InvalidSocketKind`

### QUIC 变长整数（RFC 9000 §16）

- `DecodedVarInt`：解码结果结构体（包含值与消耗字节数）
- `varIntEncodedLen(value: u64): usize ! QuicHeaderError`
- `encodeVarInt(buf: []u8, value: u64): usize ! QuicHeaderError`
- `decodeVarInt(data: []const u8): DecodedVarInt ! QuicHeaderError`

### QUIC 报文头编解码（RFC 9000 §17）

- `QuicPacketKind`：`Initial` / `ZeroRTT` / `Handshake` / `Retry` / `VersionNegotiation` / `Short`（附 `isLongHeader()` / `isShortHeader()`）
- `QuicHeader`：支持长头（版本号、DCID、SCID、Initial Token、包号长度）与 1-RTT 短头（`spin_bit`、`key_phase`、DCID、包号长度）
- `encodeQuicHeader(buf: []u8, hdr: QuicHeader): usize ! QuicHeaderError`
- `decodeQuicHeader(data: []const u8, short_dcid_len: usize): QuicHeader ! QuicHeaderError`

### HTTP/3 QPACK 静态表头分类与 `SocketKind`

- `QpackHeaderClass` / `QpackIndex` / `QpackLookup` + `classifyQpackHeader(name: string): QpackLookup`
- `SocketKind`（23 种 Socket 类型，附 `isValid()` / `isTLS()` / `isListener()` / `isWebSocket()` / `isHttp()` / `isDatabase()` / `toByte()`）+ `socketKindFromByte(b: u8)`

## 边界与安全注意事项

1. **防截断与越界解码**：`decodeQuicHeader` 与 `decodeVarInt` 在每一步读取前均严格校验剩余切片长度，任何截断报文都会安全返回 `QuicHeaderError.UnexpectedEof` 而不会越界。
2. **输出缓冲不足防护**：`encodeQuicHeader` 与 `encodeVarInt` 写入前检查目标缓冲容量，不足时返回 `QuicHeaderError.BufferTooSmall`。
3. **版本协商包特例**：`QUIC_VERSION_NEGOTIATION`（`0x00000000`）包允许更灵活的固定位，业务层应通过 `isSupportedVersion()` 判断是否为 RFC 9000 / RFC 9369 受支持版本。
4. **未知字节转 `SocketKind`**：外部反序列化或 FFI 传入的字节值应始终通过 `socketKindFromByte(b)` 校验，非法枚举值返回 `QuicHeaderError.InvalidSocketKind`。

## 示例

```cp
import {
    QUIC_VERSION_1,
    QuicPacketKind,
    QuicHeader,
    encodeQuicHeader,
    decodeQuicHeader,
    classifyQpackHeader,
    SocketKind,
} from "cap:quic_header";

let buf: [64]u8 = undefined;
const dcid: [4]u8 = [0x01, 0x02, 0x03, 0x04];
const scid: [2]u8 = [0xAA, 0xBB];
const n = encodeQuicHeader(buf[0..], QuicHeader{
    kind: QuicPacketKind.Initial,
    is_long: true,
    version: QUIC_VERSION_1,
    dcid: dcid[0..],
    scid: scid[0..],
    packet_number_len: 2,
})?;
const parsed = decodeQuicHeader(buf[0..n], 0)?;
// parsed.kind == QuicPacketKind.Initial && parsed.isSupportedVersion()

// 校验 HTTP/3 头部是否属于逐跳禁止头或命中 QPACK 静态索引
const q = classifyQpackHeader("Transfer-Encoding");
// q.class == QpackHeaderClass.Forbidden

const tls = SocketKind.uws_http_tls.isTLS(); // true
```
