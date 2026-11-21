# `cap:quic_header` — 纯 CP QUIC 报文头编解码、QPACK 头分类与 Socket 类型枚举库

## 概述

`cap:quic_header` 吸收自 Bun `src/uws_sys/quic/Header.cp` 与 `src/uws_sys/SocketKind.cp`，并补全 RFC 9000 QUIC 传输层长/短头编解码与变长整数（VarInt）引擎，提供零分配、强校验的 QUIC 协议头解析、HTTP/3 QPACK 静态头分类与 `SocketKind` 传输类型判断。

## 核心类型与 API

### QUIC 常量与错误集

- `QUIC_VERSION_1: u32 = 0x00000001`（RFC 9000）
- `QUIC_VERSION_2: u32 = 0x6b3343cf`（RFC 9369）
- `QUIC_VERSION_NEGOTIATION: u32 = 0x00000000`
- `QUIC_MAX_CID_LEN: usize = 20`
- `QUIC_VARINT_MAX: u64 = 4611686018427387903`
- `QuicHeaderError`：`UnexpectedEof` / `InvalidFixedBit` / `InvalidCidLength` / `VarIntTooLarge` / `BufferTooSmall` / `InvalidSocketKind`

### QUIC 变长整数（RFC 9000 §16）

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

## 示例

```cp
import {
    QUIC_VERSION_1,
    QuicPacketKind,
    QuicHeader,
    encodeQuicHeader,
    decodeQuicHeader,
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
const tls = SocketKind.uws_http_tls.isTLS(); // true
```
