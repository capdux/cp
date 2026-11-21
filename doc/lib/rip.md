# `cap:rip` — RFC 2453 RIPv1/v2 & RFC 2080 RIPng 零分配线协议编解码库

纯 CP 实现、零堆分配的 RIPv1/v2 (IPv4) 与 RIPng (IPv6) 路由信息协议解析、迭代与编码库。

## 导入

```cp
import {
    RIP_UDP_PORT,
    RIPNG_UDP_PORT,
    RIP_HEADER_SIZE,
    RIP_RTE_SIZE,
    RIP_V1,
    RIP_V2,
    RIPNG_V1,
    RIP_CMD_REQUEST,
    RIP_CMD_RESPONSE,
    RIP_AFI_IP,
    RIP_AFI_AUTH,
    RIP_AUTH_SIMPLE,
    RIP_AUTH_CRYPTO,
    RIP_METRIC_INFINITY,
    RIPNG_NEXT_HOP_METRIC,
    RipHeader,
    RipRte,
    RipAuthSimple,
    RipRteIter,
    RipPacket,
    RipngRte,
    RipngRteIter,
    RipngPacket,
    isRip,
    parseRip,
    parseRipAuthSimple,
    parseRipng,
    encodeRipHeader,
    encodeRipRte,
} from "cap:rip";
```

## 核心 API

- **RIPv1/v2**：`isRip(data)`、`parseRip(data): RipPacket?`、`RipPacket.entries(): RipRteIter`、`RipPacket.rawRteAt(idx)`、`parseRipAuthSimple(rte_raw): RipAuthSimple?`
- **RIPng**：`parseRipng(data): RipngPacket?`、`RipngPacket.entries(): RipngRteIter`、`RipngRte.isNextHop()`
- **编码**：`encodeRipHeader(command, version, out: *[]u8): usize?`、`encodeRipRte(rte, out: *[]u8): usize?`

## 使用示例

```cp
import { parseRip, encodeRipHeader, encodeRipRte, RipRte, RIP_CMD_RESPONSE, RIP_V2, RIP_AFI_IP } from "cap:rip";

fn buildAndParse() {
    let buf: [24]u8 = [0; 24];
    let h: []u8 = buf[0..4];
    * = encodeRipHeader(RIP_CMD_RESPONSE, RIP_V2, *h);
    let r: []u8 = buf[4..24];
    * = encodeRipRte(RipRte {
        afi: RIP_AFI_IP,
        route_tag: 0,
        ip: [10, 0, 0, 0],
        mask: [255, 0, 0, 0],
        next_hop: [0, 0, 0, 0],
        metric: 2,
    }, *r);
    const pkt = parseRip(buf[0..24]) ?? return;
    * = pkt.rte_count;
}
```
