# `cap:gre` — 零分配 RFC 2784 / 2890 / 2637 GRE 隧道协议编解码库

> **模块路径**：`lib/gre/`（入口 `lib/gre/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 RFC 2784 / 2890 标准 GRE v0（可选 Checksum / Key / Sequence 字段）、RFC 2637 PPTP Enhanced GRE v1（`payload_len` / `call_id` / `seq_num` / `ack_num`）以及 RFC 1071 16-bit 一补码校验和计算与验证。

---

## 一、导入方式

```cp
import {
    GRE_IP_PROTO,
    GRE_MIN_HEADER_SIZE,
    GRE_FLAG_CHECKSUM,
    GRE_FLAG_ROUTING,
    GRE_FLAG_KEY,
    GRE_FLAG_SEQ,
    GRE_FLAG_STRICT_ROUTE,
    GRE_FLAG_ACK,
    GRE_PROTO_IPV4,
    GRE_PROTO_IPV6,
    GRE_PROTO_TEB,
    GRE_PROTO_MPLS_UC,
    GRE_PROTO_PPP,
    GrePacket,
    greChecksum,
    verifyGreChecksum,
    parseGre,
    encodeGre,
} from "cap:gre";
```

---

## 二、核心常量与数据结构

| 符号 | 说明 |
| :--- | :--- |
| 基础常量 | `GRE_IP_PROTO = 47`, `GRE_MIN_HEADER_SIZE = 4` |
| 报头标志 | `GRE_FLAG_CHECKSUM (0x8000)`, `GRE_FLAG_ROUTING (0x4000)`, `GRE_FLAG_KEY (0x2000)`, `GRE_FLAG_SEQ (0x1000)`, `GRE_FLAG_STRICT_ROUTE (0x0800)`, `GRE_FLAG_ACK (0x0080)` |
| 协议类型 | `GRE_PROTO_IPV4 (0x0800)`, `GRE_PROTO_IPV6 (0x86DD)`, `GRE_PROTO_TEB (0x6558)`, `GRE_PROTO_MPLS_UC (0x8847)`, `GRE_PROTO_PPP (0x880B)` |
| `GrePacket` | `flags_ver`, `version`, `protocol_type`, `checksum: u16?`, `key: u32?`, `seq_num: u32?`, `ack_num: u32?`, `payload_len`, `call_id`, `header_bytes`, `payload`; `hasChecksum()`, `hasKey()`, `hasSeq()`, `hasAck()` |

---

## 三、核心 API

- `greChecksum(data: []const u8): u16` — 计算 RFC 1071 16-bit 一补码校验和。
- `verifyGreChecksum(packet: []const u8): bool` — 校验携带 `GRE_FLAG_CHECKSUM` 的完整 GRE 报文校验和是否为 `0`。
- `parseGre(data: []const u8): GrePacket?` — 解析 v0 标准 GRE 与 v1 PPTP Enhanced GRE 报文。
- `encodeGre(protocol_type, include_checksum, key, seq_num, inner_payload, out): usize?` — 零分配组装 v0 GRE 报文并自动回填 RFC 1071 校验和。

---

## 四、使用示例

```cp
import { GRE_PROTO_IPV4, encodeGre, parseGre, verifyGreChecksum } from "cap:gre";

fn roundtripGre() {
    let buf: [128]u8 = [0; 128];
    if (n := encodeGre(GRE_PROTO_IPV4, true, 0xCAFEBABE, 42, "inner-ip-packet", buf[0..128])) {
        const pkt = buf[0..n];
        if (verifyGreChecksum(pkt)) {
            if (g := parseGre(pkt)) {
                * = g.key;
                * = g.seq_num;
                * = g.payload;
            }
        }
    }
}
```
