// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8907 TACACS+ AAA Wire Protocol Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 8907 TACACS+ AAA wire protocol parser:
// 12-byte big-endian header, Authentication START/REPLY, Authorization REQUEST/RESPONSE,
// Accounting REQUEST/REPLY, and zero-allocation length-prefixed TacacsArgIter.

/// Standard TACACS+ TCP port (`49`, RFC 8907).
pub const TACACS_TCP_PORT: u16 = 49;

/// TACACS+ fixed header size in bytes (`12`, RFC 8907 Section 4.1).
pub const TACACS_HEADER_SIZE: usize = 12;

/// TACACS+ major version (`0xC`).
pub const TACACS_MAJOR_VER: u8 = 0xC;

/// TACACS+ default minor version (`0`).
pub const TACACS_MINOR_VER_DEFAULT: u8 = 0;

/// TACACS+ minor version one (`1`).
pub const TACACS_MINOR_VER_ONE: u8 = 1;

// TACACS+ Header Flags (RFC 8907 Section 4.1)
pub const TAC_PLUS_UNENCRYPTED_FLAG: u8 = 0x01;
pub const TAC_PLUS_SINGLE_CONNECT_FLAG: u8 = 0x04;

/// TACACS+ packet types (RFC 8907 Section 4.1).
pub enum TacacsType: u8 {
    authen = 1,
    author = 2,
    acct = 3,
}

/// TACACS+ Authentication REPLY status codes (RFC 8907 Section 5.2).
pub enum TacacsAuthenStatus: u8 {
    unknown = 0,
    pass = 1,
    fail = 2,
    getdata = 3,
    getuser = 4,
    getpass = 5,
    restart = 6,
    err = 7,
    follow = 0x21,
}

/// TACACS+ Authorization RESPONSE status codes (RFC 8907 Section 6.2).
pub enum TacacsAuthorStatus: u8 {
    unknown = 0,
    pass_add = 1,
    pass_repl = 2,
    fail = 0x10,
    err = 0x11,
    follow = 0x21,
}

/// TACACS+ Accounting REPLY status codes (RFC 8907 Section 7.2).
pub enum TacacsAcctStatus: u8 {
    unknown = 0,
    success = 1,
    err = 2,
    follow = 0x21,
}

/// Decoded 12-byte RFC 8907 TACACS+ header.
pub struct TacacsHeader {
    major_version: u8 = TACACS_MAJOR_VER,
    minor_version: u8 = 0,
    pkt_type: TacacsType = TacacsType.authen,
    seq_no: u8 = 1,
    flags: u8 = 0,
    session_id: u32 = 0,
    length: u32 = 0,

    fn isUnencrypted(): bool {
        return (_.flags & TAC_PLUS_UNENCRYPTED_FLAG) != 0;
    }

    fn isSingleConnect(): bool {
        return (_.flags & TAC_PLUS_SINGLE_CONNECT_FLAG) != 0;
    }
}

/// Zero-copy decoded TACACS+ packet (12B header + body slice).
pub struct TacacsPacket {
    header: TacacsHeader = TacacsHeader {},
    body: []const u8 = "",
}

/// Zero-allocation iterator over TACACS+ length-table-prefixed arguments.
pub struct TacacsArgIter {
    lens: []const u8 = "",
    data: []const u8 = "",
    idx: usize = 0,
    pos: usize = 0,
    failed: bool = false,

    pub fn init(lens: []const u8, data: []const u8): TacacsArgIter {
        return TacacsArgIter {
            lens: lens,
            data: data,
            idx: 0,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): ([]const u8)? {
        if (_.failed || _.idx >= _.lens.len) {
            return null;
        }
        const alen = _.lens[_.idx] as usize;
        if (_.data.len - _.pos < alen) {
            _.failed = true;
            return null;
        }
        const slice = _.data[_.pos..(_.pos + alen)];
        _.pos += alen;
        _.idx += 1;
        return slice;
    }
}

/// Zero-copy decoded RFC 8907 Authentication START body.
pub struct TacacsAuthenStart {
    action: u8 = 0,
    priv_lvl: u8 = 0,
    authen_type: u8 = 0,
    authen_service: u8 = 0,
    user: []const u8 = "",
    port: []const u8 = "",
    rem_addr: []const u8 = "",
    data: []const u8 = "",
}

/// Zero-copy decoded RFC 8907 Authentication REPLY body.
pub struct TacacsAuthenReply {
    status: TacacsAuthenStatus = TacacsAuthenStatus.unknown,
    raw_status: u8 = 0,
    flags: u8 = 0,
    server_msg: []const u8 = "",
    data: []const u8 = "",
}

/// Zero-copy decoded RFC 8907 Authorization REQUEST body.
pub struct TacacsAuthorRequest {
    authen_method: u8 = 0,
    priv_lvl: u8 = 0,
    authen_type: u8 = 0,
    authen_service: u8 = 0,
    arg_cnt: u8 = 0,
    user: []const u8 = "",
    port: []const u8 = "",
    rem_addr: []const u8 = "",
    arg_lens: []const u8 = "",
    args_data: []const u8 = "",

    fn args(): TacacsArgIter {
        return TacacsArgIter.init(_.arg_lens, _.args_data);
    }
}

/// Zero-copy decoded RFC 8907 Authorization RESPONSE body.
pub struct TacacsAuthorResponse {
    status: TacacsAuthorStatus = TacacsAuthorStatus.unknown,
    raw_status: u8 = 0,
    arg_cnt: u8 = 0,
    server_msg: []const u8 = "",
    data: []const u8 = "",
    arg_lens: []const u8 = "",
    args_data: []const u8 = "",

    fn args(): TacacsArgIter {
        return TacacsArgIter.init(_.arg_lens, _.args_data);
    }
}

/// Zero-copy decoded RFC 8907 Accounting REQUEST body.
pub struct TacacsAcctRequest {
    flags: u8 = 0,
    authen_method: u8 = 0,
    priv_lvl: u8 = 0,
    authen_type: u8 = 0,
    authen_service: u8 = 0,
    arg_cnt: u8 = 0,
    user: []const u8 = "",
    port: []const u8 = "",
    rem_addr: []const u8 = "",
    arg_lens: []const u8 = "",
    args_data: []const u8 = "",

    fn args(): TacacsArgIter {
        return TacacsArgIter.init(_.arg_lens, _.args_data);
    }
}

/// Zero-copy decoded RFC 8907 Accounting REPLY body.
pub struct TacacsAcctReply {
    status: TacacsAcctStatus = TacacsAcctStatus.unknown,
    raw_status: u8 = 0,
    server_msg: []const u8 = "",
    data: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn sumArgLens(lens: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < lens.len) {
        total += lens[i] as usize;
        i += 1;
    }
    return total;
}

fn classifyAuthenStatus(raw: u8): TacacsAuthenStatus {
    if (raw == 1) { return TacacsAuthenStatus.pass; }
    if (raw == 2) { return TacacsAuthenStatus.fail; }
    if (raw == 3) { return TacacsAuthenStatus.getdata; }
    if (raw == 4) { return TacacsAuthenStatus.getuser; }
    if (raw == 5) { return TacacsAuthenStatus.getpass; }
    if (raw == 6) { return TacacsAuthenStatus.restart; }
    if (raw == 7) { return TacacsAuthenStatus.err; }
    if (raw == 0x21) { return TacacsAuthenStatus.follow; }
    return TacacsAuthenStatus.unknown;
}

fn classifyAuthorStatus(raw: u8): TacacsAuthorStatus {
    if (raw == 1) { return TacacsAuthorStatus.pass_add; }
    if (raw == 2) { return TacacsAuthorStatus.pass_repl; }
    if (raw == 0x10) { return TacacsAuthorStatus.fail; }
    if (raw == 0x11) { return TacacsAuthorStatus.err; }
    if (raw == 0x21) { return TacacsAuthorStatus.follow; }
    return TacacsAuthorStatus.unknown;
}

fn classifyAcctStatus(raw: u8): TacacsAcctStatus {
    if (raw == 1) { return TacacsAcctStatus.success; }
    if (raw == 2) { return TacacsAcctStatus.err; }
    if (raw == 0x21) { return TacacsAcctStatus.follow; }
    return TacacsAcctStatus.unknown;
}

// ---------------------------------------------------------------------------
// Public Header & Body Parsing API
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 8907 TACACS+ packet (12B header + body slice).
/// Validates `major_version == 0xC`, `pkt_type in 1..3`, `seq_no >= 1`, and `12 + length <= data.len`.
pub fn parseTacacs(data: []const u8): TacacsPacket? {
    if (data.len < TACACS_HEADER_SIZE) {
        return null;
    }
    const ver = data[0];
    const major_version: u8 = ver >> 4;
    const minor_version: u8 = ver & 0x0F;
    if (major_version != TACACS_MAJOR_VER) {
        return null;
    }
    const raw_type = data[1];
    let pkt_type = TacacsType.authen;
    if (raw_type == 1) {
        pkt_type = TacacsType.authen;
    } else if (raw_type == 2) {
        pkt_type = TacacsType.author;
    } else if (raw_type == 3) {
        pkt_type = TacacsType.acct;
    } else {
        return null;
    }
    const seq_no = data[2];
    if (seq_no == 0) {
        return null;
    }
    const flags = data[3];
    const session_id = readBeU32(data, 4);
    const length = readBeU32(data, 8);
    const len_usize = length as usize;
    if (len_usize > data.len - TACACS_HEADER_SIZE) {
        return null;
    }

    return TacacsPacket {
        header: TacacsHeader {
            major_version: major_version,
            minor_version: minor_version,
            pkt_type: pkt_type,
            seq_no: seq_no,
            flags: flags,
            session_id: session_id,
            length: length,
        },
        body: data[TACACS_HEADER_SIZE..(TACACS_HEADER_SIZE + len_usize)],
    };
}

/// Returns `true` if `data` starts with a structurally valid RFC 8907 TACACS+ packet.
pub fn isTacacs(data: []const u8): bool {
    if (pkt := parseTacacs(data)) {
        * = pkt;
        return true;
    }
    return false;
}

/// Parses a plaintext/decrypted RFC 8907 Authentication START body.
pub fn parseAuthenStart(body: []const u8): TacacsAuthenStart? {
    if (body.len < 8) {
        return null;
    }
    const ulen = body[4] as usize;
    const plen = body[5] as usize;
    const rlen = body[6] as usize;
    const dlen = body[7] as usize;
    if (body.len - 8 < ulen + plen + rlen + dlen) {
        return null;
    }
    const p1 = 8 + ulen;
    const p2 = p1 + plen;
    const p3 = p2 + rlen;
    const p4 = p3 + dlen;
    return TacacsAuthenStart {
        action: body[0],
        priv_lvl: body[1],
        authen_type: body[2],
        authen_service: body[3],
        user: body[8..p1],
        port: body[p1..p2],
        rem_addr: body[p2..p3],
        data: body[p3..p4],
    };
}

/// Parses a plaintext/decrypted RFC 8907 Authentication REPLY body.
pub fn parseAuthenReply(body: []const u8): TacacsAuthenReply? {
    if (body.len < 6) {
        return null;
    }
    const raw_status = body[0];
    const flags = body[1];
    const mlen = readBeU16(body, 2) as usize;
    const dlen = readBeU16(body, 4) as usize;
    if (body.len - 6 < mlen + dlen) {
        return null;
    }
    const p1 = 6 + mlen;
    const p2 = p1 + dlen;
    return TacacsAuthenReply {
        status: classifyAuthenStatus(raw_status),
        raw_status: raw_status,
        flags: flags,
        server_msg: body[6..p1],
        data: body[p1..p2],
    };
}

/// Parses a plaintext/decrypted RFC 8907 Authorization REQUEST body.
pub fn parseAuthorRequest(body: []const u8): TacacsAuthorRequest? {
    if (body.len < 8) {
        return null;
    }
    const ulen = body[4] as usize;
    const plen = body[5] as usize;
    const rlen = body[6] as usize;
    const arg_cnt = body[7];
    const acnt = arg_cnt as usize;
    if (body.len - 8 < acnt) {
        return null;
    }
    const arg_lens = body[8..(8 + acnt)];
    const args_total = sumArgLens(arg_lens);
    const var_start = 8 + acnt;
    if (body.len - var_start < ulen + plen + rlen + args_total) {
        return null;
    }
    const p1 = var_start + ulen;
    const p2 = p1 + plen;
    const p3 = p2 + rlen;
    const p4 = p3 + args_total;
    return TacacsAuthorRequest {
        authen_method: body[0],
        priv_lvl: body[1],
        authen_type: body[2],
        authen_service: body[3],
        arg_cnt: arg_cnt,
        user: body[var_start..p1],
        port: body[p1..p2],
        rem_addr: body[p2..p3],
        arg_lens: arg_lens,
        args_data: body[p3..p4],
    };
}

/// Parses a plaintext/decrypted RFC 8907 Authorization RESPONSE body.
pub fn parseAuthorResponse(body: []const u8): TacacsAuthorResponse? {
    if (body.len < 6) {
        return null;
    }
    const raw_status = body[0];
    const arg_cnt = body[1];
    const acnt = arg_cnt as usize;
    const mlen = readBeU16(body, 2) as usize;
    const dlen = readBeU16(body, 4) as usize;
    if (body.len - 6 < acnt) {
        return null;
    }
    const arg_lens = body[6..(6 + acnt)];
    const args_total = sumArgLens(arg_lens);
    const var_start = 6 + acnt;
    if (body.len - var_start < mlen + dlen + args_total) {
        return null;
    }
    const p1 = var_start + mlen;
    const p2 = p1 + dlen;
    const p3 = p2 + args_total;
    return TacacsAuthorResponse {
        status: classifyAuthorStatus(raw_status),
        raw_status: raw_status,
        arg_cnt: arg_cnt,
        server_msg: body[var_start..p1],
        data: body[p1..p2],
        arg_lens: arg_lens,
        args_data: body[p2..p3],
    };
}

/// Parses a plaintext/decrypted RFC 8907 Accounting REQUEST body.
pub fn parseAcctRequest(body: []const u8): TacacsAcctRequest? {
    if (body.len < 9) {
        return null;
    }
    const ulen = body[5] as usize;
    const plen = body[6] as usize;
    const rlen = body[7] as usize;
    const arg_cnt = body[8];
    const acnt = arg_cnt as usize;
    if (body.len - 9 < acnt) {
        return null;
    }
    const arg_lens = body[9..(9 + acnt)];
    const args_total = sumArgLens(arg_lens);
    const var_start = 9 + acnt;
    if (body.len - var_start < ulen + plen + rlen + args_total) {
        return null;
    }
    const p1 = var_start + ulen;
    const p2 = p1 + plen;
    const p3 = p2 + rlen;
    const p4 = p3 + args_total;
    return TacacsAcctRequest {
        flags: body[0],
        authen_method: body[1],
        priv_lvl: body[2],
        authen_type: body[3],
        authen_service: body[4],
        arg_cnt: arg_cnt,
        user: body[var_start..p1],
        port: body[p1..p2],
        rem_addr: body[p2..p3],
        arg_lens: arg_lens,
        args_data: body[p3..p4],
    };
}

/// Parses a plaintext/decrypted RFC 8907 Accounting REPLY body.
pub fn parseAcctReply(body: []const u8): TacacsAcctReply? {
    if (body.len < 5) {
        return null;
    }
    const mlen = readBeU16(body, 0) as usize;
    const dlen = readBeU16(body, 2) as usize;
    const raw_status = body[4];
    if (body.len - 5 < mlen + dlen) {
        return null;
    }
    const p1 = 5 + mlen;
    const p2 = p1 + dlen;
    return TacacsAcctReply {
        status: classifyAcctStatus(raw_status),
        raw_status: raw_status,
        server_msg: body[5..p1],
        data: body[p1..p2],
    };
}
