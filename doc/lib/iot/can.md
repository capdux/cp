# `cap:can` — ISO 11898 CAN 2.0/FD、SocketCAN、SAE J1939 与 DBC 信号零分配库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

> **模块路径**: `lib/can/`  
> **导入协议**: `import { CanId, CanFrame, canFdDlcToLen, canFdLenToDlc, decodeJ1939, encodeJ1939, parseSocketCan, parseSocketCanFd, encodeSocketCan, encodeSocketCanFd, extractSignalIntel, extractSignalMotorola, signExtend } from "cap:iot";`  
> **动态挂载**: `import cap from "cap";` -> `cap.can.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:can` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的车载/工业 CAN 总线协议编解码库：

1. **SocketCAN 经典帧 (16B) & CAN FD 帧 (72B)**：
   - 完整支持 11-bit SFF / 29-bit EFF、`RTR` / `ERR` 标志位、`CANFD_BRS` / `CANFD_ESI` / `CANFD_FDF` 标志。
   - 同时提供小端（Linux 主机序）与大端（网络序 `*Be`）编解码函数，严格校验 `len <= 8` / `len <= 64` 与缓冲区长度。
2. **CAN FD DLC 双向查表 (`canFdDlcToLen` / `canFdLenToDlc`)**：
   - 严格映射 ISO 11898-1:2015 非线性 DLC 表：`0..8 -> 0..8, 9->12, 10->16, 11->20, 12->24, 13->32, 14->48, 15->64`。
3. **SAE J1939 29-bit PGN 编解码 (`decodeJ1939` / `encodeJ1939` / `encodeJ1939Pdu1`)**：
   - 自动区分 PDU1（`pf < 240`，点对点，PGN 低 8 位清零）与 PDU2（`pf >= 240`，广播，PGN 包含 `ps`）。
4. **DBC 位域信号提取 (`extractSignalIntel` / `extractSignalMotorola` / `signExtend`)**：
   - 支持 1..64 bit 无符号/有符号跨字节信号提取，兼容 Intel 小端 LSB 起始位与 Vector DBC Motorola 大端 MSB 起始位，并提供无溢出风险的二补数符号扩展。

---

## 二、 核心 API 速查表 (`lib/can/can.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `CAN_FRAME_SIZE (16)`, `CANFD_FRAME_SIZE (72)`, `CAN_MAX_DLEN (8)`, `CANFD_MAX_DLEN (64)`, `CAN_SFF_MASK`, `CAN_EFF_MASK`, `CAN_EFF_FLAG`, `CAN_RTR_FLAG`, `CAN_ERR_FLAG`, `CANFD_BRS`, `CANFD_ESI`, `CANFD_FDF` |
| DLC 映射 | `canFdDlcToLen(dlc: u8): u8?`、`canFdLenToDlc(len: u8): u8?` |
| `CanId` | `{ raw: u32 }`；含 `standard(id_11)`, `extended(id_29)`, `id()`, `isExtended()`, `isRtr()`, `isError()` |
| `CanFrame` | `{ can_id, len, flags, is_fd, data: [64]u8 }`；含 `payload(): []const u8`, `hasBrs()`, `hasEsi()` |
| `J1939Id` | `{ priority, edp, dp, pf, ps, sa, pgn }`；含 `isPdu1()`, `isPdu2()`；配合 `decodeJ1939`, `encodeJ1939`, `encodeJ1939Pdu1` |
| SocketCAN 编解码 | `parseSocketCan` / `parseSocketCanBe`、`parseSocketCanFd` / `parseSocketCanFdBe`、`encodeSocketCan` / `encodeSocketCanBe`、`encodeSocketCanFd` / `encodeSocketCanFdBe` |
| DBC 信号提取 | `extractSignalIntel(payload, start_bit, bit_len): u64?`、`extractSignalMotorola(payload, start_bit, bit_len): u64?`、`signExtend(val, bit_len): i64` |

---

## 三、 使用示例

```typescript
import { decodeJ1939, extractSignalIntel, signExtend } from "cap:iot";

fn demo() {
    // SAE J1939 EEC1 (Electronic Engine Controller 1): 0x0CF00400
    const j = decodeJ1939(0x0CF00400);
    // j.priority == 3, j.pgn == 61444 (0xF004), j.sa == 0
    * = j;

    const payload: [8]u8 = [0xF0, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    if (raw := extractSignalIntel(payload[0..], 0, 12)) {
        const signed_val = signExtend(raw, 12); // -16
        * = signed_val;
    }
}
```
