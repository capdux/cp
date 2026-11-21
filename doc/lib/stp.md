# `cap:stp` — IEEE 802.1D / 802.1w STP & RSTP BPDU 零分配编解码库

纯 CP、零堆分配的生成树协议（STP Config 35B、RSTP 36B、TCN 4B）与 IEEE 802.1t 扩展系统 ID `StpBridgeId` 编解码库。

## 导入

```cp
import {
    STP_PROTOCOL_ID,
    STP_VERSION_STP,
    STP_VERSION_RSTP,
    STP_VERSION_MSTP,
    BPDU_TYPE_CONFIG,
    BPDU_TYPE_RST,
    BPDU_TYPE_TCN,
    STP_TCN_SIZE,
    STP_CONFIG_SIZE,
    STP_RST_SIZE,
    FLAG_TC,
    FLAG_PROPOSAL,
    FLAG_LEARNING,
    FLAG_FORWARDING,
    FLAG_AGREEMENT,
    FLAG_TCA,
    StpPortRole,
    StpBridgeId,
    parseBridgeId,
    encodeBridgeId,
    StpBpdu,
    isBpdu,
    parseBpdu,
    encodeConfigBpdu,
} from "cap:stp";
```

## 核心 API

### Bridge Identifier（IEEE 802.1t）
- `StpBridgeId`：`priority: u16`（高 4 位，4096 步长）、`system_id_ext: u16`（低 12 位 VLAN ID）、`mac: [6]u8`。
- `parseBridgeId(bytes_8: []const u8): StpBridgeId?`
- `encodeBridgeId(bid: StpBridgeId, out_8: []u8): bool`

### BPDU 解析与编码
- `isBpdu(data: []const u8): bool` — 快速校验 `protocol_id == 0x0000` 与类型最小长度。
- `parseBpdu(data: []const u8): StpBpdu?` — 解析 TCN (4B) / Config (35B) / RSTP (36B) BPDU。
- `encodeConfigBpdu(bpdu: StpBpdu, out: []u8): usize?` — 按类型自动编码 4B / 35B / 36B。
- `StpBpdu` 只读辅助：
  - 状态与角色：`isTcn()`, `isRstp()`, `portRole()`, `isTopologyChange()`, `isTopologyChangeAck()`, `isProposal()`, `isAgreement()`, `isForwarding()`, `isLearning()`
  - 端口字段：`portPriority(): u8`, `portNumber(): u16`
  - IEEE 1/256s 定时器换算：`messageAgeSec()`, `maxAgeSec()`, `helloTimeSec()`, `forwardDelaySec()`

## 示例

```cp
import std from "std";
import { parseBpdu, encodeConfigBpdu, STP_VERSION_RSTP, BPDU_TYPE_RST, FLAG_FORWARDING } from "cap:stp";

fn example(raw: []const u8) {
    if (bpdu := parseBpdu(raw)) {
        if (bpdu.isRstp() && bpdu.isForwarding()) {
            * = bpdu.helloTimeSec();
        }
    }
}
```
