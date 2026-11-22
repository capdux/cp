# `cap:modbus` — Modbus TCP MBAP & Modbus RTU CRC-16 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

纯 CP、零堆分配（zero-alloc）的 Modbus TCP（MBAP 帧头）与 Modbus RTU（CRC-16/Modbus 反射多项式 `0xA001`）工业自动化线协议编解码库。

## 导入

```cp
import {
    MODBUS_TCP_PORT,
    MODBUS_MBAP_SIZE,
    MODBUS_PROTOCOL_ID,
    MODBUS_MAX_PDU_SIZE,
    MODBUS_EXCEPTION_BIT,
    FC_READ_COILS,
    FC_READ_DISCRETE_INPUTS,
    FC_READ_HOLDING_REGS,
    FC_READ_INPUT_REGS,
    FC_WRITE_SINGLE_COIL,
    FC_WRITE_SINGLE_REG,
    FC_WRITE_MULTI_COILS,
    FC_WRITE_MULTI_REGS,
    EX_ILLEGAL_FUNCTION,
    EX_ILLEGAL_DATA_ADDR,
    EX_ILLEGAL_DATA_VALUE,
    EX_SERVER_FAILURE,
    EX_ACKNOWLEDGE,
    EX_SERVER_BUSY,
    modbusCrc16,
    verifyModbusRtuCrc,
    ModbusPdu,
    ModbusTcpFrame,
    ModbusRtuFrame,
    ModbusReadReq,
    ModbusWriteSingle,
    ModbusRegIter,
    parseModbusTcp,
    encodeModbusTcp,
    parseModbusRtu,
    encodeModbusRtu,
    parseReadReq,
    encodeReadReq,
    parseWriteSingle,
    encodeWriteSingle,
    parseReadRegsResp,
} from "cap:iot";
```

## 核心 API

- **协议常量**：`MODBUS_TCP_PORT (502)`、`MODBUS_MBAP_SIZE (7)`、`MODBUS_PROTOCOL_ID (0)`、`MODBUS_MAX_PDU_SIZE (253)`、`MODBUS_EXCEPTION_BIT (0x80)`，以及全部标准功能码（`FC_*`）与异常码（`EX_*`）。
- **CRC-16/Modbus**：`modbusCrc16(data): u16`、`verifyModbusRtuCrc(frame): bool`（多项式 `0xA001`、初值 `0xFFFF`、小端尾 2 字节校验）。
- **Modbus TCP（MBAP）**：`parseModbusTcp(data): ModbusTcpFrame?`（严格校验 `protocol_id == 0`、`length >= 2`、`6 + length <= data.len`、`pdu_len <= 253`）与 `encodeModbusTcp(tx_id, unit_id, func_code, pdu_data, out: []u8): usize?`。
- **Modbus RTU**：`parseModbusRtu(data): ModbusRtuFrame?`（严格校验 `len >= 4`、`pdu_len <= 253` 与小端 CRC-16）与 `encodeModbusRtu(slave_addr, func_code, pdu_data, out: []u8): usize?`。
- **PDU 载荷辅助**：`ModbusPdu`（`isException()`, `baseFuncCode()`, `exceptionCode()`）、`parseReadReq` / `encodeReadReq`、`parseWriteSingle` / `encodeWriteSingle`、`parseReadRegsResp(pdu_data): ModbusRegIter?`（零拷贝大端 `u16` 寄存器迭代）。

## 示例

```cp
import std from "std";
import {
    FC_READ_HOLDING_REGS,
    encodeReadReq,
    encodeModbusTcp,
    parseModbusTcp,
    parseReadReq,
} from "cap:iot";

fn example() {
    let pdu_buf: [8]u8 = std.mem.zeroes([8]u8);
    let tcp_buf: [64]u8 = std.mem.zeroes([64]u8);
    if (pn := encodeReadReq(0x006B, 3, pdu_buf[0..])) {
        if (tn := encodeModbusTcp(0x1234, 1, FC_READ_HOLDING_REGS, pdu_buf[0..pn], tcp_buf[0..])) {
            if (frame := parseModbusTcp(tcp_buf[0..tn])) {
                * = parseReadReq(frame.pdu.data);
            }
        }
    }
}
```
