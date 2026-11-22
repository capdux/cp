# `cap:dnp3` — IEEE 1815 DNP3 电力与 SCADA 零分配线协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

> **模块路径**: `lib/dnp3/`  
> **导入协议**: `import { DNP3_TCP_PORT, DNP3_TLS_PORT, dnp3Crc16, parseDnp3LinkHeader, extractDnp3Payload, encodeDnp3Frame, parseDnp3Transport, parseDnp3App } from "cap:iot";`  
> **动态挂载**: `import cap from "cap";` -> `cap.dnp3.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:dnp3` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 IEEE 1815 DNP3（Distributed Network Protocol 3）电力与 SCADA 线协议编解码库：

1. **IEEE 1815 标准 CRC-16 (`dnp3Crc16`)**：
   - 采用反射多项式 `0xA6BC`、初值 `0x0000`、最终取反 `^ 0xFFFF`，线缆小端输出（标准 `"123456789"` 向量为 `0xEA82`）。
2. **FT3 Data Link 帧头与 16B 分块 CRC 编解码 (`parseDnp3LinkHeader` / `extractDnp3Payload` / `encodeDnp3Frame`)**：
   - 严格校验 `0x05 0x64` 帧起始码、`length >= 5` 及 8B 头 CRC-16。
   - 自动按 16 字节块（及尾块）校验/附加 2 字节小端 CRC-16，零分配剥离重组连续载荷。
3. **Transport & Application 层零拷贝解析 (`parseDnp3Transport` / `parseDnp3App`)**：
   - 解析 1 字节传输头（`FIN` / `FIR` / 6-bit `SEQ`）。
   - 解析应用层控制字（`FIR`/`FIN`/`CON`/`UNS`/4-bit `SEQ`）、功能码，并自动区分 2 字节请求头与 4 字节含 2B IIN（Internal Indications）响应头。

---

## 二、 核心 API 速查表 (`lib/dnp3/dnp3.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `DNP3_TCP_PORT (20000)`, `DNP3_TLS_PORT (19999)`, `DNP3_START_BYTES (0x0564)`, `DNP3_HEADER_SIZE (10)`, `DNP3_BLOCK_SIZE (16)`, `DNP3_MAX_PAYLOAD_LEN (250)`, `DNP3_CRC_POLY (0xA6BC)` |
| 链路与功能码常量 | `DL_DIR`, `DL_PRM`, `DL_FCB_DFC`, `DL_FCV`；`FC_CONFIRM (0)`, `FC_READ (1)`, `FC_WRITE (2)`, `FC_SELECT (3)`, `FC_OPERATE (4)`, `FC_DIRECT_OPERATE (5)`, `FC_COLD_RESTART (13)`, `FC_RESPONSE (129)`, `FC_UNSOLICITED_RESPONSE (130)` |
| `dnp3Crc16(data)` | 计算 IEEE 1815 DNP3 CRC-16 |
| `Dnp3LinkHeader` | `{ length, ctrl, dst, src, crc }`；含 `isFromMaster()`, `isPrimary()`, `funcCode()`, `payloadLen()` |
| `Dnp3TransportHeader` | `{ fin, fir, seq }`；配合 `parseDnp3Transport` / `encodeDnp3Transport` |
| `Dnp3AppHeader` | `{ fir, fin, con, uns, seq, func_code, iin: u16?, objects: []const u8 }`；含 `isResponse()` |
| 帧解析与编码 | `isDnp3`, `parseDnp3LinkHeader`, `dnp3WireFrameSize`, `extractDnp3Payload(frame, out)`, `encodeDnp3Frame(ctrl, dst, src, payload, out)`, `parseDnp3App` |

---

## 三、 使用示例

```typescript
import { DL_DIR, DL_PRM, encodeDnp3Frame, extractDnp3Payload, parseDnp3Transport, parseDnp3App } from "cap:iot";

fn demo() {
    // Transport (FIN=1, FIR=1, SEQ=1 -> 0xC1) + App READ (AC=0xC1, FC=1, Obj=60/1/6)
    const user_payload: [6]u8 = [0xC1, 0xC1, 0x01, 60, 1, 6];
    let wire: [64]u8 = [0; 64];
    if (n := encodeDnp3Frame(DL_DIR | DL_PRM | 4, 10, 1, user_payload[0..], wire[0..])) {
        let out: [64]u8 = [0; 64];
        if (plen := extractDnp3Payload(wire[0..n], out[0..])) {
            const tr = parseDnp3Transport(out[0]);
            if (app := parseDnp3App(out[1..plen])) {
                * = tr;
                * = app;
            }
        }
    }
}
```
