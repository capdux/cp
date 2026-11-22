# `cap:pppoe` — RFC 2516 PPPoE Discovery & Session 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配的 PPP over Ethernet（PPPoE）6 字节报头、Discovery TLV 标签（PADI/PADO/PADR/PADS/PADT）与 Session 载荷编解码库。

## 导入

```cp
import {
    PPPOE_ETHERTYPE_DISCOVERY,
    PPPOE_ETHERTYPE_SESSION,
    PPPOE_VERSION,
    PPPOE_TYPE,
    PPPOE_HEADER_SIZE,
    CODE_SESSION,
    CODE_PADO,
    CODE_PADI,
    CODE_PADR,
    CODE_PADS,
    CODE_PADT,
    TAG_END_OF_LIST,
    TAG_SERVICE_NAME,
    TAG_AC_NAME,
    TAG_HOST_UNIQ,
    TAG_AC_COOKIE,
    TAG_VENDOR_SPECIFIC,
    TAG_RELAY_SESSION_ID,
    TAG_SERVICE_NAME_ERROR,
    TAG_AC_SYSTEM_ERROR,
    TAG_GENERIC_ERROR,
    PppoeHeader,
    PppoeTag,
    PppoeTagIter,
    PppoePacket,
    isPppoe,
    parsePppoe,
    PppoeWriter,
} from "cap:net";
```

## 核心 API

- **解析与校验**：
  - `isPppoe(data)` — 校验 `len >= 6`、`version == 1 && pkt_type == 1`、`6 + length <= data.len`。
  - `parsePppoe(data): PppoePacket?` — 解析 Discovery 与 Session 报文（Session 强制 `session_id != 0`；Discovery 严格校验全部 Tag 边界并在 `TAG_END_OF_LIST` 处截停）。
  - `PppoePacket`：`isDiscovery()`、`isSession()`、`tags()`、`findTag(tag_type)`、`serviceName()`、`acName()`、`hostUniq()`、`acCookie()`。
- **零分配编码器**：`PppoeWriter`（`init`、`writeTag`、`writeEndOfList`、`writePayload`、`finish` 自动回填 2B BE `length`）。

## 示例

```cp
import std from "std";
import { CODE_PADI, TAG_SERVICE_NAME, TAG_HOST_UNIQ, PppoeWriter, parsePppoe } from "cap:net";

fn example() {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PppoeWriter.init(buf[0..], CODE_PADI, 0);
    * = w.writeTag(TAG_SERVICE_NAME, "fiber-isp");
    * = w.writeTag(TAG_HOST_UNIQ, "uniq1234");
    if (wire := w.finish()) {
        if (pkt := parsePppoe(wire)) {
            * = pkt.serviceName();
        }
    }
}
```
