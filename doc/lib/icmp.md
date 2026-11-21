# `cap:icmp` — RFC 792 ICMPv4 & RFC 4443 ICMPv6 零分配编解码库

纯 CP 实现、零堆分配的 ICMPv4 / ICMPv6 报文解析、编码与 RFC 1071 一补码校验和计算/校验库。

## 导入

```cp
import {
    ICMP_HEADER_SIZE,
    ICMP_IP_PROTO_V4,
    ICMP_IP_PROTO_V6,
    ICMP4_ECHO_REQUEST,
    ICMP4_ECHO_REPLY,
    ICMP4_DEST_UNREACH,
    ICMP4_TIME_EXCEEDED,
    ICMP6_ECHO_REQUEST,
    ICMP6_ECHO_REPLY,
    ICMP6_PACKET_TOO_BIG,
    IcmpPacket,
    icmp4Checksum,
    verifyIcmp4Checksum,
    icmp6Checksum,
    verifyIcmp6Checksum,
    parseIcmp,
    encodeIcmp4,
    encodeIcmp4Echo,
    encodeIcmp6,
    encodeIcmp6Echo,
} from "cap:icmp";
```

## 核心 API

### RFC 1071 校验和
- `icmp4Checksum(packet: []const u8): u16` / `verifyIcmp4Checksum(packet: []const u8): bool`
- `icmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): u16` / `verifyIcmp6Checksum(src_ip: [16]u8, dst_ip: [16]u8, packet: []const u8): bool`（包含 40B IPv6 伪首部 + NextHeader=58）

### 解析视图 `IcmpPacket`
- `parseIcmp(data: []const u8): IcmpPacket?`
- 字段：`icmp_type: u8`, `code: u8`, `checksum: u16`, `rest_of_header: u32`, `payload: []const u8`
- 辅助：`echoId()`, `echoSeq()`, `nextHopMtu()`, `packetTooBigMtu()`, `isEchoRequest4()`, `isEchoReply4()`, `isEchoRequest6()`, `isEchoReply6()`

### 零分配编码器
- `encodeIcmp4(icmp_type, code, rest_of_header, payload, out: *[]u8): usize?`
- `encodeIcmp4Echo(is_reply, id, seq, payload, out: *[]u8): usize?`
- `encodeIcmp6(src_ip, dst_ip, icmp_type, code, rest_of_header, payload, out: *[]u8): usize?`
- `encodeIcmp6Echo(src_ip, dst_ip, is_reply, id, seq, payload, out: *[]u8): usize?`

## 使用示例

```cp
import { encodeIcmp4Echo, parseIcmp, verifyIcmp4Checksum } from "cap:icmp";

fn pingExample() {
    let buf: [64]u8 = [0; 64];
    let out: []u8 = buf[0..64];
    const n = encodeIcmp4Echo(false, 0x1234, 1, "hello", *out) ?? return;
    const ok = verifyIcmp4Checksum(out[0..n]);
    const pkt = parseIcmp(out[0..n]) ?? return;
    * = ok;
    * = pkt.echoId();
}
```
