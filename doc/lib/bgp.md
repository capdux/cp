# `cap:bgp` — 零分配 RFC 4271 BGP-4 边界网关路由协议解析标准库

`cap:bgp` 是纯 CP、零堆分配的 RFC 4271 BGP-4 协议解析库，支持 19B 报头校验、TCP 拼接流多报文迭代、`OPEN`（含可选能力参数 TLV）、`UPDATE`（压缩 IPv4 前缀与标准/扩展长度 Path Attributes）、`NOTIFICATION` 与 `KEEPALIVE`。

---

## 快速导入

```typescript
import {
    BGP_HEADER_SIZE,
    BGP_MARKER_LEN,
    BGP_MAX_MESSAGE_LEN,
    BGP_PORT,
    ATTR_ORIGIN,
    ATTR_AS_PATH,
    ATTR_NEXT_HOP,
    ATTR_LOCAL_PREF,
    BgpMsgType,
    BgpMessage,
    BgpStreamIter,
    BgpOptParamIter,
    BgpPrefixIter,
    BgpPathAttrIter,
    parseBgpMessage,
    parseBgpOpen,
    parseBgpUpdate,
    parseBgpNotification,
} from "cap:bgp";
```

---

## 核心 API

- **报头与流迭代**：`parseBgpMessage(data): BgpMessage?`、`BgpStreamIter.init(data)` / `next(*_)`（校验 16B 全 `0xFF` Marker、`19 <= length <= 4096`、`KEEPALIVE` 严格 19B）
- **`OPEN` 解析**：`parseBgpOpen(body): BgpOpen?`（`version`, `my_as`, `hold_time`, `bgp_id`, `optParams(): BgpOptParamIter`）
- **`UPDATE` 解析**：`parseBgpUpdate(body): BgpUpdate?`（零拷贝 `withdrawn_routes`, `path_attrs`, `nlri`），配合 `BgpPrefixIter`（`/0..32` 压缩前缀解码）与 `BgpPathAttrIter`（标准 1B 与 `flags & 0x10` 2B 扩展长度属性）
- **`NOTIFICATION` 解析**：`parseBgpNotification(body): BgpNotification?`
