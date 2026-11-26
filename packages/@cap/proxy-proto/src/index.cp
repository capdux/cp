// @cap/proxy-proto — Zero-heap HAProxy PROXY Protocol v1 & v2 codec
// MIT License (clean-room CP implementation)

pub const PP_OK: i32 = 0;
pub const PP_INCOMPLETE: i32 = 1;
pub const PP_INVALID: i32 = -1;

pub const PROTO_UNKNOWN: u8 = 0;
pub const PROTO_TCP4: u8 = 1;
pub const PROTO_TCP6: u8 = 2;

pub const V1_MAX_LEN: usize = 107;
pub const V2_HEADER_LEN: usize = 16;

pub const V2_FAM_UNSPEC: u8 = 0;
pub const V2_FAM_INET: u8 = 1;
pub const V2_FAM_INET6: u8 = 2;
pub const V2_FAM_UNIX: u8 = 3;

pub const V2_TRANS_UNSPEC: u8 = 0;
pub const V2_TRANS_STREAM: u8 = 1;
pub const V2_TRANS_DGRAM: u8 = 2;

const V2_SIG: [12]u8 = [
    0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,
];

pub struct ProxyV1Header {
    pub proto: u8,
    pub src_ip: string,
    pub dst_ip: string,
    pub src_port: u16,
    pub dst_port: u16,
    pub consumed: usize,
}

pub struct ProxyV2Header {
    pub is_proxy: bool,
    pub family: u8,
    pub transport: u8,
    pub payload_len: u16,
    pub src_ipv4: [4]u8,
    pub dst_ipv4: [4]u8,
    pub src_port: u16,
    pub dst_port: u16,
    pub consumed: usize,
}

fn bytesEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isV1Prefix(data: []const u8) -> bool {
    if (data.len < 6) {
        return false;
    }
    return bytesEq(data[0..6], "PROXY ");
}

pub fn isV2Signature(data: []const u8) -> bool {
    if (data.len < 12) {
        return false;
    }
    let i: usize = 0;
    while (i < 12) {
        if (data[i] != V2_SIG[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parsePortStrict(s: string) -> i32 {
    if (s.len == 0 || s.len > 5) {
        return -1;
    }
    if (s.len > 1 && s[0] == '0' as u8) {
        return -1;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return -1;
        }
        val = val * 10 + ((c - ('0' as u8)) as u32);
        if (val > 65535) {
            return -1;
        }
        i += 1;
    }
    return val as i32;
}

pub fn parseV1(data: string, out: *ProxyV1Header) -> i32 {
    const prefix = "PROXY ";
    if (data.len < 6) {
        let i: usize = 0;
        while (i < data.len) {
            if (data[i] != prefix[i]) {
                return PP_INVALID;
            }
            i += 1;
        }
        return PP_INCOMPLETE;
    }
    if (!isV1Prefix(data)) {
        return PP_INVALID;
    }

    let search_limit: usize = data.len;
    if (search_limit > V1_MAX_LEN) {
        search_limit = V1_MAX_LEN;
    }
    let crlf_found: bool = false;
    let crlf: usize = 0;
    let i: usize = 6;
    while (i + 1 < search_limit) {
        if (data[i] == '\r' as u8 && data[i + 1] == '\n' as u8) {
            crlf = i;
            crlf_found = true;
            break;
        }
        if (data[i] == '\r' as u8 || data[i] == '\n' as u8) {
            return PP_INVALID;
        }
        i += 1;
    }
    if (!crlf_found) {
        if (data.len >= V1_MAX_LEN) {
            return PP_INVALID;
        }
        return PP_INCOMPLETE;
    }

    const line_end: usize = crlf + 2;
    const body: string = data[6..crlf];

    // Read protocol token
    let p_end: usize = 0;
    while (p_end < body.len && body[p_end] != ' ' as u8) {
        p_end += 1;
    }
    const proto_tok: string = body[0..p_end];

    if (bytesEq(proto_tok, "UNKNOWN")) {
        out.* = ProxyV1Header {
            proto: PROTO_UNKNOWN,
            src_ip: "",
            dst_ip: "",
            src_port: 0,
            dst_port: 0,
            consumed: line_end,
        };
        return PP_OK;
    }

    let proto: u8 = PROTO_UNKNOWN;
    if (bytesEq(proto_tok, "TCP4")) {
        proto = PROTO_TCP4;
    } else if (bytesEq(proto_tok, "TCP6")) {
        proto = PROTO_TCP6;
    } else {
        return PP_INVALID;
    }

    if (p_end >= body.len || body[p_end] != ' ' as u8) {
        return PP_INVALID;
    }

    // Parse 4 single-space-separated fields: src_ip dst_ip src_port dst_port
    let pos: usize = p_end + 1;
    let s1: usize = pos;
    while (pos < body.len && body[pos] != ' ' as u8) {
        pos += 1;
    }
    if (pos == s1 || pos >= body.len) {
        return PP_INVALID;
    }
    const src_ip: string = body[s1..pos];
    pos += 1;

    let s2: usize = pos;
    while (pos < body.len && body[pos] != ' ' as u8) {
        pos += 1;
    }
    if (pos == s2 || pos >= body.len) {
        return PP_INVALID;
    }
    const dst_ip: string = body[s2..pos];
    pos += 1;

    let s3: usize = pos;
    while (pos < body.len && body[pos] != ' ' as u8) {
        pos += 1;
    }
    if (pos == s3 || pos >= body.len) {
        return PP_INVALID;
    }
    const sp_i: i32 = parsePortStrict(body[s3..pos]);
    if (sp_i < 0) {
        return PP_INVALID;
    }
    pos += 1;

    if (pos >= body.len) {
        return PP_INVALID;
    }
    const dp_i: i32 = parsePortStrict(body[pos..body.len]);
    if (dp_i < 0) {
        return PP_INVALID;
    }

    out.* = ProxyV1Header {
        proto: proto,
        src_ip: src_ip,
        dst_ip: dst_ip,
        src_port: sp_i as u16,
        dst_port: dp_i as u16,
        consumed: line_end,
    };
    return PP_OK;
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

fn appendU16(out: []u8, pos: usize, v: u16) -> i64 {
    let digits: [6]u8 = undefined;
    let count: usize = 0;
    if (v == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u16 = v;
        while (cur > 0) {
            digits[count] = ('0' as u8) + ((cur % 10) as u8);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > out.len - pos) {
        return -1;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p as i64;
}

pub fn encodeV1(
    proto: u8,
    src_ip: string,
    dst_ip: string,
    src_port: u16,
    dst_port: u16,
    out: []u8,
) -> usize {
    if (proto == PROTO_UNKNOWN) {
        const p: i64 = appendSlice(out, 0, "PROXY UNKNOWN\r\n");
        if (p < 0) {
            return 0;
        }
        return p as usize;
    }

    let hdr: string = "";
    if (proto == PROTO_TCP4) {
        hdr = "PROXY TCP4 ";
    } else if (proto == PROTO_TCP6) {
        hdr = "PROXY TCP6 ";
    } else {
        return 0;
    }

    if (src_ip.len == 0 || dst_ip.len == 0) {
        return 0;
    }

    let p: i64 = appendSlice(out, 0, hdr);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, src_ip);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, " ");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, dst_ip);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, " ");
    if (p < 0) {
        return 0;
    }
    p = appendU16(out, p as usize, src_port);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, " ");
    if (p < 0) {
        return 0;
    }
    p = appendU16(out, p as usize, dst_port);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, "\r\n");
    if (p < 0 || (p as usize) > V1_MAX_LEN) {
        return 0;
    }
    return p as usize;
}

pub fn parseV2(data: []const u8, out: *ProxyV2Header) -> i32 {
    if (data.len < 12) {
        let i: usize = 0;
        while (i < data.len) {
            if (data[i] != V2_SIG[i]) {
                return PP_INVALID;
            }
            i += 1;
        }
        return PP_INCOMPLETE;
    }
    if (!isV2Signature(data)) {
        return PP_INVALID;
    }
    if (data.len < V2_HEADER_LEN) {
        return PP_INCOMPLETE;
    }

    const ver_cmd: u8 = data[12];
    if ((ver_cmd >> 4) != 2) {
        return PP_INVALID;
    }
    const cmd: u8 = ver_cmd & 0x0F;
    if (cmd > 1) {
        return PP_INVALID;
    }
    const is_proxy: bool = cmd == 1;

    const fam_trans: u8 = data[13];
    const family: u8 = (fam_trans >> 4) & 0x0F;
    const transport: u8 = fam_trans & 0x0F;
    if (family > 3 || transport > 2) {
        return PP_INVALID;
    }

    const payload_len: u16 = (((data[14] as u16) << 8) | (data[15] as u16));
    const total: usize = V2_HEADER_LEN + (payload_len as usize);
    if (data.len < total) {
        return PP_INCOMPLETE;
    }

    let src_ipv4: [4]u8 = [0, 0, 0, 0];
    let dst_ipv4: [4]u8 = [0, 0, 0, 0];
    let src_port: u16 = 0;
    let dst_port: u16 = 0;

    if (is_proxy && family == V2_FAM_INET) {
        if (payload_len < 12) {
            return PP_INVALID;
        }
        src_ipv4[0] = data[16];
        src_ipv4[1] = data[17];
        src_ipv4[2] = data[18];
        src_ipv4[3] = data[19];
        dst_ipv4[0] = data[20];
        dst_ipv4[1] = data[21];
        dst_ipv4[2] = data[22];
        dst_ipv4[3] = data[23];
        src_port = (((data[24] as u16) << 8) | (data[25] as u16));
        dst_port = (((data[26] as u16) << 8) | (data[27] as u16));
    }

    out.* = ProxyV2Header {
        is_proxy: is_proxy,
        family: family,
        transport: transport,
        payload_len: payload_len,
        src_ipv4: src_ipv4,
        dst_ipv4: dst_ipv4,
        src_port: src_port,
        dst_port: dst_port,
        consumed: total,
    };
    return PP_OK;
}

pub fn encodeV2Ipv4(
    src_ip: [4]u8,
    dst_ip: [4]u8,
    src_port: u16,
    dst_port: u16,
    out: []u8,
) -> usize {
    if (out.len < 28) {
        return 0;
    }
    let i: usize = 0;
    while (i < 12) {
        out[i] = V2_SIG[i];
        i += 1;
    }
    out[12] = 0x21; // v2 + PROXY
    out[13] = 0x11; // AF_INET + STREAM
    out[14] = 0x00;
    out[15] = 0x0C; // 12 bytes
    out[16] = src_ip[0];
    out[17] = src_ip[1];
    out[18] = src_ip[2];
    out[19] = src_ip[3];
    out[20] = dst_ip[0];
    out[21] = dst_ip[1];
    out[22] = dst_ip[2];
    out[23] = dst_ip[3];
    out[24] = ((src_port >> 8) & 0xFF) as u8;
    out[25] = (src_port & 0xFF) as u8;
    out[26] = ((dst_port >> 8) & 0xFF) as u8;
    out[27] = (dst_port & 0xFF) as u8;
    return 28;
}
