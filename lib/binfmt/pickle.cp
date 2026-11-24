// Origin: Pure-CP Stdlib (Zero-Alloc Python Pickle Protocol 2-5 Parser & Static Auditor) | Created: 2026-11-22

pub const OP_PROTO: u8 = 0x80;
pub const OP_FRAME: u8 = 0x95;
pub const OP_STOP: u8 = 0x2E;
pub const OP_NONE: u8 = 0x4E;
pub const OP_NEWTRUE: u8 = 0x88;
pub const OP_NEWFALSE: u8 = 0x89;
pub const OP_BININT: u8 = 0x4A;
pub const OP_BININT1: u8 = 0x4B;
pub const OP_BININT2: u8 = 0x4D;
pub const OP_LONG1: u8 = 0x8A;
pub const OP_LONG4: u8 = 0x8B;
pub const OP_BINFLOAT: u8 = 0x47;
pub const OP_SHORT_BINSTRING: u8 = 0x55;
pub const OP_BINSTRING: u8 = 0x54;
pub const OP_SHORT_BINBYTES: u8 = 0x43;
pub const OP_BINBYTES: u8 = 0x42;
pub const OP_BINBYTES8: u8 = 0x8E;
pub const OP_SHORT_BINUNICODE: u8 = 0x8C;
pub const OP_BINUNICODE: u8 = 0x58;
pub const OP_BINUNICODE8: u8 = 0x8D;
pub const OP_EMPTY_LIST: u8 = 0x5D;
pub const OP_EMPTY_DICT: u8 = 0x7D;
pub const OP_EMPTY_TUPLE: u8 = 0x29;
pub const OP_TUPLE1: u8 = 0x85;
pub const OP_TUPLE2: u8 = 0x86;
pub const OP_TUPLE3: u8 = 0x87;
pub const OP_TUPLE: u8 = 0x74;
pub const OP_MARK: u8 = 0x28;
pub const OP_APPEND: u8 = 0x61;
pub const OP_APPENDS: u8 = 0x65;
pub const OP_SETITEM: u8 = 0x73;
pub const OP_SETITEMS: u8 = 0x75;
pub const OP_BINGET: u8 = 0x68;
pub const OP_LONG_BINGET: u8 = 0x6A;
pub const OP_BINPUT: u8 = 0x71;
pub const OP_LONG_BINPUT: u8 = 0x72;
pub const OP_MEMOIZE: u8 = 0x94;
pub const OP_GLOBAL: u8 = 0x63;
pub const OP_STACK_GLOBAL: u8 = 0x93;
pub const OP_REDUCE: u8 = 0x52;
pub const OP_BUILD: u8 = 0x62;
pub const OP_NEWOBJ: u8 = 0x81;
pub const OP_NEWOBJ_EX: u8 = 0x92;
pub const OP_INST: u8 = 0x69;
pub const OP_OBJ: u8 = 0x6F;
pub const OP_PERSID: u8 = 0x50;
pub const OP_BINPERSID: u8 = 0x51;
pub const OP_EXT1: u8 = 0x82;
pub const OP_EXT2: u8 = 0x83;
pub const OP_EXT4: u8 = 0x84;
pub const OP_POP: u8 = 0x30;
pub const OP_POP_MARK: u8 = 0x31;
pub const OP_DUP: u8 = 0x32;
pub const OP_EMPTY_SET: u8 = 0x8F;
pub const OP_ADDITEMS: u8 = 0x90;
pub const OP_FROZENSET: u8 = 0x91;

pub struct PickleOp {
    opcode: u8,
    offset: usize,
    byte_len: usize,
    int_val: i64,
    uint_val: u64,
    float_val: f64,
    text: []const u8,
    text2: []const u8,
}

pub struct PickleAudit {
    proto: u8,
    op_count: u64,
    global_count: u64,
    reduce_count: u64,
    frame_count: u64,
    has_stop: bool,
    has_unsafe_ops: bool,
    valid: bool,
}

pub fn detectPickleProto(data: []const u8): u8? {
    if (data.len < 2 || data[0] != OP_PROTO) {
        return null;
    }
    const ver = data[1];
    if (ver >= 2 && ver <= 5) {
        return ver;
    }
    return null;
}

pub fn isPickle(data: []const u8): bool {
    if (_v := detectPickleProto(data)) {
        return true;
    }
    return false;
}

fn readU16LE(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn readU64LE(buf: []const u8, off: usize): u64 {
    const lo = readU32LE(buf, off) as u64;
    const hi = readU32LE(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn readU64BE(buf: []const u8, off: usize): u64 {
    const b0 = buf[off] as u64;
    const b1 = buf[off + 1] as u64;
    const b2 = buf[off + 2] as u64;
    const b3 = buf[off + 3] as u64;
    const b4 = buf[off + 4] as u64;
    const b5 = buf[off + 5] as u64;
    const b6 = buf[off + 6] as u64;
    const b7 = buf[off + 7] as u64;
    return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) |
        (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
}

fn decodeSignedLeTwosComplement(bytes: []const u8): i64 {
    if (bytes.len == 0 || bytes.len > 8) {
        return 0;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        const shift: u6 = (i * 8) as u6;
        acc = acc | ((bytes[i] as u64) << shift);
        i += 1;
    }
    // Sign-extend if MSB of highest byte is set
    if ((bytes[bytes.len - 1] & 0x80) != 0 && bytes.len < 8) {
        const sign_shift: u6 = (bytes.len * 8) as u6;
        acc = acc | ((0xFFFF_FFFF_FFFF_FFFF as u64) << sign_shift);
    }
    unsafe {
        const v: i64 = @bitCast(acc);
        return v;
    }
}

fn findNewline(buf: []const u8, start: usize): usize? {
    let i: usize = start;
    while (i < buf.len) {
        if (buf[i] == '\n') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn isZeroArgOp(op: u8): bool {
    return op == OP_STOP ||
        op == OP_NONE ||
        op == OP_NEWTRUE ||
        op == OP_NEWFALSE ||
        op == OP_EMPTY_LIST ||
        op == OP_EMPTY_DICT ||
        op == OP_EMPTY_TUPLE ||
        op == OP_TUPLE1 ||
        op == OP_TUPLE2 ||
        op == OP_TUPLE3 ||
        op == OP_TUPLE ||
        op == OP_MARK ||
        op == OP_APPEND ||
        op == OP_APPENDS ||
        op == OP_SETITEM ||
        op == OP_SETITEMS ||
        op == OP_MEMOIZE ||
        op == OP_STACK_GLOBAL ||
        op == OP_REDUCE ||
        op == OP_BUILD ||
        op == OP_NEWOBJ ||
        op == OP_NEWOBJ_EX ||
        op == OP_OBJ ||
        op == OP_BINPERSID ||
        op == OP_POP ||
        op == OP_POP_MARK ||
        op == OP_DUP ||
        op == OP_EMPTY_SET ||
        op == OP_ADDITEMS ||
        op == OP_FROZENSET;
}

pub fn isUnsafePickleOp(op: u8): bool {
    return op == OP_GLOBAL ||
        op == OP_STACK_GLOBAL ||
        op == OP_REDUCE ||
        op == OP_BUILD ||
        op == OP_NEWOBJ ||
        op == OP_NEWOBJ_EX ||
        op == OP_INST ||
        op == OP_OBJ ||
        op == OP_PERSID ||
        op == OP_BINPERSID ||
        op == OP_EXT1 ||
        op == OP_EXT2 ||
        op == OP_EXT4;
}

pub struct PickleIter {
    data: []const u8,
    pos: usize,
    done: bool,
    had_error: bool,

    static fn init(data: []const u8): PickleIter {
        return PickleIter{
            .data = data,
            .pos = 0,
            .done = false,
            .had_error = false,
        };
    }

    fn next(*_): PickleOp? {
        if (_.done || _.pos >= _.data.len) {
            _.done = true;
            return null;
        }

        const start = _.pos;
        const op = _.data[start];
        const rem = _.data.len - start;
        const empty_slice = _.data[0..0];

        if (isZeroArgOp(op)) {
            _.pos = start + 1;
            if (op == OP_STOP) {
                _.done = true;
            }
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 1,
                .int_val = 0,
                .uint_val = 0,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 1-byte unsigned integer argument
        if (op == OP_PROTO || op == OP_BININT1 || op == OP_BINGET || op == OP_BINPUT || op == OP_EXT1) {
            if (rem < 2) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const v = _.data[start + 1];
            _.pos = start + 2;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 2,
                .int_val = v as i64,
                .uint_val = v as u64,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 2-byte LE unsigned integer argument
        if (op == OP_BININT2 || op == OP_EXT2) {
            if (rem < 3) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const v = readU16LE(_.data, start + 1);
            _.pos = start + 3;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 3,
                .int_val = v as i64,
                .uint_val = v as u64,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 4-byte LE signed i32 argument
        if (op == OP_BININT || op == OP_EXT4) {
            if (rem < 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const raw = readU32LE(_.data, start + 1);
            let sv: i32 = 0;
            unsafe { sv = @bitCast(raw); }
            _.pos = start + 5;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 5,
                .int_val = sv as i64,
                .uint_val = raw as u64,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 4-byte LE unsigned u32 argument
        if (op == OP_LONG_BINGET || op == OP_LONG_BINPUT) {
            if (rem < 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const v = readU32LE(_.data, start + 1);
            _.pos = start + 5;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 5,
                .int_val = v as i64,
                .uint_val = v as u64,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 8-byte LE frame length
        if (op == OP_FRAME) {
            if (rem < 9) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const v = readU64LE(_.data, start + 1);
            _.pos = start + 9;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 9,
                .int_val = 0,
                .uint_val = v,
                .float_val = 0.0,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 8-byte BE IEEE-754 float64
        if (op == OP_BINFLOAT) {
            if (rem < 9) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const bits = readU64BE(_.data, start + 1);
            let fv: f64 = 0.0;
            unsafe { fv = @bitCast(bits); }
            _.pos = start + 9;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 9,
                .int_val = 0,
                .uint_val = bits,
                .float_val = fv,
                .text = empty_slice,
                .text2 = empty_slice,
            };
        }

        // 1-byte length prefixed bytes/unicode/long1
        if (op == OP_SHORT_BINSTRING || op == OP_SHORT_BINBYTES || op == OP_SHORT_BINUNICODE || op == OP_LONG1) {
            if (rem < 2) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const n: usize = _.data[start + 1] as usize;
            if (n > rem - 2) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const payload = _.data[(start + 2)..(start + 2 + n)];
            _.pos = start + 2 + n;
            let iv: i64 = 0;
            if (op == OP_LONG1) {
                iv = decodeSignedLeTwosComplement(payload);
            }
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 2 + n,
                .int_val = iv,
                .uint_val = n as u64,
                .float_val = 0.0,
                .text = payload,
                .text2 = empty_slice,
            };
        }

        // 4-byte LE signed length prefixed (BINSTRING / LONG4)
        if (op == OP_BINSTRING || op == OP_LONG4) {
            if (rem < 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const raw = readU32LE(_.data, start + 1);
            let sn: i32 = 0;
            unsafe { sn = @bitCast(raw); }
            if (sn < 0) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const n: usize = sn as usize;
            if (n > rem - 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const payload = _.data[(start + 5)..(start + 5 + n)];
            _.pos = start + 5 + n;
            let iv: i64 = 0;
            if (op == OP_LONG4) {
                iv = decodeSignedLeTwosComplement(payload);
            }
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 5 + n,
                .int_val = iv,
                .uint_val = n as u64,
                .float_val = 0.0,
                .text = payload,
                .text2 = empty_slice,
            };
        }

        // 4-byte LE unsigned length prefixed (BINBYTES / BINUNICODE)
        if (op == OP_BINBYTES || op == OP_BINUNICODE) {
            if (rem < 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const n_u32 = readU32LE(_.data, start + 1);
            const n: usize = n_u32 as usize;
            if (n > rem - 5) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const payload = _.data[(start + 5)..(start + 5 + n)];
            _.pos = start + 5 + n;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 5 + n,
                .int_val = 0,
                .uint_val = n_u32 as u64,
                .float_val = 0.0,
                .text = payload,
                .text2 = empty_slice,
            };
        }

        // 8-byte LE unsigned length prefixed (BINBYTES8 / BINUNICODE8)
        if (op == OP_BINBYTES8 || op == OP_BINUNICODE8) {
            if (rem < 9) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const n_u64 = readU64LE(_.data, start + 1);
            if (n_u64 > ((rem - 9) as u64)) {
                _.had_error = true;
                _.done = true;
                return null;
            }
            const n: usize = n_u64 as usize;
            const payload = _.data[(start + 9)..(start + 9 + n)];
            _.pos = start + 9 + n;
            return PickleOp{
                .opcode = op,
                .offset = start,
                .byte_len = 9 + n,
                .int_val = 0,
                .uint_val = n_u64,
                .float_val = 0.0,
                .text = payload,
                .text2 = empty_slice,
            };
        }

        // Two newline-terminated lines: module\nname\n (GLOBAL / INST)
        if (op == OP_GLOBAL || op == OP_INST) {
            if (nl1 := findNewline(_.data, start + 1)) {
                if (nl2 := findNewline(_.data, nl1 + 1)) {
                    const mod_name = _.data[(start + 1)..nl1];
                    const attr_name = _.data[(nl1 + 1)..nl2];
                    _.pos = nl2 + 1;
                    return PickleOp{
                        .opcode = op,
                        .offset = start,
                        .byte_len = (nl2 + 1) - start,
                        .int_val = 0,
                        .uint_val = 0,
                        .float_val = 0.0,
                        .text = mod_name,
                        .text2 = attr_name,
                    };
                }
            }
            _.had_error = true;
            _.done = true;
            return null;
        }

        // Single newline-terminated line: pid\n (PERSID)
        if (op == OP_PERSID) {
            if (nl := findNewline(_.data, start + 1)) {
                const pid = _.data[(start + 1)..nl];
                _.pos = nl + 1;
                return PickleOp{
                    .opcode = op,
                    .offset = start,
                    .byte_len = (nl + 1) - start,
                    .int_val = 0,
                    .uint_val = 0,
                    .float_val = 0.0,
                    .text = pid,
                    .text2 = empty_slice,
                };
            }
            _.had_error = true;
            _.done = true;
            return null;
        }

        // Unknown / unsupported opcode
        _.had_error = true;
        _.done = true;
        return null;
    }
}

pub fn auditPickle(data: []const u8): PickleAudit {
    let proto: u8 = 0;
    let op_count: u64 = 0;
    let global_count: u64 = 0;
    let reduce_count: u64 = 0;
    let frame_count: u64 = 0;
    let has_stop = false;
    let has_unsafe_ops = false;

    let it = PickleIter.init(data);
    while (op := it.next()) {
        op_count += 1;
        if (op.opcode == OP_PROTO) {
            proto = op.uint_val as u8;
        } else if (op.opcode == OP_FRAME) {
            frame_count += 1;
        } else if (op.opcode == OP_STOP) {
            has_stop = true;
        }

        if (op.opcode == OP_GLOBAL || op.opcode == OP_STACK_GLOBAL) {
            global_count += 1;
        }
        if (op.opcode == OP_REDUCE) {
            reduce_count += 1;
        }
        if (isUnsafePickleOp(op.opcode)) {
            has_unsafe_ops = true;
        }
    }

    return PickleAudit{
        .proto = proto,
        .op_count = op_count,
        .global_count = global_count,
        .reduce_count = reduce_count,
        .frame_count = frame_count,
        .has_stop = has_stop,
        .has_unsafe_ops = has_unsafe_ops,
        .valid = has_stop && !it.had_error,
    };
}
