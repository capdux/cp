# `cap:snmp` — RFC 1157 / RFC 3416 SNMP v1/v2c 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP 实现、零堆分配的 SNMP v1 / v2c 简单网络管理协议 ASN.1 BER/DER 报文、PDU、VarBind 与 OID 解析库。

## 导入

```cp
import {
    SNMP_PORT,
    SNMP_TRAP_PORT,
    SNMP_V1,
    SNMP_V2C,
    SnmpPduType,
    SnmpValueTag,
    SnmpVarBind,
    SnmpVarBindIter,
    SnmpMessage,
    parseSnmpMessage,
    formatSnmpOid,
} from "cap:net";
```

## 常量与枚举

| 符号 | 类型 / 值 | 说明 |
| :--- | :--- | :--- |
| `SNMP_PORT` | `u16 = 161` | SNMP 标准请求/响应 UDP 端口 |
| `SNMP_TRAP_PORT` | `u16 = 162` | SNMP Trap / Inform 通知 UDP 端口 |
| `SNMP_V1` | `u8 = 0` | SNMPv1 协议版本号 (RFC 1157) |
| `SNMP_V2C` | `u8 = 1` | SNMPv2c 协议版本号 (RFC 3416) |
| `SnmpPduType` | `enum: u8` | `get_request=0xA0`, `get_next_request=0xA1`, `get_response=0xA2`, `set_request=0xA3`, `v1_trap=0xA4`, `get_bulk_request=0xA5`, `inform_request=0xA6`, `v2_trap=0xA7`, `report=0xA8`, `unknown=0` |
| `SnmpValueTag` | `enum: u8` | `integer=0x02`, `octet_string=0x04`, `null_val=0x05`, `oid=0x06`, `ip_address=0x40`, `counter32=0x41`, `gauge32=0x42`, `timeticks=0x43`, `opaque_val=0x44`, `counter64=0x46`, `no_such_object=0x80`, `no_such_instance=0x81`, `end_of_mib_view=0x82` |

## 核心 API

### `parseSnmpMessage(data: []const u8): SnmpMessage?`
解析外层 `SEQUENCE { version INTEGER, community OCTET STRING, pdu [CONTEXT] }`。
- 支持确定长度短/长形式 BER 长度编码；拒绝不定长度 `0x80`、非法版本、未知 PDU 标签、截断与尾随垃圾字节。
- 标准 PDU (`0xA0..0xA3`, `0xA5..0xA8`) 填充 `request_id`、`error_status`（`GetBulkRequest` 中为 `non_repeaters`）、`error_index`（`GetBulkRequest` 中为 `max_repetitions`）与 `varbinds_der`。
- v1 Trap PDU (`0xA4`) 填充 `enterprise_oid`、`agent_addr`、`generic_trap`、`specific_trap`、`timestamp` 与 `varbinds_der`。

### `SnmpMessage`
- `varbinds(): SnmpVarBindIter`：返回零分配 VarBind 迭代器。
- `nonRepeaters(): i64` / `maxRepetitions(): i64`：`GetBulkRequest` 参数便捷访问器。

### `SnmpVarBind` 与 `SnmpVarBindIter`
- `SnmpVarBindIter.init(varbinds_der: []const u8): SnmpVarBindIter`
- `next(*_): SnmpVarBind?`：逐项解码 `SEQUENCE { oid OBJECT IDENTIFIER, value ANY }`；格式异常时置 `had_error = true` 并终止。
- `SnmpVarBind` 标量提取方法：
  - `asI64(): i64?`（`integer`）
  - `asU64(): u64?`（`counter32` / `gauge32` / `timeticks` / `counter64`，支持 9 字节前导 `0x00` Counter64）
  - `asOctetString(): ([]const u8)?`（`octet_string`）
  - `asIp4(): ([4]u8)?`（4 字节 `ip_address`）
  - `isNull(): bool` / `isException(): bool`

### `formatSnmpOid(oid_bytes: []const u8, out: *[]u8): usize?`
将 BER 编码 OID 字节流零分配格式化为点分十进制 ASCII 字符串（如 `"1.3.6.1.2.1.1.1.0"`），返回写入长度；畸形、截断、非最小 VLQ 或缓冲区不足时返回 `null`。

## 使用示例

```cp
import { parseSnmpMessage, formatSnmpOid, SnmpPduType } from "cap:net";

fn inspectPacket(pkt: []const u8) {
    const msg = parseSnmpMessage(pkt) ?? return;
    if (msg.pdu_type == .get_response) {
        let it = msg.varbinds();
        let oid_buf: [128]u8 = [0; 128];
        let oid_slice: []u8 = oid_buf[0..128];
        while (vb := it.next()) {
            if (n := formatSnmpOid(vb.oid_bytes, *oid_slice)) {
                const oid_str = oid_slice[0..n];
                * = oid_str;
            }
            if (ticks := vb.asU64()) {
                * = ticks;
            }
        }
    }
}
```
