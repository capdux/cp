# `cap:vxlan` — RFC 7348 VXLAN & VXLAN-GPE 零分配编解码库

纯 CP、零堆分配的 RFC 7348 VXLAN、VXLAN-GPE 及 VXLAN-GBP 8 字节大端隧道封装头与内层载荷编解码器。

## 导入

```cp
import {
    VXLAN_HEADER_SIZE,
    VXLAN_DEFAULT_PORT,
    VXLAN_GPE_PORT,
    VXLAN_MAX_VNI,
    VXLAN_FLAG_I,
    VXLAN_GPE_FLAG_P,
    VXLAN_GPE_FLAG_O,
    VXLAN_GBP_FLAG_G,
    VxlanNextProto,
    VxlanHeader,
    VxlanPacket,
    vxlanVerifyHeader,
    parseVxlan,
    parseVxlanStrict,
    parseVxlanGpe,
    encodeVxlan,
    encodeVxlanGpe,
    encodeVxlanGbp,
} from "cap:vxlan";
```

## 常量与枚举

- `VXLAN_HEADER_SIZE = 8`、`VXLAN_DEFAULT_PORT = 4789`、`VXLAN_GPE_PORT = 4790`、`VXLAN_MAX_VNI = 0x00FFFFFF`
- 标志位：`VXLAN_FLAG_I = 0x08`、`VXLAN_GPE_FLAG_P = 0x04`、`VXLAN_GPE_FLAG_O = 0x01`、`VXLAN_GBP_FLAG_G = 0x80`
- `VxlanNextProto: u8`：`none(0)`, `ipv4(1)`, `ipv6(2)`, `ethernet(3)`, `nsh(4)`, `mpls(5)`, `unknown(255)`

## 核心 API

### 解析与校验
- `parseVxlan(data: []const u8): VxlanPacket?` — 宽松模式解析（校验 `len >= 8` 与 `I-bit`，自动提取 GPE `next_proto` 与 GBP `group_policy_id`）。
- `parseVxlanStrict(data: []const u8): VxlanPacket?` — RFC 7348 严格模式（要求 `flags == 0x08` 且全部保留字节为 0）。
- `parseVxlanGpe(data: []const u8): VxlanPacket?` — VXLAN-GPE 模式（校验 `I-bit` + `P-bit` 与保留位，解析偏移 3 字节为 `next_proto`）。
- `vxlanVerifyHeader(data: []const u8): bool` — 批量帧头快速校验。
- `VxlanHeader` / `VxlanPacket` 只读辅助：`hasValidVni()`, `isGpe()`, `isOam()`, `hasGbp()`。

### 零分配编码
- `encodeVxlan(vni: u32, inner_payload: []const u8, out: []u8): usize?`
- `encodeVxlanGpe(vni: u32, next_proto: VxlanNextProto, is_oam: bool, inner_payload: []const u8, out: []u8): usize?`
- `encodeVxlanGbp(vni: u32, group_policy_id: u16, inner_payload: []const u8, out: []u8): usize?`

## 示例

```cp
import std from "std";
import { encodeVxlan, parseVxlanStrict } from "cap:vxlan";

fn roundtrip() {
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const inner_eth = "ETH_FRAME_BYTES";
    if (n := encodeVxlan(100, inner_eth, out[0..])) {
        if (pkt := parseVxlanStrict(out[0..n])) {
            * = pkt.header.vni;
            * = pkt.payload;
        }
    }
}
```
