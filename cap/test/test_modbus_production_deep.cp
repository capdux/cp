// Source: cp/cap/test/test_modbus_production_deep.cp (Production deep tests for cap:modbus)

import std from "std";
import { test, expect_eq } from "cap:test";
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
} from "cap:modbus";

test("1. Protocol constants and Modbus TCP MBAP encode/parse roundtrip (ReadReq & WriteSingle)", () => {
    expect_eq(MODBUS_TCP_PORT, 502 as u16);
    expect_eq(MODBUS_MBAP_SIZE, 7 as usize);
    expect_eq(MODBUS_PROTOCOL_ID, 0 as u16);
    expect_eq(MODBUS_MAX_PDU_SIZE, 253 as usize);
    expect_eq(MODBUS_EXCEPTION_BIT, 0x80 as u8);
    expect_eq(FC_READ_COILS, 0x01 as u8);
    expect_eq(FC_READ_DISCRETE_INPUTS, 0x02 as u8);
    expect_eq(FC_READ_HOLDING_REGS, 0x03 as u8);
    expect_eq(FC_READ_INPUT_REGS, 0x04 as u8);
    expect_eq(FC_WRITE_SINGLE_COIL, 0x05 as u8);
    expect_eq(FC_WRITE_SINGLE_REG, 0x06 as u8);
    expect_eq(FC_WRITE_MULTI_COILS, 0x0F as u8);
    expect_eq(FC_WRITE_MULTI_REGS, 0x10 as u8);

    // Read Holding Registers (FC 0x03): start=0x006B, qty=3
    let pdu_buf: [8]u8 = std.mem.zeroes([8]u8);
    const pn = encodeReadReq(0x006B, 3, pdu_buf[0..]) ?? unreachable;
    expect_eq(pn, 4 as usize);

    let tcp_buf: [64]u8 = std.mem.zeroes([64]u8);
    const tn = encodeModbusTcp(0x1234, 0x01, FC_READ_HOLDING_REGS, pdu_buf[0..pn], tcp_buf[0..]) ?? unreachable;
    expect_eq(tn, 12 as usize);

    const frame = parseModbusTcp(tcp_buf[0..tn]) ?? unreachable;
    expect_eq(frame.tx_id, 0x1234 as u16);
    expect_eq(frame.protocol_id, 0 as u16);
    expect_eq(frame.length, 6 as u16);
    expect_eq(frame.unit_id, 0x01 as u8);
    expect_eq(frame.pdu.func_code, FC_READ_HOLDING_REGS);
    expect_eq(frame.pdu.isException(), false);
    expect_eq(frame.pdu.baseFuncCode(), FC_READ_HOLDING_REGS);
    expect_eq(frame.pdu.exceptionCode() == null, true);

    const req = parseReadReq(frame.pdu.data) ?? unreachable;
    expect_eq(req.start_addr, 0x006B as u16);
    expect_eq(req.quantity, 3 as u16);

    // Write Single Register (FC 0x06): addr=0x0001, value=0xABCD
    const wn = encodeWriteSingle(0x0001, 0xABCD, pdu_buf[0..]) ?? unreachable;
    const tn2 = encodeModbusTcp(0xBEEF, 0x07, FC_WRITE_SINGLE_REG, pdu_buf[0..wn], tcp_buf[0..]) ?? unreachable;
    const f2 = parseModbusTcp(tcp_buf[0..tn2]) ?? unreachable;
    expect_eq(f2.tx_id, 0xBEEF as u16);
    expect_eq(f2.unit_id, 0x07 as u8);
    const ws = parseWriteSingle(f2.pdu.data) ?? unreachable;
    expect_eq(ws.addr, 0x0001 as u16);
    expect_eq(ws.value, 0xABCD as u16);
});

test("2. CRC-16/Modbus standard test vectors and Modbus RTU encode/parse roundtrip", () => {
    // Standard CRC-16/Modbus check vector: ASCII "123456789" -> 0x4B37
    expect_eq(modbusCrc16("123456789"), 0x4B37 as u16);

    // Classic Modbus RTU Read Holding Registers vector:
    // Slave=0x01, FC=0x03, Start=0x0000, Qty=0x000A -> CRC=0xCDC5 (LE bytes: 0xC5, 0xCD)
    let rtu_vec: [8]u8 = [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD];
    expect_eq(modbusCrc16(rtu_vec[0..6]), 0xCDC5 as u16);
    expect_eq(verifyModbusRtuCrc(rtu_vec[0..]), true);

    const parsed_vec = parseModbusRtu(rtu_vec[0..]) ?? unreachable;
    expect_eq(parsed_vec.slave_addr, 0x01 as u8);
    expect_eq(parsed_vec.pdu.func_code, FC_READ_HOLDING_REGS);
    expect_eq(parsed_vec.crc, 0xCDC5 as u16);
    const req = parseReadReq(parsed_vec.pdu.data) ?? unreachable;
    expect_eq(req.start_addr, 0 as u16);
    expect_eq(req.quantity, 10 as u16);

    // Encode RTU frame and verify exact wire byte match with standard vector
    let out: [32]u8 = std.mem.zeroes([32]u8);
    let pdu: [4]u8 = [0x00, 0x00, 0x00, 0x0A];
    const rn = encodeModbusRtu(0x01, FC_READ_HOLDING_REGS, pdu[0..], out[0..]) ?? unreachable;
    expect_eq(rn, 8 as usize);
    expect_eq(std.mem.eql(u8, out[0..rn], rtu_vec[0..]), true);
});

test("3. Read Holding/Input Registers response ModbusRegIter zero-copy iteration", () => {
    // 3 registers: 0x022B (555), 0x0000 (0), 0x0064 (100)
    let resp_pdu: [7]u8 = [0x06, 0x02, 0x2B, 0x00, 0x00, 0x00, 0x64];
    let tcp_buf: [64]u8 = std.mem.zeroes([64]u8);
    const tn = encodeModbusTcp(0x0042, 0x01, FC_READ_HOLDING_REGS, resp_pdu[0..], tcp_buf[0..]) ?? unreachable;
    const frame = parseModbusTcp(tcp_buf[0..tn]) ?? unreachable;

    let it = parseReadRegsResp(frame.pdu.data) ?? unreachable;
    expect_eq(it.count(), 3 as usize);
    expect_eq(it.next() ?? 0xFFFF, 0x022B as u16);
    expect_eq(it.next() ?? 0xFFFF, 0x0000 as u16);
    expect_eq(it.next() ?? 0xFFFF, 0x0064 as u16);
    expect_eq(it.next() == null, true);

    // Empty 0-register response (byte_count == 0)
    let empty_pdu: [1]u8 = [0x00];
    let empty_it = parseReadRegsResp(empty_pdu[0..]) ?? unreachable;
    expect_eq(empty_it.count(), 0 as usize);
    expect_eq(empty_it.next() == null, true);
});

test("4. Exception frames (fc | 0x80) decoding over both TCP and RTU", () => {
    expect_eq(EX_ILLEGAL_FUNCTION, 0x01 as u8);
    expect_eq(EX_ILLEGAL_DATA_ADDR, 0x02 as u8);
    expect_eq(EX_ILLEGAL_DATA_VALUE, 0x03 as u8);
    expect_eq(EX_SERVER_FAILURE, 0x04 as u8);
    expect_eq(EX_ACKNOWLEDGE, 0x05 as u8);
    expect_eq(EX_SERVER_BUSY, 0x06 as u8);

    // TCP Exception: FC_READ_HOLDING_REGS | 0x80 = 0x83, EX_ILLEGAL_DATA_ADDR (0x02)
    let ex_payload: [1]u8 = [EX_ILLEGAL_DATA_ADDR];
    let tcp_buf: [32]u8 = std.mem.zeroes([32]u8);
    const tn = encodeModbusTcp(0x0009, 0x05, FC_READ_HOLDING_REGS | MODBUS_EXCEPTION_BIT, ex_payload[0..], tcp_buf[0..]) ?? unreachable;
    const tf = parseModbusTcp(tcp_buf[0..tn]) ?? unreachable;
    expect_eq(tf.pdu.isException(), true);
    expect_eq(tf.pdu.baseFuncCode(), FC_READ_HOLDING_REGS);
    expect_eq(tf.pdu.exceptionCode() ?? 0, EX_ILLEGAL_DATA_ADDR);

    // RTU Exception: FC_WRITE_SINGLE_REG | 0x80 = 0x86, EX_SERVER_BUSY (0x06)
    let ex_busy: [1]u8 = [EX_SERVER_BUSY];
    let rtu_buf: [16]u8 = std.mem.zeroes([16]u8);
    const rn = encodeModbusRtu(0x0A, FC_WRITE_SINGLE_REG | MODBUS_EXCEPTION_BIT, ex_busy[0..], rtu_buf[0..]) ?? unreachable;
    expect_eq(rn, 5 as usize);
    const rf = parseModbusRtu(rtu_buf[0..rn]) ?? unreachable;
    expect_eq(rf.slave_addr, 0x0A as u8);
    expect_eq(rf.pdu.isException(), true);
    expect_eq(rf.pdu.baseFuncCode(), FC_WRITE_SINGLE_REG);
    expect_eq(rf.pdu.exceptionCode() ?? 0, EX_SERVER_BUSY);
});

test("5. Malformed frames: bad CRC, non-zero protocol_id, oversize PDU (>253B), and truncation rejection", () => {
    // 1) Corrupted RTU CRC rejected
    let bad_rtu: [8]u8 = [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCE];
    expect_eq(verifyModbusRtuCrc(bad_rtu[0..]), false);
    expect_eq(parseModbusRtu(bad_rtu[0..]) == null, true);
    // Too short RTU (< 4 bytes)
    let short_rtu: [3]u8 = [0x01, 0x03, 0x00];
    expect_eq(verifyModbusRtuCrc(short_rtu[0..]), false);
    expect_eq(parseModbusRtu(short_rtu[0..]) == null, true);

    // 2) Non-zero protocol_id in Modbus TCP rejected
    let bad_proto: [12]u8 = [0x00, 0x01, 0x00, 0x01, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01];
    expect_eq(parseModbusTcp(bad_proto[0..]) == null, true);

    // 3) MBAP length < 2 rejected
    let bad_len_small: [8]u8 = [0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03];
    expect_eq(parseModbusTcp(bad_len_small[0..]) == null, true);

    // 4) Truncated TCP payload (6 + length > data.len) rejected
    let trunc_tcp: [10]u8 = [0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00];
    expect_eq(parseModbusTcp(trunc_tcp[0..]) == null, true);

    // 5) Oversize PDU (> 253 bytes, i.e., pdu_data.len > 252) rejected by both TCP and RTU encoders & parsers
    let max_pdu_data: [252]u8 = std.mem.zeroes([252]u8);
    let over_pdu_data: [253]u8 = std.mem.zeroes([253]u8);
    let big_buf: [300]u8 = std.mem.zeroes([300]u8);

    // Exact 253B PDU (1B fc + 252B data) succeeds
    const ok_tcp = encodeModbusTcp(1, 1, FC_WRITE_MULTI_REGS, max_pdu_data[0..], big_buf[0..]) ?? unreachable;
    expect_eq(ok_tcp, 260 as usize);
    expect_eq(parseModbusTcp(big_buf[0..ok_tcp]) != null, true);

    const ok_rtu = encodeModbusRtu(1, FC_WRITE_MULTI_REGS, max_pdu_data[0..], big_buf[0..]) ?? unreachable;
    expect_eq(ok_rtu, 256 as usize);
    expect_eq(parseModbusRtu(big_buf[0..ok_rtu]) != null, true);

    // 254B PDU (1B fc + 253B data) rejected
    expect_eq(encodeModbusTcp(1, 1, FC_WRITE_MULTI_REGS, over_pdu_data[0..], big_buf[0..]) == null, true);
    expect_eq(encodeModbusRtu(1, FC_WRITE_MULTI_REGS, over_pdu_data[0..], big_buf[0..]) == null, true);

    // 6) Malformed ReadReq / WriteSingle / ReadRegsResp (odd byte_count or length mismatch)
    let three_bytes: [3]u8 = [0x00, 0x01, 0x02];
    expect_eq(parseReadReq(three_bytes[0..]) == null, true);
    expect_eq(parseWriteSingle(three_bytes[0..]) == null, true);

    let odd_regs: [4]u8 = [0x03, 0x00, 0x01, 0x02];
    expect_eq(parseReadRegsResp(odd_regs[0..]) == null, true);

    let mismatch_regs: [4]u8 = [0x04, 0x00, 0x01, 0x02];
    expect_eq(parseReadRegsResp(mismatch_regs[0..]) == null, true);
});
