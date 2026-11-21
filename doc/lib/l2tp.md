# `cap:l2tp` — 零分配 RFC 2661 L2TPv2 & RFC 3931 L2TPv3 线协议解析库

> **模块路径**：`lib/l2tp/`（入口 `lib/l2tp/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 RFC 2661 L2TPv2 控制/数据报文（`T/L/S/O/P` 标志与可选字段）、RFC 3931 L2TPv3 Control-over-UDP 报头，以及 6 字节头 / 10-bit 长度 AVP 零拷贝迭代。

---

## 一、导入方式

```cp
import {
    L2TP_UDP_PORT,
    L2TP_IP_PROTO,
    L2TP_V2,
    L2TP_V3,
    L2TP_AVP_HEADER_SIZE,
    FLAG_TYPE_CTRL,
    FLAG_LENGTH,
    FLAG_SEQUENCE,
    FLAG_OFFSET,
    FLAG_PRIORITY,
    AVP_MESSAGE_TYPE,
    AVP_RESULT_CODE,
    AVP_PROTOCOL_VERSION,
    AVP_FRAMING_CAPS,
    AVP_BEARER_CAPS,
    AVP_HOST_NAME,
    AVP_VENDOR_NAME,
    AVP_ASSIGNED_TUNNEL_ID,
    AVP_RECEIVE_WINDOW_SIZE,
    AVP_ASSIGNED_SESSION_ID,
    AVP_CALL_SERIAL_NUMBER,
    L2tpAvp,
    L2tpAvpIter,
    L2tpv2Packet,
    L2tpv3ControlHeader,
    parseL2tpv2,
    parseL2tpv3Control,
    isL2tp,
} from "cap:l2tp";
```

---

## 二、核心数据结构

| 符号 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `L2tpAvp` | `mandatory`, `hidden`, `length`, `vendor_id`, `attr_type`, `value` | 单个 L2TP AVP 零拷贝视图 |
| `L2tpAvpIter` | `init(avp_bytes)`, `next(*_)`, `hasError()` | 零分配 AVP 迭代器 |
| `L2tpv2Packet` | `is_control`, `has_length`, `has_seq`, `has_offset`, `is_priority`, `length`, `tunnel_id`, `session_id`, `ns`, `nr`, `offset_size`, `payload`; `avps()` | RFC 2661 L2TPv2 报文视图 |
| `L2tpv3ControlHeader` | `length`, `cc_id`, `ns`, `nr`, `avp_bytes`; `avps()` | RFC 3931 L2TPv3 Control-over-UDP 报文视图 |

---

## 三、核心 API

- `parseL2tpv2(data)` — 严格校验 `version == 2`、控制报文必须置位 `L`/`S` 且无 `O`/`P`，按顺序解析可选字段。
- `parseL2tpv3Control(data)` — 严格校验前 4B `session_id == 0`、`T=1, L=1, S=1, ver=3` 与 `12 <= length <= data.len - 4`。
- `isL2tp(data)` — 快速判定是否为合法 L2TPv2 或 L2TPv3 Control-over-UDP 报文。

---

## 四、使用示例

```cp
import { parseL2tpv2, AVP_HOST_NAME } from "cap:l2tp";

fn inspectL2tp(pkt: []const u8) {
    if (v2 := parseL2tpv2(pkt)) {
        if (v2.is_control) {
            let it = v2.avps();
            while (avp := it.next()) {
                if (avp.attr_type == AVP_HOST_NAME) {
                    * = avp.value;
                }
            }
        }
    }
}
```
