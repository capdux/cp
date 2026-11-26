// Type definitions for CP

export class FrameHeader {
    fin: bool,
    rsv: u8,
    opcode: u8,
    masked: bool,
    mask_key: [4]u8,
    payload_len: u64,
    header_len: usize,
}

export fn isControlOpcode(opcode: u8): bool;
export fn isValidOpcode(opcode: u8): bool;
export fn applyMask(payload: []u8, mask_key: [4]u8);
export fn decodeHeader(data: &[]u8, out: &mut FrameHeader): i32;
export fn encodeHeader(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, out: []u8): usize;
export fn encodeFrame(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload: &[]u8, out: []u8): usize;
export fn encodeClosePayload(code: u16, reason: string, out: []u8): usize;
export fn isValidCloseCode(code: u16): bool;
