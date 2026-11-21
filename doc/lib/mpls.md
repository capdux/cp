# `cap:mpls` — RFC 3032 MPLS 标签栈 + RFC 4385 PW-CW + RFC 5586 G-ACh 零分配解析库

纯 CP、零堆分配的 MPLS 标签栈（Label Stack Entry）、Pseudowire Control Word（PW-CW）与 Generic Associated Channel Header（G-ACh）线协议编解码库。

## 导入

```cp
import {
    MPLS_ETHERTYPE_UNICAST,
    MPLS_ETHERTYPE_MULTICAST,
    MPLS_LSE_SIZE,
    MPLS_MAX_STACK_DEPTH,
    LABEL_IPV4_EXPLICIT_NULL,
    LABEL_ROUTER_ALERT,
    LABEL_IPV6_EXPLICIT_NULL,
    LABEL_IMPLICIT_NULL,
    LABEL_ENTROPY_INDICATOR,
    LABEL_GAL,
    LABEL_OAM_ALERT,
    LABEL_EXTENSION,
    MplsPayloadKind,
    MplsLabel,
    decodeLse,
    encodeLse,
    MplsLabelIter,
    peekPayloadVersion,
    MplsStack,
    parseMpls,
    encodeMplsStack,
    MplsGach,
    parseMplsGach,
    MplsPwCw,
    parseMplsPwCw,
} from "cap:mpls";
```

## 核心 API

- **LSE 编解码**：`decodeLse(raw: u32): MplsLabel`、`encodeLse(l: MplsLabel): u32`、`MplsLabel.isSpecial()` / `encode()`。
- **标签栈编解码与迭代**：
  - `parseMpls(data: []const u8): MplsStack?` — 迭代 4B LSE 直至 `bottom_of_stack == true`（防御截断、缺失 BoS 与超过 `MPLS_MAX_STACK_DEPTH=16` 层栈）。
  - `MplsStack.labels(): MplsLabelIter`、`topLabel()`、`bottomLabel()`、`payloadKind()`。
  - `encodeMplsStack(labels: []const MplsLabel, payload: []const u8, out: []u8): usize?` — 零分配编码标签栈与载荷。
- **G-ACh / PW-CW / 负载嗅探**：
  - `peekPayloadVersion(payload: []const u8): MplsPayloadKind` — 根据首 4-bit nibble 识别 `ipv4(4)`、`ipv6(6)`、`gach(1)`、`pw_cw(0)`。
  - `parseMplsGach(payload: []const u8): MplsGach?`（RFC 5586，校验首 nibble == `0x1`）。
  - `parseMplsPwCw(payload: []const u8): MplsPwCw?`（RFC 4385，校验首 nibble == `0x0`）。

## 示例

```cp
import std from "std";
import { MplsLabel, encodeMplsStack, parseMpls } from "cap:mpls";

fn example() {
    const stack: [2]MplsLabel = [
        MplsLabel{ .label = 1000, .tc = 5, .bottom_of_stack = false, .ttl = 64 },
        MplsLabel{ .label = 2000, .tc = 0, .bottom_of_stack = true, .ttl = 255 },
    ];
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const ipv4_stub: [4]u8 = [0x45, 0x00, 0x00, 0x14];
    if (n := encodeMplsStack(stack[0..], ipv4_stub[0..], buf[0..])) {
        if (parsed := parseMpls(buf[0..n])) {
            * = parsed.label_count;
            * = parsed.payloadKind();
        }
    }
}
```
