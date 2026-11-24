// Origin: Pure-CP Stdlib (Zero-Alloc MS-SMB2 / SMB3 Wire Protocol Parser) | Created: 2026-11-22

// Standard SMB Ports & Header Sizes
pub const SMB_TCP_PORT: u16 = 445;
pub const SMB2_HEADER_SIZE: usize = 64;
pub const SMB3_TRANSFORM_HEADER_SIZE: usize = 52;
pub const NETBIOS_SESSION_HEADER_SIZE: usize = 4;

// Protocol Magic Bytes
pub const SMB2_MAGIC: [4]u8 = [0xFE, 'S', 'M', 'B'];
pub const SMB3_TRANSFORM_MAGIC: [4]u8 = [0xFD, 'S', 'M', 'B'];

// MS-SMB2 Commands (Section 2.2.1)
pub const CMD_NEGOTIATE: u16 = 0x00;
pub const CMD_SESSION_SETUP: u16 = 0x01;
pub const CMD_LOGOFF: u16 = 0x02;
pub const CMD_TREE_CONNECT: u16 = 0x03;
pub const CMD_TREE_DISCONNECT: u16 = 0x04;
pub const CMD_CREATE: u16 = 0x05;
pub const CMD_CLOSE: u16 = 0x06;
pub const CMD_FLUSH: u16 = 0x07;
pub const CMD_READ: u16 = 0x08;
pub const CMD_WRITE: u16 = 0x09;
pub const CMD_LOCK: u16 = 0x0A;
pub const CMD_IOCTL: u16 = 0x0B;
pub const CMD_CANCEL: u16 = 0x0C;
pub const CMD_ECHO: u16 = 0x0D;
pub const CMD_QUERY_DIRECTORY: u16 = 0x0E;
pub const CMD_CHANGE_NOTIFY: u16 = 0x0F;
pub const CMD_QUERY_INFO: u16 = 0x10;
pub const CMD_SET_INFO: u16 = 0x11;
pub const CMD_OPLOCK_BREAK: u16 = 0x12;

// MS-SMB2 Header Flags
pub const FLAG_SERVER_TO_REDIR: u32 = 0x00000001;
pub const FLAG_ASYNC_COMMAND: u32 = 0x00000002;
pub const FLAG_RELATED_OPERATIONS: u32 = 0x00000004;
pub const FLAG_SIGNED: u32 = 0x00000008;

// MS-SMB2 / SMB3 Dialects
pub const DIALECT_202: u16 = 0x0202;
pub const DIALECT_210: u16 = 0x0210;
pub const DIALECT_300: u16 = 0x0300;
pub const DIALECT_302: u16 = 0x0302;
pub const DIALECT_311: u16 = 0x0311;

fn readU16Le(data: []const u8, off: usize): u16 {
    const b0 = data[off] as u16;
    const b1 = (data[off + 1] as u16) << 8;
    return b0 | b1;
}

fn readU32Le(data: []const u8, off: usize): u32 {
    const b0 = data[off] as u32;
    const b1 = (data[off + 1] as u32) << 8;
    const b2 = (data[off + 2] as u32) << 16;
    const b3 = (data[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = (readU32Le(data, off + 4) as u64) << 32;
    return lo | hi;
}

fn readBytes16(data: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        out[i] = data[off + i];
        i += 1;
    }
    return out;
}

/// Unwraps a 4-byte NetBIOS Session Service / Direct TCP transport header (`0x00` + 24-bit BE length).
pub fn unwrapNetbiosSession(data: []const u8): ([]const u8)? {
    if (data.len < NETBIOS_SESSION_HEADER_SIZE || data[0] != 0x00) {
        return null;
    }
    const payload_len = ((data[1] as usize) << 16)
        | ((data[2] as usize) << 8)
        | (data[3] as usize);
    if (payload_len == 0 || payload_len > data.len - NETBIOS_SESSION_HEADER_SIZE) {
        return null;
    }
    return data[NETBIOS_SESSION_HEADER_SIZE..(NETBIOS_SESSION_HEADER_SIZE + payload_len)];
}

/// Returns `true` if `data` begins with the 4-byte SMB2 protocol magic (`0xFE, 'S', 'M', 'B'`).
pub fn isSmb2(data: []const u8): bool {
    return data.len >= 4
        && data[0] == SMB2_MAGIC[0]
        && data[1] == SMB2_MAGIC[1]
        && data[2] == SMB2_MAGIC[2]
        && data[3] == SMB2_MAGIC[3];
}

/// Returns `true` if `data` begins with the 4-byte SMB3 Transform magic (`0xFD, 'S', 'M', 'B'`).
pub fn isSmb3Transform(data: []const u8): bool {
    return data.len >= 4
        && data[0] == SMB3_TRANSFORM_MAGIC[0]
        && data[1] == SMB3_TRANSFORM_MAGIC[1]
        && data[2] == SMB3_TRANSFORM_MAGIC[2]
        && data[3] == SMB3_TRANSFORM_MAGIC[3];
}

/// MS-SMB2 §2.2.1 64-byte Synchronous or Asynchronous packet header.
pub struct Smb2Header {
    credit_charge: u16,
    status: u32,
    command: u16,
    credits: u16,
    flags: u32,
    next_command: u32,
    message_id: u64,
    is_async: bool,
    async_id: u64,
    process_id: u32,
    tree_id: u32,
    session_id: u64,
    signature: [16]u8,

    fn isResponse(): bool {
        return (_.flags & FLAG_SERVER_TO_REDIR) != 0;
    }

    fn isSigned(): bool {
        return (_.flags & FLAG_SIGNED) != 0;
    }

    fn isRelated(): bool {
        return (_.flags & FLAG_RELATED_OPERATIONS) != 0;
    }

    fn isChained(): bool {
        return _.next_command != 0;
    }
}

/// Parsed SMB2 packet containing its 64-byte header and zero-copy body slice.
pub struct Smb2Packet {
    header: Smb2Header,
    body: []const u8,
}

/// Parses a single SMB2 packet (or the first packet in a compound chain).
/// Strictly validates magic, `structure_size == 64`, and 8-byte aligned `next_command` bounds.
pub fn parseSmb2(data: []const u8): Smb2Packet? {
    if (data.len < SMB2_HEADER_SIZE || !isSmb2(data)) {
        return null;
    }
    const structure_size = readU16Le(data, 4);
    if (structure_size != 64) {
        return null;
    }
    const credit_charge = readU16Le(data, 6);
    const status = readU32Le(data, 8);
    const command = readU16Le(data, 12);
    const credits = readU16Le(data, 14);
    const flags = readU32Le(data, 16);
    const next_command = readU32Le(data, 20);
    const message_id = readU64Le(data, 24);

    const is_async = (flags & FLAG_ASYNC_COMMAND) != 0;
    let async_id: u64 = 0;
    let process_id: u32 = 0;
    let tree_id: u32 = 0;
    if (is_async) {
        async_id = readU64Le(data, 32);
    } else {
        process_id = readU32Le(data, 32);
        tree_id = readU32Le(data, 36);
    }

    const session_id = readU64Le(data, 40);
    const signature = readBytes16(data, 48);

    let body_end = data.len;
    if (next_command != 0) {
        const next_off = next_command as usize;
        if ((next_off & 7) != 0 || next_off < SMB2_HEADER_SIZE || next_off > data.len) {
            return null;
        }
        body_end = next_off;
    }

    return Smb2Packet{
        .header = Smb2Header{
            .credit_charge = credit_charge,
            .status = status,
            .command = command,
            .credits = credits,
            .flags = flags,
            .next_command = next_command,
            .message_id = message_id,
            .is_async = is_async,
            .async_id = async_id,
            .process_id = process_id,
            .tree_id = tree_id,
            .session_id = session_id,
            .signature = signature,
        },
        .body = data[SMB2_HEADER_SIZE..body_end],
    };
}

/// Zero-copy iterator over a compound/chained SMB2 message sequence.
pub struct Smb2ChainIter {
    data: []const u8,
    pos: usize,
    done: bool,
    malformed: bool,

    pub static fn init(data: []const u8): Smb2ChainIter {
        return Smb2ChainIter{
            .data = data,
            .pos = 0,
            .done = false,
            .malformed = false,
        };
    }

    fn next(*_): Smb2Packet? {
        if (_.done || _.pos >= _.data.len) {
            return null;
        }
        const slice = _.data[_.pos..];
        if (pkt := parseSmb2(slice)) {
            if (pkt.header.next_command == 0) {
                _.pos = _.data.len;
                _.done = true;
            } else {
                const step = pkt.header.next_command as usize;
                if (step >= slice.len) {
                    _.malformed = true;
                    _.done = true;
                    return null;
                }
                _.pos += step;
            }
            return pkt;
        } else {
            _.malformed = true;
            _.done = true;
            return null;
        }
    }

    fn isMalformed(): bool {
        return _.malformed;
    }
}

/// Creates a zero-copy iterator over one or more compound/chained SMB2 messages.
pub fn iterSmb2Chain(data: []const u8): Smb2ChainIter {
    return Smb2ChainIter.init(data);
}

/// MS-SMB2 §2.2.3 `SMB2 NEGOTIATE Request`.
pub struct Smb2NegotiateReq {
    dialect_count: u16,
    security_mode: u16,
    capabilities: u32,
    client_guid: [16]u8,
    dialects_bytes: []const u8,

    fn dialectAt(idx: usize): u16? {
        if (idx >= (_.dialect_count as usize)) {
            return null;
        }
        const off = idx * 2;
        if (off + 2 > _.dialects_bytes.len) {
            return null;
        }
        return readU16Le(_.dialects_bytes, off);
    }
}

/// Parses an `SMB2 NEGOTIATE Request` body from a parsed `Smb2Packet`.
pub fn parseSmb2NegotiateReq(pkt: Smb2Packet): Smb2NegotiateReq? {
    if (pkt.header.command != CMD_NEGOTIATE || pkt.header.isResponse()) {
        return null;
    }
    const b = pkt.body;
    if (b.len < 36) {
        return null;
    }
    const structure_size = readU16Le(b, 0);
    if (structure_size != 36) {
        return null;
    }
    const dialect_count = readU16Le(b, 2);
    if (dialect_count == 0) {
        return null;
    }
    const dialects_len = (dialect_count as usize) * 2;
    if (dialects_len > b.len - 36) {
        return null;
    }
    return Smb2NegotiateReq{
        .dialect_count = dialect_count,
        .security_mode = readU16Le(b, 4),
        .capabilities = readU32Le(b, 8),
        .client_guid = readBytes16(b, 12),
        .dialects_bytes = b[36..(36 + dialects_len)],
    };
}

/// MS-SMB2 §2.2.4 `SMB2 NEGOTIATE Response`.
pub struct Smb2NegotiateResp {
    security_mode: u16,
    dialect_revision: u16,
    server_guid: [16]u8,
    capabilities: u32,
    max_transact_size: u32,
    max_read_size: u32,
    max_write_size: u32,
    security_buffer: []const u8,
}

/// Parses an `SMB2 NEGOTIATE Response` body and extracts its zero-copy SPNEGO security buffer.
pub fn parseSmb2NegotiateResp(pkt: Smb2Packet): Smb2NegotiateResp? {
    if (pkt.header.command != CMD_NEGOTIATE || !pkt.header.isResponse()) {
        return null;
    }
    const b = pkt.body;
    if (b.len < 64) {
        return null;
    }
    const structure_size = readU16Le(b, 0);
    if (structure_size != 65) {
        return null;
    }
    const sec_off = readU16Le(b, 56) as usize;
    const sec_len = readU16Le(b, 58) as usize;

    let sec_buf: []const u8 = b[0..0];
    if (sec_len > 0) {
        if (sec_off < SMB2_HEADER_SIZE) {
            return null;
        }
        const rel_off = sec_off - SMB2_HEADER_SIZE;
        if (rel_off > b.len || sec_len > b.len - rel_off) {
            return null;
        }
        sec_buf = b[rel_off..(rel_off + sec_len)];
    }

    return Smb2NegotiateResp{
        .security_mode = readU16Le(b, 2),
        .dialect_revision = readU16Le(b, 4),
        .server_guid = readBytes16(b, 8),
        .capabilities = readU32Le(b, 24),
        .max_transact_size = readU32Le(b, 28),
        .max_read_size = readU32Le(b, 32),
        .max_write_size = readU32Le(b, 36),
        .security_buffer = sec_buf,
    };
}

/// MS-SMB2 §2.2.41 52-byte `SMB2 TRANSFORM_HEADER` for encrypted SMB3 messages.
pub struct Smb3TransformHeader {
    signature: [16]u8,
    nonce: [16]u8,
    orig_msg_size: u32,
    flags: u16,
    session_id: u64,
    encrypted_data: []const u8,
}

/// Parses an SMB3 `TRANSFORM_HEADER` (`0xFD, 'S', 'M', 'B'`) and slices its encrypted payload.
pub fn parseSmb3Transform(data: []const u8): Smb3TransformHeader? {
    if (data.len < SMB3_TRANSFORM_HEADER_SIZE || !isSmb3Transform(data)) {
        return null;
    }
    const orig_msg_size = readU32Le(data, 36);
    if (orig_msg_size == 0 || (orig_msg_size as usize) > data.len - SMB3_TRANSFORM_HEADER_SIZE) {
        return null;
    }
    return Smb3TransformHeader{
        .signature = readBytes16(data, 4),
        .nonce = readBytes16(data, 20),
        .orig_msg_size = orig_msg_size,
        .flags = readU16Le(data, 42),
        .session_id = readU64Le(data, 44),
        .encrypted_data = data[SMB3_TRANSFORM_HEADER_SIZE..],
    };
}
