// Origin: Pure-CP Stdlib (Zero-Alloc MS-NLMP NTLM v1/v2 Wire Protocol Parser) | Created: 2026-11-22

// MS-NLMP Signature & Message Types
pub const NTLM_SIGNATURE: []const u8 = "NTLMSSP\x00";
pub const NTLM_MSG_NEGOTIATE: u32 = 1;
pub const NTLM_MSG_CHALLENGE: u32 = 2;
pub const NTLM_MSG_AUTHENTICATE: u32 = 3;

// MS-NLMP Negotiate Flags (Section 2.2.2.5)
pub const NTLMSSP_NEGOTIATE_UNICODE: u32 = 0x00000001;
pub const NTLMSSP_NEGOTIATE_OEM: u32 = 0x00000002;
pub const NTLMSSP_REQUEST_TARGET: u32 = 0x00000004;
pub const NTLMSSP_NEGOTIATE_SIGN: u32 = 0x00000010;
pub const NTLMSSP_NEGOTIATE_SEAL: u32 = 0x00000020;
pub const NTLMSSP_NEGOTIATE_NTLM: u32 = 0x00000200;
pub const NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY: u32 = 0x00080000;
pub const NTLMSSP_NEGOTIATE_TARGET_INFO: u32 = 0x00800000;
pub const NTLMSSP_NEGOTIATE_VERSION: u32 = 0x02000000;
pub const NTLMSSP_NEGOTIATE_128: u32 = 0x20000000;
pub const NTLMSSP_NEGOTIATE_56: u32 = 0x80000000;

// MS-NLMP AV_PAIR IDs (Section 2.2.2.1)
pub const MsvAvEOL: u16 = 0;
pub const MsvAvNbComputerName: u16 = 1;
pub const MsvAvNbDomainName: u16 = 2;
pub const MsvAvDnsComputerName: u16 = 3;
pub const MsvAvDnsDomainName: u16 = 4;
pub const MsvAvDnsTreeName: u16 = 5;
pub const MsvAvFlags: u16 = 6;
pub const MsvAvTimestamp: u16 = 7;
pub const MsvAvSingleHost: u16 = 8;
pub const MsvAvTargetName: u16 = 9;
pub const MsvAvChannelBindings: u16 = 10;

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

fn hasNtlmSignature(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    return data[0] == 'N'
        && data[1] == 'T'
        && data[2] == 'L'
        && data[3] == 'M'
        && data[4] == 'S'
        && data[5] == 'S'
        && data[6] == 'P'
        && data[7] == 0;
}

/// Reads an 8-byte MS-NLMP Security Buffer descriptor (`Len: u16`, `MaxLen: u16`, `Offset: u32`)
/// and returns the zero-copy slice inside `data`, strictly checking bounds.
fn readSecBuf(data: []const u8, desc_off: usize): ([]const u8)? {
    if (desc_off + 8 > data.len) {
        return null;
    }
    const sec_len = readU16Le(data, desc_off) as usize;
    const max_len = readU16Le(data, desc_off + 2) as usize;
    const buf_off = readU32Le(data, desc_off + 4) as usize;

    if (sec_len == 0) {
        return data[0..0];
    }
    if (max_len != 0 && sec_len > max_len) {
        return null;
    }
    if (buf_off > data.len || sec_len > data.len - buf_off) {
        return null;
    }
    return data[buf_off..(buf_off + sec_len)];
}

/// MS-NLMP §2.2.2.10 8-byte `VERSION` structure.
pub struct NtlmVersion {
    major: u8,
    minor: u8,
    build: u16,
    ntlm_revision: u8,
}

fn parseVersionAt(data: []const u8, off: usize): NtlmVersion? {
    if (off + 8 > data.len) {
        return null;
    }
    return NtlmVersion{
        .major = data[off],
        .minor = data[off + 1],
        .build = readU16Le(data, off + 2),
        .ntlm_revision = data[off + 7],
    };
}

/// Peeks the MS-NLMP message type (`1`=Negotiate, `2`=Challenge, `3`=Authenticate)
/// after verifying the 8-byte `"NTLMSSP\0"` signature.
pub fn peekNtlmType(data: []const u8): u32? {
    if (data.len < 12 || !hasNtlmSignature(data)) {
        return null;
    }
    const msg_type = readU32Le(data, 8);
    if (msg_type == NTLM_MSG_NEGOTIATE || msg_type == NTLM_MSG_CHALLENGE || msg_type == NTLM_MSG_AUTHENTICATE) {
        return msg_type;
    }
    return null;
}

/// Returns `true` if `data` starts with a valid `"NTLMSSP\0"` signature and known message type (1..3).
pub fn isNtlm(data: []const u8): bool {
    return peekNtlmType(data) != null;
}

/// MS-NLMP §2.2.2.1 `AV_PAIR` attribute-value entry (`AvId: u16`, `AvLen: u16`, `Value`).
pub struct NtlmAvPair {
    av_id: u16,
    value: []const u8,
}

/// Zero-copy iterator over an MS-NLMP `TargetInfo` AV_PAIR TLV sequence.
/// Stops cleanly on `MsvAvEOL (0)` or marks `isTruncated()` if a TLV header/payload is truncated.
pub struct NtlmAvPairIter {
    data: []const u8,
    pos: usize,
    done: bool,
    saw_eol: bool,
    truncated: bool,

    pub static fn init(data: []const u8): NtlmAvPairIter {
        return NtlmAvPairIter{
            .data = data,
            .pos = 0,
            .done = false,
            .saw_eol = false,
            .truncated = false,
        };
    }

    fn next(*_): NtlmAvPair? {
        if (_.done || _.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 4) {
            _.truncated = true;
            _.done = true;
            return null;
        }
        const av_id = readU16Le(_.data, _.pos);
        const av_len = readU16Le(_.data, _.pos + 2) as usize;
        const val_start = _.pos + 4;
        if (av_len > _.data.len - val_start) {
            _.truncated = true;
            _.done = true;
            return null;
        }
        _.pos = val_start + av_len;
        if (av_id == MsvAvEOL) {
            _.saw_eol = true;
            _.done = true;
            return null;
        }
        return NtlmAvPair{
            .av_id = av_id,
            .value = _.data[val_start..(val_start + av_len)],
        };
    }

    fn isTruncated(): bool {
        return _.truncated;
    }

    fn sawEol(): bool {
        return _.saw_eol;
    }
}

/// MS-NLMP §2.2.1.1 Type 1 `NEGOTIATE_MESSAGE`.
pub struct NtlmNegotiate {
    flags: u32,
    domain: []const u8,
    workstation: []const u8,
    version: NtlmVersion?,
}

/// Parses an MS-NLMP Type 1 `NEGOTIATE_MESSAGE`.
pub fn parseNtlmNegotiate(data: []const u8): NtlmNegotiate? {
    const msg_type = peekNtlmType(data) ?? return null;
    if (msg_type != NTLM_MSG_NEGOTIATE || data.len < 16) {
        return null;
    }
    const flags = readU32Le(data, 12);

    let domain: []const u8 = data[0..0];
    let workstation: []const u8 = data[0..0];
    if (data.len >= 32) {
        domain = readSecBuf(data, 16) ?? return null;
        workstation = readSecBuf(data, 24) ?? return null;
    } else if (data.len != 16) {
        return null;
    }

    let version: NtlmVersion? = null;
    if ((flags & NTLMSSP_NEGOTIATE_VERSION) != 0) {
        if (data.len < 40) {
            return null;
        }
        version = parseVersionAt(data, 32);
    }

    return NtlmNegotiate{
        .flags = flags,
        .domain = domain,
        .workstation = workstation,
        .version = version,
    };
}

/// MS-NLMP §2.2.1.2 Type 2 `CHALLENGE_MESSAGE`.
pub struct NtlmChallenge {
    flags: u32,
    server_challenge: [8]u8,
    target_name: []const u8,
    target_info: []const u8,
    version: NtlmVersion?,

    fn avPairs(): NtlmAvPairIter {
        return NtlmAvPairIter.init(_.target_info);
    }

    fn findAvPair(id: u16): ([]const u8)? {
        let it = _.avPairs();
        while (true) {
            if (pair := it.next()) {
                if (pair.av_id == id) {
                    return pair.value;
                }
            } else {
                break;
            }
        }
        return null;
    }
}

/// Parses an MS-NLMP Type 2 `CHALLENGE_MESSAGE`.
pub fn parseNtlmChallenge(data: []const u8): NtlmChallenge? {
    const msg_type = peekNtlmType(data) ?? return null;
    if (msg_type != NTLM_MSG_CHALLENGE || data.len < 40) {
        return null;
    }
    const target_name = readSecBuf(data, 12) ?? return null;
    const flags = readU32Le(data, 20);
    const server_challenge: [8]u8 = [
        data[24], data[25], data[26], data[27],
        data[28], data[29], data[30], data[31],
    ];

    let target_info: []const u8 = data[0..0];
    if (data.len >= 48) {
        target_info = readSecBuf(data, 40) ?? return null;
    } else if ((flags & NTLMSSP_NEGOTIATE_TARGET_INFO) != 0) {
        return null;
    }

    let version: NtlmVersion? = null;
    if ((flags & NTLMSSP_NEGOTIATE_VERSION) != 0) {
        if (data.len < 56) {
            return null;
        }
        version = parseVersionAt(data, 48);
    }

    return NtlmChallenge{
        .flags = flags,
        .server_challenge = server_challenge,
        .target_name = target_name,
        .target_info = target_info,
        .version = version,
    };
}

/// MS-NLMP §2.2.1.3 Type 3 `AUTHENTICATE_MESSAGE`.
pub struct NtlmAuthenticate {
    flags: u32,
    lm_response: []const u8,
    nt_response: []const u8,
    domain: []const u8,
    user: []const u8,
    workstation: []const u8,
    session_key: []const u8,
    version: NtlmVersion?,

    /// Returns `true` when `nt_response` exceeds the 24-byte NTLMv1 fixed length (NTLMv2 response).
    fn isNtlmV2(): bool {
        return _.nt_response.len > 24;
    }
}

/// Parses an MS-NLMP Type 3 `AUTHENTICATE_MESSAGE` with all 6 Security Buffers.
pub fn parseNtlmAuthenticate(data: []const u8): NtlmAuthenticate? {
    const msg_type = peekNtlmType(data) ?? return null;
    if (msg_type != NTLM_MSG_AUTHENTICATE || data.len < 64) {
        return null;
    }
    const lm_response = readSecBuf(data, 12) ?? return null;
    const nt_response = readSecBuf(data, 20) ?? return null;
    const domain = readSecBuf(data, 28) ?? return null;
    const user = readSecBuf(data, 36) ?? return null;
    const workstation = readSecBuf(data, 44) ?? return null;
    const session_key = readSecBuf(data, 52) ?? return null;
    const flags = readU32Le(data, 60);

    let version: NtlmVersion? = null;
    if ((flags & NTLMSSP_NEGOTIATE_VERSION) != 0) {
        if (data.len < 72) {
            return null;
        }
        version = parseVersionAt(data, 64);
    }

    return NtlmAuthenticate{
        .flags = flags,
        .lm_response = lm_response,
        .nt_response = nt_response,
        .domain = domain,
        .user = user,
        .workstation = workstation,
        .session_key = session_key,
        .version = version,
    };
}
