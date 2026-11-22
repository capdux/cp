# `cap:lacp` — IEEE 802.1AX / 802.3ad LACPDU 零分配线协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP 实现、零堆分配的 IEEE 802.1AX / 802.3ad Link Aggregation Control Protocol (LACP) 报文解析与编码库。

## 导入

```cp
import {
    SLOW_PROTO_ETHERTYPE,
    LACP_SUBTYPE,
    LACP_VERSION,
    LACP_MIN_LEN,
    LACP_FULL_LEN,
    TLV_TERMINATOR,
    TLV_ACTOR,
    TLV_PARTNER,
    TLV_COLLECTOR,
    STATE_ACTIVITY,
    STATE_TIMEOUT,
    STATE_AGGREGATION,
    STATE_SYNC,
    STATE_COLLECTING,
    STATE_DISTRIBUTING,
    STATE_DEFAULTED,
    STATE_EXPIRED,
    LacpPeerInfo,
    LacpPdu,
    isLacp,
    parseLacp,
    encodeLacp,
} from "cap:iot";
```

## 核心 API

- `isLacp(data: []const u8): bool`：快速校验长度 `>= 110`、Subtype `0x01`、Version `0x01` 及 Actor/Partner/Collector/Terminator TLV 签名。
- `parseLacp(data: []const u8): LacpPdu?`：零拷贝解析 Actor/Partner 信息与 Collector Max Delay。
- `encodeLacp(pdu: LacpPdu, out: *[]u8): usize?`：零分配输出标准 110 字节 LACPDU（含 50 字节尾部零填充）。
- `LacpPeerInfo` 状态位辅助：`isActive()`, `isShortTimeout()`, `isAggregatable()`, `isInSync()`, `isCollecting()`, `isDistributing()`, `isDefaulted()`, `isExpired()`。

## 使用示例

```cp
import { LacpPdu, LacpPeerInfo, encodeLacp, parseLacp, STATE_ACTIVITY, STATE_SYNC } from "cap:iot";

fn roundtrip() {
    const pdu = LacpPdu {
        subtype: 1,
        version: 1,
        actor: LacpPeerInfo {
            system_priority: 32768,
            system_id: [0x00, 0x11, 0x22, 0x33, 0x44, 0x55],
            key: 10,
            port_priority: 128,
            port_number: 1,
            state: STATE_ACTIVITY | STATE_SYNC,
        },
        partner: LacpPeerInfo {
            system_priority: 32768,
            system_id: [0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB],
            key: 20,
            port_priority: 128,
            port_number: 2,
            state: STATE_ACTIVITY,
        },
        collector_max_delay: 100,
    };
    let buf: [110]u8 = [0; 110];
    let out: []u8 = buf[0..110];
    const n = encodeLacp(pdu, *out) ?? return;
    const decoded = parseLacp(out[0..n]) ?? return;
    * = decoded.actor.isInSync();
}
```
