# `cap:igmp` — 零分配 RFC 1112 / 2236 / 3376 IGMP v1/v2/v3 组播协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**：`lib/igmp/`（入口 `lib/igmp/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 IGMP v1/v2 8 字节报文、IGMPv3 Membership Query（含源地址列表、`S`/`QRV`/`QQIC`）、IGMPv3 Membership Report 多 Group Record（含 Aux 跳过与 `sourceAt` 零拷贝访问）、RFC 3376 8-bit 浮点时间码解码与 RFC 1071 16-bit 校验和编解码。

---

## 一、导入方式

```cp
import {
    IGMP_IP_PROTO,
    IGMP_MIN_SIZE,
    IGMP_V3_QUERY_MIN_SIZE,
    IgmpType,
    IgmpV3RecordType,
    IgmpV3GroupRecord,
    IgmpPacket,
    IgmpV3RecordIter,
    igmpChecksum,
    verifyIgmpChecksum,
    igmpDecodeFloatCode,
    parseIgmp,
    encodeIgmpV2,
} from "cap:net";
```

---

## 二、核心常量与数据结构

| 符号 | 说明 |
| :--- | :--- |
| 协议常量 | `IGMP_IP_PROTO = 2`, `IGMP_MIN_SIZE = 8`, `IGMP_V3_QUERY_MIN_SIZE = 12` |
| `IgmpType: u8` | `membership_query (0x11)`, `v1_report (0x12)`, `v2_report (0x16)`, `v2_leave (0x17)`, `v3_report (0x22)`, `unknown (0)` |
| `IgmpV3RecordType: u8` | `mode_is_include (1)`, `mode_is_exclude (2)`, `change_to_include (3)`, `change_to_exclude (4)`, `allow_new_sources (5)`, `block_old_sources (6)`, `unknown (0)` |
| `IgmpPacket` | `msg_type`, `raw_type`, `max_resp_code`, `checksum`, `group_addr`, `is_v3_query`, `suppress_router_side`, `qrv`, `qqic`, `num_sources`, `sources_slice`, `num_group_records`, `records_slice`; `querySourceAt(idx)`, `v3Records()` |
| `IgmpV3GroupRecord` | `rec_type`, `raw_rec_type`, `aux_data_words`, `num_sources`, `multicast_addr`, `sources_slice`, `aux_slice`; `sourceAt(idx)` |

---

## 三、核心 API

- `igmpChecksum(data: []const u8): u16` / `verifyIgmpChecksum(packet: []const u8): bool` — RFC 1071 16-bit 校验和计算与验证。
- `igmpDecodeFloatCode(code: u8): u32` — RFC 3376 `Max Resp Code` / `QQIC` 浮点时间码解码。
- `parseIgmp(data: []const u8): IgmpPacket?` — 自动识别并解析 IGMP v1/v2/v3 报文。
- `IgmpV3RecordIter` — 零分配迭代 IGMPv3 Report 内的 Group Records。
- `encodeIgmpV2(msg_type, max_resp_code, group_addr, out): usize?` — 编码 8B IGMP v1/v2 报文并自动回填校验和。

---

## 四、使用示例

```cp
import { IgmpType, encodeIgmpV2, parseIgmp, verifyIgmpChecksum } from "cap:net";

fn joinMulticast() {
    let buf: [16]u8 = [0; 16];
    if (n := encodeIgmpV2(IgmpType.v2_report, 0, [239, 1, 2, 3], buf[0..16])) {
        if (verifyIgmpChecksum(buf[0..n])) {
            if (pkt := parseIgmp(buf[0..n])) {
                * = pkt.group_addr;
            }
        }
    }
}
```
