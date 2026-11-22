// Origin: Pure-CP Stdlib (Zero-Alloc Modbus TCP MBAP & Modbus RTU CRC-16 Codec) | Created: 2026-11-22

pub const MODBUS_TCP_PORT: u16 = 502;
pub const MODBUS_MBAP_SIZE: usize = 7;
pub const MODBUS_PROTOCOL_ID: u16 = 0;
pub const MODBUS_MAX_PDU_SIZE: usize = 253;
pub const MODBUS_EXCEPTION_BIT: u8 = 0x80;

// Modbus Standard Function Codes
pub const FC_READ_COILS: u8 = 0x01;
pub const FC_READ_DISCRETE_INPUTS: u8 = 0x02;
pub const FC_READ_HOLDING_REGS: u8 = 0x03;
pub const FC_READ_INPUT_REGS: u8 = 0x04;
pub const FC_WRITE_SINGLE_COIL: u8 = 0x05;
pub const FC_WRITE_SINGLE_REG: u8 = 0x06;
pub const FC_WRITE_MULTI_COILS: u8 = 0x0F;
pub const FC_WRITE_MULTI_REGS: u8 = 0x10;

// Modbus Standard Exception Codes
pub const EX_ILLEGAL_FUNCTION: u8 = 0x01;
pub const EX_ILLEGAL_DATA_ADDR: u8 = 0x02;
pub const EX_ILLEGAL_DATA_VALUE: u8 = 0x03;
pub const EX_SERVER_FAILURE: u8 = 0x04;
pub const EX_ACKNOWLEDGE: u8 = 0x05;
pub const EX_SERVER_BUSY: u8 = 0x06;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const hi = buf[off] as u16;
    const lo = buf[off + 1] as u16;
    return (hi << 8) | lo;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn readU16Le(buf: []const u8, off: usize): u16 {
    const lo = buf[off] as u16;
    const hi = buf[off + 1] as u16;
    return (hi << 8) | lo;
}

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

/// Computes CRC-16/Modbus (reflected polynomial `0xA001`, init `0xFFFF`).
pub fn modbusCrc16(data: []const u8): u16 {
    let crc: u16 = 0xFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc ^= data[i] as u16;
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 0x0001) != 0) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// Verifies the trailing 2-byte little-endian CRC-16/Modbus of an RTU frame.
pub fn verifyModbusRtuCrc(frame: []const u8): bool {
    if (frame.len < 4) {
        return false;
    }
    const payload = frame[0 .. frame.len - 2];
    const expected = modbusCrc16(payload);
    const actual = readU16Le(frame, frame.len - 2);
    return expected == actual;
}

/// Modbus Protocol Data Unit (1-byte function code + payload bytes).
pub struct ModbusPdu {
    func_code: u8,
    data: []const u8,

    fn isException(): bool {
        return (_.func_code & MODBUS_EXCEPTION_BIT) != 0;
    }

    fn baseFuncCode(): u8 {
        return _.func_code & 0x7F;
    }

    fn exceptionCode(): u8? {
        if (_.isException() && _.data.len == 1) {
            return _.data[0];
        }
        return null;
    }
}

/// Parsed Modbus TCP frame (7-byte MBAP header + PDU).
pub struct ModbusTcpFrame {
    tx_id: u16,
    protocol_id: u16,
    length: u16,
    unit_id: u8,
    pdu: ModbusPdu,
}

/// Parsed Modbus RTU frame (1-byte slave address + PDU + 2-byte little-endian CRC-16).
pub struct ModbusRtuFrame {
    slave_addr: u8,
    pdu: ModbusPdu,
    crc: u16,
}

/// Parsed payload for Read Coils / Discrete Inputs / Holding / Input Registers (FC 0x01..0x04).
pub struct ModbusReadReq {
    start_addr: u16,
    quantity: u16,
}

/// Parsed payload for Write Single Coil / Write Single Register (FC 0x05 / 0x06).
pub struct ModbusWriteSingle {
    addr: u16,
    value: u16,
}

/// Zero-copy iterator over big-endian 16-bit registers in a Read Holding/Input Registers response.
pub struct ModbusRegIter {
    data: []const u8,
    offset: usize,

    fn count(): usize {
        return _.data.len / 2;
    }

    fn next(*_): u16? {
        if (_.offset + 2 > _.data.len) {
            return null;
        }
        const val = readU16Be(_.data, _.offset);
        _.offset += 2;
        return val;
    }
}

/// Strictly parses a Modbus TCP frame.
/// Validates `protocol_id == 0`, `length >= 2`, `6 + length <= data.len`, and `pdu_len <= 253`.
pub fn parseModbusTcp(data: []const u8): ModbusTcpFrame? {
    if (data.len < MODBUS_MBAP_SIZE + 1) {
        return null;
    }
    const tx_id = readU16Be(data, 0);
    const protocol_id = readU16Be(data, 2);
    if (protocol_id != MODBUS_PROTOCOL_ID) {
        return null;
    }
    const length = readU16Be(data, 4);
    if (length < 2) {
        return null;
    }
    const length_usize = length as usize;
    const pdu_len = length_usize - 1;
    if (pdu_len > MODBUS_MAX_PDU_SIZE) {
        return null;
    }
    const frame_end = 6 + length_usize;
    if (frame_end > data.len) {
        return null;
    }
    const unit_id = data[6];
    const func_code = data[7];
    const pdu_data = data[8..frame_end];
    return ModbusTcpFrame{
        .tx_id = tx_id,
        .protocol_id = protocol_id,
        .length = length,
        .unit_id = unit_id,
        .pdu = ModbusPdu{
            .func_code = func_code,
            .data = pdu_data,
        },
    };
}

/// Zero-alloc encoder for a Modbus TCP frame (MBAP header + PDU).
pub fn encodeModbusTcp(tx_id: u16, unit_id: u8, func_code: u8, pdu_data: []const u8, out: []u8): usize? {
    const pdu_len = 1 + pdu_data.len;
    if (pdu_len > MODBUS_MAX_PDU_SIZE) {
        return null;
    }
    const total = MODBUS_MBAP_SIZE + pdu_len;
    if (out.len < total) {
        return null;
    }
    const mbap_len = (1 + pdu_len) as u16;
    writeU16Be(out, 0, tx_id);
    writeU16Be(out, 2, MODBUS_PROTOCOL_ID);
    writeU16Be(out, 4, mbap_len);
    out[6] = unit_id;
    out[7] = func_code;
    let i: usize = 0;
    while (i < pdu_data.len) {
        out[8 + i] = pdu_data[i];
        i += 1;
    }
    return total;
}

/// Strictly parses a Modbus RTU frame and verifies its trailing 2-byte little-endian CRC-16.
pub fn parseModbusRtu(data: []const u8): ModbusRtuFrame? {
    if (data.len < 4) {
        return null;
    }
    const pdu_len = data.len - 3;
    if (pdu_len > MODBUS_MAX_PDU_SIZE) {
        return null;
    }
    if (!verifyModbusRtuCrc(data)) {
        return null;
    }
    const slave_addr = data[0];
    const func_code = data[1];
    const pdu_data = data[2 .. data.len - 2];
    const crc = readU16Le(data, data.len - 2);
    return ModbusRtuFrame{
        .slave_addr = slave_addr,
        .pdu = ModbusPdu{
            .func_code = func_code,
            .data = pdu_data,
        },
        .crc = crc,
    };
}

/// Zero-alloc encoder for a Modbus RTU frame (slave_addr + PDU + little-endian CRC-16).
pub fn encodeModbusRtu(slave_addr: u8, func_code: u8, pdu_data: []const u8, out: []u8): usize? {
    const pdu_len = 1 + pdu_data.len;
    if (pdu_len > MODBUS_MAX_PDU_SIZE) {
        return null;
    }
    const total = 1 + pdu_len + 2;
    if (out.len < total) {
        return null;
    }
    out[0] = slave_addr;
    out[1] = func_code;
    let i: usize = 0;
    while (i < pdu_data.len) {
        out[2 + i] = pdu_data[i];
        i += 1;
    }
    const crc = modbusCrc16(out[0 .. total - 2]);
    writeU16Le(out, total - 2, crc);
    return total;
}

/// Parses a 4-byte Read Request PDU payload (`start_addr`, `quantity`).
pub fn parseReadReq(pdu_data: []const u8): ModbusReadReq? {
    if (pdu_data.len != 4) {
        return null;
    }
    return ModbusReadReq{
        .start_addr = readU16Be(pdu_data, 0),
        .quantity = readU16Be(pdu_data, 2),
    };
}

/// Zero-alloc encoder for a 4-byte Read Request PDU payload.
pub fn encodeReadReq(start_addr: u16, quantity: u16, out: []u8): usize? {
    if (out.len < 4) {
        return null;
    }
    writeU16Be(out, 0, start_addr);
    writeU16Be(out, 2, quantity);
    return 4;
}

/// Parses a 4-byte Write Single Coil/Register PDU payload (`addr`, `value`).
pub fn parseWriteSingle(pdu_data: []const u8): ModbusWriteSingle? {
    if (pdu_data.len != 4) {
        return null;
    }
    return ModbusWriteSingle{
        .addr = readU16Be(pdu_data, 0),
        .value = readU16Be(pdu_data, 2),
    };
}

/// Zero-alloc encoder for a 4-byte Write Single Coil/Register PDU payload.
pub fn encodeWriteSingle(addr: u16, value: u16, out: []u8): usize? {
    if (out.len < 4) {
        return null;
    }
    writeU16Be(out, 0, addr);
    writeU16Be(out, 2, value);
    return 4;
}

/// Parses a Read Holding/Input Registers response payload (`[byte_count, reg0_hi, reg0_lo, ...]`)
/// into a zero-copy `ModbusRegIter`.
pub fn parseReadRegsResp(pdu_data: []const u8): ModbusRegIter? {
    if (pdu_data.len < 1) {
        return null;
    }
    const byte_count = pdu_data[0] as usize;
    if ((byte_count & 1) != 0) {
        return null;
    }
    if (1 + byte_count != pdu_data.len) {
        return null;
    }
    return ModbusRegIter{
        .data = pdu_data[1 .. 1 + byte_count],
        .offset = 0,
    };
}
