# `cap:iscsi` — RFC 7143 iSCSI 48B BHS、Login/Text KV 与 CRC32C 零分配解析库

> **模块路径**: `lib/iscsi/`  
> **导入协议**: `import { ISCSI_TCP_PORT, ISCSI_BHS_SIZE, OP_LOGIN_REQ, OP_SCSI_CMD, iscsiCrc32c, parseIscsiPdu, parseIscsiScsiCmd, parseIscsiLoginReq, IscsiKvIter, findIscsiKv } from "cap:iscsi";`  
> **动态挂载**: `import cap from "cap";` -> `cap.iscsi.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:iscsi` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 7143 iSCSI 存储协议线缆解析与 CRC32C 校验库：

1. **Castagnoli CRC32C (`iscsiCrc32c`)**：
   - 采用反射多项式 `0x82F63B78`（`"123456789"` -> `0xE3069283`），在开启 `HeaderDigest`（覆盖 BHS + AHS）或 `DataDigest`（覆盖 4B 对齐 Data Segment）时按小端 4B 实时校验。
2. **48 字节 BHS 与完整 PDU 解析 (`parseIscsiPdu`)**：
   - 零拷贝解析 `I`/`Opcode`/`F`/AHS/24-bit `DataSegmentLength`/`LUN`/`ITT`，自动处理 AHS 长度（4B 字）与 4B 对齐 Data Segment 边界。
3. **SCSI Command & Login Request 专用解码器 (`parseIscsiScsiCmd` / `parseIscsiLoginReq`)**：
   - 完整提取 SCSI R/W/Attr/ExpectedDataLen/CmdSN/ExpStatSN/16B CDB 与 Login T/C/CSG/NSG/ISID/TSIH/CID。
4. **Login/Text NUL 分隔键值对迭代 (`IscsiKvIter` / `findIscsiKv`)**：
   - 零拷贝迭代 `Key=Value\0` 谈判参数。

---

## 二、 核心 API 速查表 (`lib/iscsi/iscsi.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `ISCSI_TCP_PORT (3260)`, `ISCSI_BHS_SIZE (48)`, `ISCSI_DIGEST_SIZE (4)`, `ISCSI_CRC32C_POLY (0x82F63B78)`；`OP_NOP_OUT (0x00)` .. `OP_REJECT (0x3F)` |
| `iscsiCrc32c(data)` | Castagnoli CRC32C 校验和 |
| `IscsiBhs` / `IscsiPdu` | 48B BHS（含 `ahsBytesLen()`, `paddedDataLen()`）与零拷贝 PDU 视图 |
| `IscsiScsiCmd` / `IscsiLoginReq` | `parseIscsiScsiCmd(pdu)`、`parseIscsiLoginReq(pdu)` |
| `IscsiKv` / `IscsiKvIter` | `IscsiKvIter.init(data_seg)`、`findIscsiKv(data_seg, key)` |
| `parseIscsiPdu(data, has_hdr_digest, has_data_digest)` | 解析并可选校验双 CRC32C 摘要 |

---

## 三、 使用示例

```typescript
import { parseIscsiPdu, parseIscsiLoginReq, findIscsiKv } from "cap:iscsi";

fn demo(pkt: []const u8) {
    if (pdu := parseIscsiPdu(pkt, true, true)) {
        if (login := parseIscsiLoginReq(pdu)) {
            if (init_name := findIscsiKv(pdu.data_segment, "InitiatorName")) {
                * = login;
                * = init_name;
            }
        }
    }
}
```
