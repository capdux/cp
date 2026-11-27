// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1350 / RFC 2347-2349 TFTP Wire Codec) | Created: 2026-11-22

pub const TFTP_UDP_PORT: u16 = 69;
pub const TFTP_DEFAULT_BLKSIZE: usize = 512;
pub const TFTP_DATA_HEADER_SIZE: usize = 4;

// TFTP Opcodes (RFC 1350 + RFC 2347)
pub const OP_RRQ: u16 = 1;
pub const OP_WRQ: u16 = 2;
pub const OP_DATA: u16 = 3;
pub const OP_ACK: u16 = 4;
pub const OP_ERROR: u16 = 5;
pub const OP_OACK: u16 = 6;

// TFTP Error Codes (RFC 1350 + RFC 2347)
pub const ERR_NOT_DEFINED: u16 = 0;
pub const ERR_FILE_NOT_FOUND: u16 = 1;
pub const ERR_ACCESS_VIOLATION: u16 = 2;
pub const ERR_DISK_FULL: u16 = 3;
pub const ERR_ILLEGAL_OP: u16 = 4;
pub const ERR_UNKNOWN_TID: u16 = 5;
pub const ERR_FILE_EXISTS: u16 = 6;
pub const ERR_NO_SUCH_USER: u16 = 7;
pub const ERR_OPT_NEGOTIATION: u16 = 8;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findNul(buf: []const u8, start: usize): usize? {
    let i = start;
    while (i < buf.len) {
        if (buf[i] == 0) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn containsNul(s: []const u8): bool {
    return findNul(s, 0) != null;
}

pub struct TftpOption {
    name: []const u8,
    value: []const u8,
}

/// Zero-copy iterator over RFC 2347 NUL-delimited `name\0value\0` option pairs.
pub struct TftpOptIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    static fn init(options_bytes: []const u8): TftpOptIter {
        return TftpOptIter{
            .data = options_bytes,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): TftpOption? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        let name_end: usize = 0;
        if (ne := findNul(_.data, _.offset)) {
            name_end = ne;
        } else {
            _.failed = true;
            return null;
        }
        const val_start = name_end + 1;
        if (val_start >= _.data.len) {
            // Odd number of NUL-terminated strings (missing value)
            _.failed = true;
            return null;
        }
        let val_end: usize = 0;
        if (ve := findNul(_.data, val_start)) {
            val_end = ve;
        } else {
            _.failed = true;
            return null;
        }
        const name = _.data[_.offset..name_end];
        const value = _.data[val_start..val_end];
        _.offset = val_end + 1;
        return TftpOption{
            .name = name,
            .value = value,
        };
    }
}

fn validateOptions(options_bytes: []const u8): bool {
    let it = TftpOptIter.init(options_bytes);
    while (true) {
        if (opt := it.next()) {
            * = opt;
        } else {
            break;
        }
    }
    return !it.hasError();
}

fn findOptionIn(options_bytes: []const u8, name: []const u8): ([]const u8)? {
    let it = TftpOptIter.init(options_bytes);
    while (true) {
        if (opt := it.next()) {
            if (asciiEqIgnoreCase(opt.name, name)) {
                return opt.value;
            }
        } else {
            break;
        }
    }
    return null;
}

pub struct TftpRequest {
    is_write: bool,
    filename: []const u8,
    mode: []const u8,
    options_bytes: []const u8,

    fn options(): TftpOptIter {
        return TftpOptIter.init(_.options_bytes);
    }

    fn findOption(name: []const u8): ([]const u8)? {
        return findOptionIn(_.options_bytes, name);
    }
}

pub struct TftpData {
    block: u16,
    payload: []const u8,

    fn isLastBlock(blksize: usize): bool {
        return _.payload.len < blksize;
    }
}

pub struct TftpAck {
    block: u16,
}

pub struct TftpError {
    code: u16,
    message: []const u8,
}

pub struct TftpOack {
    options_bytes: []const u8,

    fn options(): TftpOptIter {
        return TftpOptIter.init(_.options_bytes);
    }

    fn findOption(name: []const u8): ([]const u8)? {
        return findOptionIn(_.options_bytes, name);
    }
}

/// Peeks the 2-byte Big-Endian TFTP opcode if valid (1..6).
pub fn peekTftpOpcode(data: []const u8): u16? {
    if (data.len < 2) {
        return null;
    }
    const op = readU16Be(data, 0);
    if (op < OP_RRQ || op > OP_OACK) {
        return null;
    }
    return op;
}

/// Fast validation checking whether `data` starts with a valid TFTP opcode and minimum packet size.
pub fn isTftp(data: []const u8): bool {
    const op = peekTftpOpcode(data) ?? return false;
    if (op == OP_ACK) return data.len == 4;
    if (op == OP_DATA) return data.len >= 4;
    if (op == OP_ERROR) return data.len >= 5;
    if (op == OP_RRQ || op == OP_WRQ) return data.len >= 4;
    return data.len >= 2;
}

/// Parses a TFTP RRQ (1) or WRQ (2) packet including optional RFC 2347 option pairs.
pub fn parseTftpRequest(data: []const u8): TftpRequest? {
    const op = peekTftpOpcode(data) ?? return null;
    if (op != OP_RRQ && op != OP_WRQ) {
        return null;
    }
    const fname_end = findNul(data, 2) ?? return null;
    const mode_start = fname_end + 1;
    if (mode_start >= data.len) {
        return null;
    }
    const mode_end = findNul(data, mode_start) ?? return null;
    const opt_slice = data[(mode_end + 1)..];
    if (!validateOptions(opt_slice)) {
        return null;
    }
    return TftpRequest{
        .is_write = (op == OP_WRQ),
        .filename = data[2..fname_end],
        .mode = data[mode_start..mode_end],
        .options_bytes = opt_slice,
    };
}

/// Parses a TFTP DATA (3) packet.
pub fn parseTftpData(data: []const u8): TftpData? {
    if (data.len < TFTP_DATA_HEADER_SIZE) {
        return null;
    }
    if (readU16Be(data, 0) != OP_DATA) {
        return null;
    }
    return TftpData{
        .block = readU16Be(data, 2),
        .payload = data[TFTP_DATA_HEADER_SIZE..],
    };
}

/// Parses a TFTP ACK (4) packet. Strictly requires length == 4.
pub fn parseTftpAck(data: []const u8): TftpAck? {
    if (data.len != 4) {
        return null;
    }
    if (readU16Be(data, 0) != OP_ACK) {
        return null;
    }
    return TftpAck{
        .block = readU16Be(data, 2),
    };
}

/// Parses a TFTP ERROR (5) packet. Strictly requires trailing NUL termination.
pub fn parseTftpError(data: []const u8): TftpError? {
    if (data.len < 5) {
        return null;
    }
    if (readU16Be(data, 0) != OP_ERROR) {
        return null;
    }
    const nul_pos = findNul(data, 4) ?? return null;
    return TftpError{
        .code = readU16Be(data, 2),
        .message = data[4..nul_pos],
    };
}

/// Parses a TFTP OACK (6) packet and validates all NUL-delimited option pairs.
pub fn parseTftpOack(data: []const u8): TftpOack? {
    if (data.len < 2) {
        return null;
    }
    if (readU16Be(data, 0) != OP_OACK) {
        return null;
    }
    const opt_slice = data[2..];
    if (!validateOptions(opt_slice)) {
        return null;
    }
    return TftpOack{
        .options_bytes = opt_slice,
    };
}

/// Zero-alloc encoder for TFTP RRQ / WRQ without options.
pub fn encodeTftpRequest(is_write: bool, filename: []const u8, mode: []const u8, out: []u8): usize? {
    if (containsNul(filename) || containsNul(mode)) {
        return null;
    }
    const total = 2 + filename.len + 1 + mode.len + 1;
    if (out.len < total) {
        return null;
    }
    writeU16Be(out, 0, if (is_write) OP_WRQ else OP_RRQ);
    let pos: usize = 2;
    let i: usize = 0;
    while (i < filename.len) {
        out[pos + i] = filename[i];
        i += 1;
    }
    pos += filename.len;
    out[pos] = 0;
    pos += 1;

    let j: usize = 0;
    while (j < mode.len) {
        out[pos + j] = mode[j];
        j += 1;
    }
    pos += mode.len;
    out[pos] = 0;
    return total;
}

/// Zero-alloc encoder for TFTP DATA (3).
pub fn encodeTftpData(block: u16, payload: []const u8, out: []u8): usize? {
    const total = TFTP_DATA_HEADER_SIZE + payload.len;
    if (out.len < total) {
        return null;
    }
    writeU16Be(out, 0, OP_DATA);
    writeU16Be(out, 2, block);
    let i: usize = 0;
    while (i < payload.len) {
        out[TFTP_DATA_HEADER_SIZE + i] = payload[i];
        i += 1;
    }
    return total;
}

/// Zero-alloc encoder for TFTP ACK (4).
pub fn encodeTftpAck(block: u16, out: []u8): usize? {
    if (out.len < 4) {
        return null;
    }
    writeU16Be(out, 0, OP_ACK);
    writeU16Be(out, 2, block);
    return 4;
}

/// Zero-alloc encoder for TFTP ERROR (5).
pub fn encodeTftpError(code: u16, msg: []const u8, out: []u8): usize? {
    if (containsNul(msg)) {
        return null;
    }
    const total = 4 + msg.len + 1;
    if (out.len < total) {
        return null;
    }
    writeU16Be(out, 0, OP_ERROR);
    writeU16Be(out, 2, code);
    let i: usize = 0;
    while (i < msg.len) {
        out[4 + i] = msg[i];
        i += 1;
    }
    out[4 + msg.len] = 0;
    return total;
}

/// Zero-alloc option writer for appending RFC 2347 options to RRQ, WRQ, or OACK packets.
pub struct TftpOptWriter {
    buf: []u8,
    pos: usize,
    valid: bool,

    static fn initRequest(buf: []u8, is_write: bool, filename: []const u8, mode: []const u8): TftpOptWriter {
        if (n := encodeTftpRequest(is_write, filename, mode, buf)) {
            return TftpOptWriter{
                .buf = buf,
                .pos = n,
                .valid = true,
            };
        }
        return TftpOptWriter{
            .buf = buf,
            .pos = 0,
            .valid = false,
        };
    }

    static fn initOack(buf: []u8): TftpOptWriter {
        if (buf.len < 2) {
            return TftpOptWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        writeU16Be(buf, 0, OP_OACK);
        return TftpOptWriter{
            .buf = buf,
            .pos = 2,
            .valid = true,
        };
    }

    fn writeOption(*_, name: []const u8, value: []const u8): bool {
        if (!_.valid || containsNul(name) || containsNul(value)) {
            return false;
        }
        const need = name.len + 1 + value.len + 1;
        if (_.pos + need > _.buf.len) {
            return false;
        }
        let i: usize = 0;
        while (i < name.len) {
            _.buf[_.pos + i] = name[i];
            i += 1;
        }
        _.pos += name.len;
        _.buf[_.pos] = 0;
        _.pos += 1;

        let j: usize = 0;
        while (j < value.len) {
            _.buf[_.pos + j] = value[j];
            j += 1;
        }
        _.pos += value.len;
        _.buf[_.pos] = 0;
        _.pos += 1;
        return true;
    }

    fn finish(): ([]const u8)? {
        if (!_.valid) {
            return null;
        }
        return _.buf[0.._.pos];
    }
}
