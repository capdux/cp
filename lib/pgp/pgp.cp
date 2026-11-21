// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OpenPGP RFC 4880/9580 Armor & Packet Parser)

/// OpenPGP Radix-64 CRC-24 初始值与生成多项式（RFC 4880 §6.1）
pub const PGP_CRC24_INIT: u32 = 0xB704CE;
pub const PGP_CRC24_POLY: u32 = 0x1864CFB;

/// OpenPGP 核心 Packet Tag 常量（RFC 4880 / RFC 9580）
pub const TAG_PKESK: u8 = 1;
pub const TAG_SIGNATURE: u8 = 2;
pub const TAG_SKESK: u8 = 3;
pub const TAG_ONE_PASS_SIG: u8 = 4;
pub const TAG_SECRET_KEY: u8 = 5;
pub const TAG_PUBLIC_KEY: u8 = 6;
pub const TAG_SECRET_SUBKEY: u8 = 7;
pub const TAG_COMPRESSED_DATA: u8 = 8;
pub const TAG_SED: u8 = 9;
pub const TAG_MARKER: u8 = 10;
pub const TAG_LITERAL_DATA: u8 = 11;
pub const TAG_TRUST: u8 = 12;
pub const TAG_USER_ID: u8 = 13;
pub const TAG_PUBLIC_SUBKEY: u8 = 14;
pub const TAG_USER_ATTR: u8 = 17;
pub const TAG_SEIPD: u8 = 18;
pub const TAG_MDC: u8 = 19;

/// OpenPGP ASCII Armor 零拷贝结构视图
pub struct PgpArmorHeader {
    block_type: []const u8,
    headers_slice: []const u8,
    body_b64: []const u8,
    crc24_b64: []const u8,
}

/// OpenPGP 二进制 Packet 帧零拷贝视图
pub struct PgpPacket {
    tag: u8,
    is_new_format: bool,
    body: []const u8,
    total_bytes: usize,
}

/// OpenPGP v4 公钥包快捷解析结果
pub struct PgpPublicKeyInfo {
    version: u8,
    creation_time: u32,
    algorithm: u8,
    key_material: []const u8,
}

/// OpenPGP v4 签名包快捷解析结果
pub struct PgpSignatureInfo {
    version: u8,
    sig_type: u8,
    pub_alg: u8,
    hash_alg: u8,
    hashed_subpackets: []const u8,
    unhashed_subpackets: []const u8,
}

/// OpenPGP Literal Data（Tag 11）快捷解析结果
pub struct PgpLiteralData {
    format: u8,
    filename: []const u8,
    timestamp: u32,
    payload: []const u8,
}

/// OpenPGP 二进制 Old/New Format Packet 零分配流式迭代器
pub struct PgpPacketIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(data: []const u8): PgpPacketIter {
        return PgpPacketIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): PgpPacket? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const start = _.pos;
        const ptag: u8 = _.data[start];
        if ((ptag & 0x80) == 0) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const is_new: bool = (ptag & 0x40) != 0;
        let tag: u8 = 0;
        let body_start: usize = start + 1;
        let body_len_u64: u64 = 0;

        if (!is_new) {
            // Old Format: bits 5..2 = tag, bits 1..0 = length_type
            tag = (ptag >> 2) & 0x0F;
            if (tag == 0) {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
            const len_type: u8 = ptag & 0x03;
            if (len_type == 0) {
                if (_.data.len - body_start < 1) {
                    _.had_error = true;
                    _.pos = _.data.len;
                    return null;
                }
                body_len_u64 = _.data[body_start] as u64;
                body_start += 1;
            } else if (len_type == 1) {
                if (_.data.len - body_start < 2) {
                    _.had_error = true;
                    _.pos = _.data.len;
                    return null;
                }
                body_len_u64 = readU16Be(_.data, body_start) as u64;
                body_start += 2;
            } else if (len_type == 2) {
                if (_.data.len - body_start < 4) {
                    _.had_error = true;
                    _.pos = _.data.len;
                    return null;
                }
                body_len_u64 = readU32Be(_.data, body_start) as u64;
                body_start += 4;
            } else {
                // Indeterminate length: extends to EOF
                body_len_u64 = (_.data.len - body_start) as u64;
            }
        } else {
            // New Format: bits 5..0 = tag
            tag = ptag & 0x3F;
            if (tag == 0 || body_start >= _.data.len) {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
            const b1: u8 = _.data[body_start];
            body_start += 1;
            if (b1 < 192) {
                body_len_u64 = b1 as u64;
            } else if (b1 <= 223) {
                if (body_start >= _.data.len) {
                    _.had_error = true;
                    _.pos = _.data.len;
                    return null;
                }
                const b2: u8 = _.data[body_start];
                body_start += 1;
                body_len_u64 = ((((b1 - 192) as u64) << 8) + (b2 as u64)) + 192;
            } else if (b1 == 255) {
                if (_.data.len - body_start < 4) {
                    _.had_error = true;
                    _.pos = _.data.len;
                    return null;
                }
                body_len_u64 = readU32Be(_.data, body_start) as u64;
                body_start += 4;
            } else {
                // 224..254 Partial Body Length 不适用于单切片零拷贝视图
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
        }

        const rem_u64: u64 = (_.data.len - body_start) as u64;
        if (body_len_u64 > rem_u64) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const body_len: usize = body_len_u64 as usize;
        const end_pos: usize = body_start + body_len;
        _.pos = end_pos;

        return PgpPacket {
            tag: tag,
            is_new_format: is_new,
            body: _.data[body_start..end_pos],
            total_bytes: end_pos - start,
        };
    }
}

// ---------------------------------------------------------------------------
// 内部字节、行与 Base64 辅助函数
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    const base = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[base + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

struct LineView {
    line: []const u8,
    line_start: usize,
    next_pos: usize,
}

fn nextLine(text: []const u8, start: usize): LineView? {
    if (start >= text.len) return null;
    let i: usize = start;
    while (i < text.len && text[i] != '\n') {
        i += 1;
    }
    let end = i;
    if (end > start && text[end - 1] == '\r') {
        end -= 1;
    }
    const next_pos: usize = if (i < text.len) i + 1 else i;
    return LineView {
        line: text[start..end],
        line_start: start,
        next_pos: next_pos,
    };
}

fn b64Val(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

fn b64DecodeStreaming(src: []const u8, out: *[]u8): usize? {
    let quad: [4]u8 = [0; 4];
    let qlen: usize = 0;
    let out_pos: usize = 0;
    let finished: bool = false;

    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        i += 1;
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            continue;
        }
        if (finished) {
            return null;
        }
        quad[qlen] = c;
        qlen += 1;
        if (qlen == 4) {
            const v0 = b64Val(quad[0]) ?? return null;
            const v1 = b64Val(quad[1]) ?? return null;
            if (quad[2] == '=') {
                if (quad[3] != '=' || (v1 & 0x0F) != 0) return null;
                if (out_pos + 1 > out.*.len) return null;
                out.*[out_pos] = (v0 << 2) | (v1 >> 4);
                out_pos += 1;
                finished = true;
            } else if (quad[3] == '=') {
                const v2 = b64Val(quad[2]) ?? return null;
                if ((v2 & 0x03) != 0) return null;
                if (out_pos + 2 > out.*.len) return null;
                out.*[out_pos] = (v0 << 2) | (v1 >> 4);
                out.*[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
                out_pos += 2;
                finished = true;
            } else {
                const v2 = b64Val(quad[2]) ?? return null;
                const v3 = b64Val(quad[3]) ?? return null;
                if (out_pos + 3 > out.*.len) return null;
                out.*[out_pos] = (v0 << 2) | (v1 >> 4);
                out.*[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
                out.*[out_pos + 2] = ((v2 & 0x03) << 6) | v3;
                out_pos += 3;
            }
            qlen = 0;
        }
    }

    if (qlen != 0) {
        return null;
    }
    return out_pos;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 计算 OpenPGP Radix-64 CRC-24 校验和（24-bit 无符号值 `0..=0xFFFFFF`）
pub fn pgpCrc24(data: []const u8): u32 {
    let crc: u32 = PGP_CRC24_INIT;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ ((data[i] as u32) << 16);
        let bit: u8 = 0;
        while (bit < 8) {
            crc = crc << 1;
            if ((crc & 0x1000000) != 0) {
                crc = crc ^ PGP_CRC24_POLY;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc & 0xFFFFFF;
}

/// 零拷贝解析 OpenPGP ASCII Armor 块结构（BEGIN/Header/空行/Body/可选 `=xxxx` CRC-24/END）
pub fn parsePgpArmor(text: []const u8): PgpArmorHeader? {
    let pos: usize = 0;
    let block_type: []const u8 = text[0..0];
    let found_begin: bool = false;

    while (true) {
        const lv = nextLine(text, pos) ?? break;
        pos = lv.next_pos;
        if (startsWith(lv.line, "-----BEGIN ") && endsWith(lv.line, "-----") && lv.line.len > 16) {
            block_type = lv.line[11..lv.line.len - 5];
            found_begin = true;
            break;
        }
    }
    if (!found_begin || block_type.len == 0) {
        return null;
    }

    // 读取 Armor Header 键值对直至空行
    const headers_start: usize = pos;
    let headers_end: usize = pos;
    let found_blank: bool = false;
    while (true) {
        const lv = nextLine(text, pos) ?? break;
        if (lv.line.len == 0) {
            headers_end = lv.line_start;
            pos = lv.next_pos;
            found_blank = true;
            break;
        }
        pos = lv.next_pos;
    }
    if (!found_blank) {
        return null;
    }

    // 扫描 Base64 正文、可选 `=xxxx` CRC-24 行，直至匹配的 `-----END <block_type>-----`
    const body_start: usize = pos;
    let body_end: usize = pos;
    let crc24_b64: []const u8 = text[0..0];
    let saw_crc: bool = false;
    let found_end: bool = false;

    while (true) {
        const lv = nextLine(text, pos) ?? break;
        if (startsWith(lv.line, "-----END ") && endsWith(lv.line, "-----") && lv.line.len > 14) {
            const end_type = lv.line[9..lv.line.len - 5];
            if (!bytesEq(end_type, block_type)) {
                return null;
            }
            if (!saw_crc) {
                body_end = lv.line_start;
            }
            found_end = true;
            break;
        }
        if (saw_crc) {
            // `=xxxx` 后必须紧跟 END 行
            return null;
        }
        if (lv.line.len > 0 && lv.line[0] == '=') {
            if (lv.line.len != 5) {
                return null;
            }
            body_end = lv.line_start;
            crc24_b64 = lv.line[1..5];
            saw_crc = true;
        }
        pos = lv.next_pos;
    }

    if (!found_end) {
        return null;
    }

    return PgpArmorHeader {
        block_type: block_type,
        headers_slice: text[headers_start..headers_end],
        body_b64: text[body_start..body_end],
        crc24_b64: crc24_b64,
    };
}

/// 解码 OpenPGP ASCII Armor 至 `out`，若存在 `=xxxx` 则强制校验 `pgpCrc24`
pub fn decodePgpArmor(text: []const u8, out: *[]u8): usize? {
    const armor = parsePgpArmor(text) ?? return null;
    const decoded_len = b64DecodeStreaming(armor.body_b64, out) ?? return null;

    if (armor.crc24_b64.len > 0) {
        if (armor.crc24_b64.len != 4) {
            return null;
        }
        let crc_bytes: [3]u8 = [0; 3];
        let crc_slice: []u8 = crc_bytes[0..3];
        const crc_len = b64DecodeStreaming(armor.crc24_b64, *crc_slice) ?? return null;
        if (crc_len != 3) {
            return null;
        }
        const declared_crc: u32 =
            ((crc_bytes[0] as u32) << 16) |
            ((crc_bytes[1] as u32) << 8) |
            (crc_bytes[2] as u32);
        const actual_crc: u32 = pgpCrc24(out.*[0..decoded_len]);
        if (actual_crc != declared_crc) {
            return null;
        }
    }

    return decoded_len;
}

/// 快捷解析 OpenPGP v4 Public Key Packet（Tag 6 / Tag 14）
pub fn parsePgpPublicKey(body: []const u8): PgpPublicKeyInfo? {
    if (body.len < 6 || body[0] != 4) {
        return null;
    }
    return PgpPublicKeyInfo {
        version: body[0],
        creation_time: readU32Be(body, 1),
        algorithm: body[5],
        key_material: body[6..body.len],
    };
}

/// 快捷解析 OpenPGP v4 Signature Packet（Tag 2）
pub fn parsePgpSignature(body: []const u8): PgpSignatureInfo? {
    if (body.len < 6 || body[0] != 4) {
        return null;
    }
    const hashed_len: usize = readU16Be(body, 4) as usize;
    if (body.len - 6 < hashed_len + 2) {
        return null;
    }
    const hashed = body[6..6 + hashed_len];
    const unhashed_len_off: usize = hashed_len + 6;
    const unhashed_len: usize = readU16Be(body, unhashed_len_off) as usize;
    const unhashed_start: usize = unhashed_len_off + 2;
    if (body.len - unhashed_start < unhashed_len) {
        return null;
    }
    return PgpSignatureInfo {
        version: body[0],
        sig_type: body[1],
        pub_alg: body[2],
        hash_alg: body[3],
        hashed_subpackets: hashed,
        unhashed_subpackets: body[unhashed_start..unhashed_start + unhashed_len],
    };
}

/// 快捷解析 OpenPGP Literal Data Packet（Tag 11）
pub fn parsePgpLiteralData(body: []const u8): PgpLiteralData? {
    if (body.len < 6) {
        return null;
    }
    const fmt = body[0];
    if (fmt != 'b' && fmt != 't' && fmt != 'u') {
        return null;
    }
    const name_len: usize = body[1] as usize;
    if (body.len - 2 < name_len + 4) {
        return null;
    }
    const ts_off: usize = name_len + 2;
    return PgpLiteralData {
        format: fmt,
        filename: body[2..ts_off],
        timestamp: readU32Be(body, ts_off),
        payload: body[ts_off + 4..body.len],
    };
}
