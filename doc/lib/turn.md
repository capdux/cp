# `cap:turn` — RFC 8656 TURN 线协议、ChannelData 帧与 XOR-ADDRESS 零分配编解码库

> **模块路径**: `lib/turn/`  
> **导入协议**: `import { TURN_UDP_PORT, TURNS_TLS_PORT, TURN_MAGIC_COOKIE, METHOD_ALLOCATE, parseTurnMessage, decodeXorAddr, encodeXorAddr, parseChannelData, encodeChannelData } from "cap:turn";`  
> **动态挂载**: `import cap from "cap";` -> `cap.turn.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:turn` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 8656（TURN: Traversal Using Relays around NAT）线协议解析与编码库：

1. **STUN/TURN 报文头与属性零拷贝解析 (`isTurnMessage` / `parseTurnMessage` / `TurnAttrIter`)**：
   - 严格校验前 2 位为 `00`、`Magic Cookie == 0x2112A442`、载荷长度 4 字节对齐且 `20 + length <= data.len`。
   - 完整解算 12-bit Method（`ALLOCATE` / `REFRESH` / `SEND` / `DATA` / `CREATE_PERMISSION` / `CHANNEL_BIND`）与 2-bit Message Class（Request / Indication / Success / Error）。
   - `TurnAttrIter` 自动按 4 字节边界跳过 TLV 属性填充字节。
2. **IPv4 & IPv6 `XOR-ADDRESS` 编解码 (`decodeXorAddr` / `encodeXorAddr`)**：
   - 严格遵循 RFC 5389 §15.2 / RFC 8656：端口与 `0x2112` 异或；IPv4 地址与 Magic Cookie 异或；IPv6 地址前 4B 与 Magic Cookie 异或、后 12B 与 96-bit `tx_id` 异或。
3. **ChannelData 帧编解码 (`isChannelData` / `parseChannelData` / `encodeChannelData`)**：
   - 支持 RFC 8656 §12.4 轻量级 4 字节头封装，严格校验通道号范围 `0x4000..0x7FFF` 与载荷长度边界。

---

## 二、 核心 API 速查表 (`lib/turn/turn.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 基础常量 | `TURN_UDP_PORT (3478)`, `TURNS_TLS_PORT (5349)`, `TURN_MAGIC_COOKIE (0x2112A442)`, `TURN_HEADER_SIZE (20)`, `CHANNEL_DATA_HEADER_SIZE (4)`, `CHANNEL_MIN (0x4000)`, `CHANNEL_MAX (0x7FFF)` |
| Method / Class / Transport | `METHOD_ALLOCATE (0x003)` .. `METHOD_CHANNEL_BIND (0x009)`；`CLASS_REQUEST (0)` .. `CLASS_ERROR (3)`；`TRANSPORT_UDP (17)`, `TRANSPORT_TCP (6)` |
| Attribute Type | `ATTR_ERROR_CODE`, `ATTR_CHANNEL_NUMBER`, `ATTR_LIFETIME`, `ATTR_XOR_PEER_ADDRESS`, `ATTR_DATA`, `ATTR_XOR_RELAYED_ADDRESS`, `ATTR_REQUESTED_ADDR_FAMILY`, `ATTR_EVEN_PORT`, `ATTR_REQUESTED_TRANSPORT`, `ATTR_DONT_FRAGMENT`, `ATTR_RESERVATION_TOKEN` |
| `TurnHeader` | `{ msg_type, method, msg_class, length, tx_id }`；含 `isRequest()`, `isIndication()`, `isSuccess()`, `isError()` |
| `TurnAttr` / `TurnAttrIter` | 零拷贝 TLV 属性与 4B 对齐迭代器（含 `hasError()`） |
| `TurnMessage` | `{ header, attrs_bytes }`；含 `attrs()`, `findAttr(t)`, `lifetime()`, `requestedTransport()`, `channelNumber()`, `dataAttr()`, `xorRelayedAddress()`, `xorPeerAddress()` |
| `TurnXorAddr` | `{ is_ipv6: bool, port: u16, addr: [16]u8 }`；配合 `decodeXorAddr(value, tx_id)` 与 `encodeXorAddr(addr, tx_id, out)` |
| `TurnChannelData` | `{ channel, length, payload }`；配合 `isChannelData`, `parseChannelData`, `encodeChannelData` |
| `isTurnMessage` / `parseTurnMessage` | 校验并零拷贝解析 TURN 控制报文 |

---

## 三、 使用示例

```typescript
import { parseTurnMessage, encodeChannelData, parseChannelData } from "cap:turn";

fn demo(pkt: []const u8) {
    if (msg := parseTurnMessage(pkt)) {
        if (lt := msg.lifetime()) {
            * = lt;
        }
        if (relayed := msg.xorRelayedAddress()) {
            * = relayed.port;
        }
    }

    let frame_buf: [64]u8 = [0; 64];
    if (n := encodeChannelData(0x4000, "hello", frame_buf[0..])) {
        if (cd := parseChannelData(frame_buf[0..n])) {
            // cd.channel == 0x4000, cd.payload == "hello"
            * = cd;
        }
    }
}
```
