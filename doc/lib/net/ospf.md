# `cap:ospf` — RFC 2328 OSPFv2 零分配路由协议解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP 实现、零堆分配的 RFC 2328 OSPFv2 协议解析库，支持 24B 公共报头、RFC 2328 特殊校验和（跳过 8B 认证区）、Hello 邻居解析、20B LSA 头与 LSU/LSAck 零拷贝迭代。

## 导入

```cp
import {
    OSPF_IP_PROTO,
    OSPF_V2,
    OSPF_HEADER_SIZE,
    OSPF_LSA_HEADER_SIZE,
    AUTH_NONE,
    AUTH_SIMPLE_PW,
    AUTH_CRYPTO,
    OspfPacketType,
    OspfLsaType,
    OspfHeader,
    OspfPacket,
    OspfHello,
    OspfLsaHeader,
    OspfLsuEntry,
    OspfLsuIter,
    OspfLsackIter,
    ospf2Checksum,
    verifyOspf2Checksum,
    isOspf2,
    parseOspf2,
    parseOspfHello,
    parseOspfLsaHeader,
} from "cap:net";
```

## 核心 API

- **校验和**：
  - `ospf2Checksum(packet: []const u8): u16`：跳过偏移 `12..14` 校验和字段与偏移 `16..24` 8B 认证字段计算 RFC 1071 校验和。
  - `verifyOspf2Checksum(packet: []const u8): bool`：跳过偏移 `16..24` 8B 认证字段校验完整报文。
- **报头与分派**：
  - `isOspf2(data: []const u8): bool`
  - `parseOspf2(data: []const u8): OspfPacket?`
- **Hello 解析**：
  - `parseOspfHello(body: []const u8): OspfHello?`（`neighborCount()` / `neighborAt(idx)`）
- **LSA / LSU / LSAck**：
  - `parseOspfLsaHeader(data: []const u8): OspfLsaHeader?`
  - `OspfLsuIter.init(lsu_body): OspfLsuIter?` -> `next(*_): OspfLsuEntry?`
  - `OspfLsackIter.init(lsack_body): OspfLsackIter` -> `next(*_): OspfLsaHeader?`

## 使用示例

```cp
import { parseOspf2, parseOspfHello, verifyOspf2Checksum } from "cap:net";

fn handleOspf(pkt_bytes: []const u8) {
    if (!verifyOspf2Checksum(pkt_bytes)) { return; }
    const pkt = parseOspf2(pkt_bytes) ?? return;
    if (pkt.header.pkt_type == .hello) {
        const hello = parseOspfHello(pkt.body) ?? return;
        * = hello.neighborCount();
    }
}
```
