// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Minisign & OpenBSD Signify Codec)

/// Minisign / Signify 非信任注释行前缀
pub const MINISIGN_UNTRUSTED_PREFIX: []const u8 = "untrusted comment: ";

/// Minisign 信任注释行前缀
pub const MINISIGN_TRUSTED_PREFIX: []const u8 = "trusted comment: ";

/// 纯 Ed25519 传统签名算法标识（0x45 0x64）
pub const MINISIGN_ALG_LEGACY: []const u8 = "Ed";

/// BLAKE2b-512 预哈希 Ed25519 签名算法标识（0x45 0x44）
pub const MINISIGN_ALG_PREHASH: []const u8 = "ED";

/// Minisign 公钥算法标识（0x52 0x57）
pub const MINISIGN_PUBKEY_ALG: []const u8 = "RW";

/// Minisign 公钥二进制长度（2B "RW" + 8B key_id + 32B Ed25519 pk）
pub const MINISIGN_PUBKEY_BIN_LEN: usize = 42;

/// Minisign 签名二进制长度（2B "Ed"/"ED" + 8B key_id + 64B Ed25519 sig）
pub const MINISIGN_SIG_BIN_LEN: usize = 74;

/// Minisign 全局签名二进制长度（64B Ed25519 global sig）
pub const MINISIGN_GLOBAL_SIG_BIN_LEN: usize = 64;

/// Minisign / Signify 公钥结构（零堆分配）
pub struct MinisignPublicKey {
    untrusted_comment: []const u8,
    key_id: [8]u8,
    public_key: [32]u8,

    /// 将 8 字节 `key_id` 格式化为 16 字节大写十六进制字符串
    fn keyIdHex(out_16: *[16]u8) {
        const hex_digits = "0123456789ABCDEF";
        let i: usize = 0;
        while (i < 8) {
            const b = _.key_id[i];
            out_16.*[i * 2] = hex_digits[((b >> 4) & 0x0F) as usize];
            out_16.*[i * 2 + 1] = hex_digits[(b & 0x0F) as usize];
            i += 1;
        }
    }
}

/// Minisign 4 行签名（兼容 OpenBSD Signify 2 行签名）结构（零堆分配）
pub struct MinisignSignature {
    untrusted_comment: []const u8,
    alg: [2]u8,
    is_prehashed: bool,
    key_id: [8]u8,
    signature: [64]u8,
    trusted_comment: []const u8,
    global_signature: [64]u8,
    has_global_sig: bool,

    /// 检查该签名的 8 字节 `key_id` 是否与目标公钥匹配
    fn matchesKeyId(pk: MinisignPublicKey): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.key_id[i] != pk.key_id[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

// ---------------------------------------------------------------------------
// 内部零分配 Base64 编解码与行切分辅助函数
// ---------------------------------------------------------------------------

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimLineSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

struct LineSplit {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(text: []const u8, start: usize): LineSplit? {
    if (start >= text.len) {
        return null;
    }
    let i: usize = start;
    while (i < text.len && text[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > start && text[line_end - 1] == '\r') {
        line_end -= 1;
    }
    const next_pos: usize = if (i < text.len) i + 1 else i;
    return LineSplit {
        line: text[start..line_end],
        next_pos: next_pos,
    };
}

fn hasNonEmptyRemainder(text: []const u8, start: usize): bool {
    let i: usize = start;
    while (i < text.len) {
        const c = text[i];
        if (c != '\n' && c != '\r') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn b64Val(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

fn b64Decode(src: []const u8, out: *[]u8): usize? {
    if (src.len == 0 || (src.len % 4) != 0) {
        return null;
    }
    let out_pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const is_last = (i + 4) == src.len;
        const c0 = src[i];
        const c1 = src[i + 1];
        const c2 = src[i + 2];
        const c3 = src[i + 3];

        const v0 = b64Val(c0) ?? return null;
        const v1 = b64Val(c1) ?? return null;

        if (c2 == '=') {
            if (!is_last || c3 != '=') {
                return null;
            }
            if ((v1 & 0x0F) != 0) {
                return null;
            }
            if (out_pos + 1 > out.*.len) {
                return null;
            }
            out.*[out_pos] = (v0 << 2) | (v1 >> 4);
            out_pos += 1;
        } else if (c3 == '=') {
            if (!is_last) {
                return null;
            }
            const v2 = b64Val(c2) ?? return null;
            if ((v2 & 0x03) != 0) {
                return null;
            }
            if (out_pos + 2 > out.*.len) {
                return null;
            }
            out.*[out_pos] = (v0 << 2) | (v1 >> 4);
            out.*[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
            out_pos += 2;
        } else {
            const v2 = b64Val(c2) ?? return null;
            const v3 = b64Val(c3) ?? return null;
            if (out_pos + 3 > out.*.len) {
                return null;
            }
            out.*[out_pos] = (v0 << 2) | (v1 >> 4);
            out.*[out_pos + 1] = ((v1 & 0x0F) << 4) | (v2 >> 2);
            out.*[out_pos + 2] = ((v2 & 0x03) << 6) | v3;
            out_pos += 3;
        }
        i += 4;
    }
    return out_pos;
}

fn b64Encode(src: []const u8, out: *[]u8, pos: *usize): bool {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const needed: usize = ((src.len + 2) / 3) * 4;
    if (out.*.len - pos.* < needed) {
        return false;
    }

    let i: usize = 0;
    while (i + 3 <= src.len) {
        const b0 = src[i];
        const b1 = src[i + 1];
        const b2 = src[i + 2];
        out.*[pos.*] = alphabet[((b0 >> 2) & 0x3F) as usize];
        out.*[pos.* + 1] = alphabet[((((b0 & 0x03) << 4) | (b1 >> 4)) & 0x3F) as usize];
        out.*[pos.* + 2] = alphabet[((((b1 & 0x0F) << 2) | (b2 >> 6)) & 0x3F) as usize];
        out.*[pos.* + 3] = alphabet[(b2 & 0x3F) as usize];
        pos.* += 4;
        i += 3;
    }

    const rem = src.len - i;
    if (rem == 1) {
        const b0 = src[i];
        out.*[pos.*] = alphabet[((b0 >> 2) & 0x3F) as usize];
        out.*[pos.* + 1] = alphabet[((b0 & 0x03) << 4) as usize];
        out.*[pos.* + 2] = '=';
        out.*[pos.* + 3] = '=';
        pos.* += 4;
    } else if (rem == 2) {
        const b0 = src[i];
        const b1 = src[i + 1];
        out.*[pos.*] = alphabet[((b0 >> 2) & 0x3F) as usize];
        out.*[pos.* + 1] = alphabet[((((b0 & 0x03) << 4) | (b1 >> 4)) & 0x3F) as usize];
        out.*[pos.* + 2] = alphabet[((b1 & 0x0F) << 2) as usize];
        out.*[pos.* + 3] = '=';
        pos.* += 4;
    }

    return true;
}

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (out.*.len - pos.* < src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out.*[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
    return true;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析单行裸 Base64 Minisign 公钥（常见于命令行 `-P RW...`）
pub fn parseMinisignPublicKeyLine(b64: []const u8): MinisignPublicKey? {
    const trimmed = trimLineSpaces(b64);
    let bin: [42]u8 = [0; 42];
    let bin_slice: []u8 = bin[0..42];
    const decoded_len = b64Decode(trimmed, *bin_slice) ?? return null;
    if (decoded_len != MINISIGN_PUBKEY_BIN_LEN) {
        return null;
    }
    // 严格校验公钥算法标识 "RW" (0x52 0x57)
    if (bin[0] != 'R' || bin[1] != 'W') {
        return null;
    }

    let key_id: [8]u8 = [0; 8];
    let k: usize = 0;
    while (k < 8) {
        key_id[k] = bin[2 + k];
        k += 1;
    }

    let pk: [32]u8 = [0; 32];
    let p: usize = 0;
    while (p < 32) {
        pk[p] = bin[10 + p];
        p += 1;
    }

    return MinisignPublicKey {
        untrusted_comment: b64[0..0],
        key_id: key_id,
        public_key: pk,
    };
}

/// 解析 2 行 `.pub` 格式 Minisign / Signify 公钥文件
pub fn parseMinisignPublicKey(text: []const u8): MinisignPublicKey? {
    const l1 = nextLine(text, 0) ?? return null;
    if (!startsWith(l1.line, MINISIGN_UNTRUSTED_PREFIX)) {
        return null;
    }
    const comment = l1.line[MINISIGN_UNTRUSTED_PREFIX.len..l1.line.len];

    const l2 = nextLine(text, l1.next_pos) ?? return null;
    if (hasNonEmptyRemainder(text, l2.next_pos)) {
        return null;
    }

    let pk = parseMinisignPublicKeyLine(l2.line) ?? return null;
    pk.untrusted_comment = comment;
    return pk;
}

/// 解析 4 行 `.minisig` 格式签名（同时兼容 2 行 OpenBSD Signify 签名格式）
pub fn parseMinisignSignature(text: []const u8): MinisignSignature? {
    const l1 = nextLine(text, 0) ?? return null;
    if (!startsWith(l1.line, MINISIGN_UNTRUSTED_PREFIX)) {
        return null;
    }
    const untrusted = l1.line[MINISIGN_UNTRUSTED_PREFIX.len..l1.line.len];

    const l2 = nextLine(text, l1.next_pos) ?? return null;
    let bin_sig: [74]u8 = [0; 74];
    let bin_sig_slice: []u8 = bin_sig[0..74];
    const sig_len = b64Decode(trimLineSpaces(l2.line), *bin_sig_slice) ?? return null;
    if (sig_len != MINISIGN_SIG_BIN_LEN) {
        return null;
    }

    let is_prehashed: bool = false;
    if (bin_sig[0] == 'E' && bin_sig[1] == 'd') {
        is_prehashed = false;
    } else if (bin_sig[0] == 'E' && bin_sig[1] == 'D') {
        is_prehashed = true;
    } else {
        return null;
    }

    let alg: [2]u8 = [bin_sig[0], bin_sig[1]];
    let key_id: [8]u8 = [0; 8];
    let k: usize = 0;
    while (k < 8) {
        key_id[k] = bin_sig[2 + k];
        k += 1;
    }

    let signature: [64]u8 = [0; 64];
    let s: usize = 0;
    while (s < 64) {
        signature[s] = bin_sig[10 + s];
        s += 1;
    }

    let global_signature: [64]u8 = [0; 64];

    // 若后面无非空行，则按 2 行 OpenBSD Signify 兼容格式返回
    if (!hasNonEmptyRemainder(text, l2.next_pos)) {
        return MinisignSignature {
            untrusted_comment: untrusted,
            alg: alg,
            is_prehashed: is_prehashed,
            key_id: key_id,
            signature: signature,
            trusted_comment: text[0..0],
            global_signature: global_signature,
            has_global_sig: false,
        };
    }

    // 4 行 Minisign 格式：行 3 为 trusted comment，行 4 为 64B global signature
    const l3 = nextLine(text, l2.next_pos) ?? return null;
    if (!startsWith(l3.line, MINISIGN_TRUSTED_PREFIX)) {
        return null;
    }
    const trusted = l3.line[MINISIGN_TRUSTED_PREFIX.len..l3.line.len];

    const l4 = nextLine(text, l3.next_pos) ?? return null;
    if (hasNonEmptyRemainder(text, l4.next_pos)) {
        return null;
    }

    let global_slice: []u8 = global_signature[0..64];
    const g_len = b64Decode(trimLineSpaces(l4.line), *global_slice) ?? return null;
    if (g_len != MINISIGN_GLOBAL_SIG_BIN_LEN) {
        return null;
    }

    return MinisignSignature {
        untrusted_comment: untrusted,
        alg: alg,
        is_prehashed: is_prehashed,
        key_id: key_id,
        signature: signature,
        trusted_comment: trusted,
        global_signature: global_signature,
        has_global_sig: true,
    };
}

/// 零分配格式化 2 行 `.pub` Minisign 公钥文件
pub fn formatMinisignPublicKey(
    comment: []const u8,
    key_id: [8]u8,
    pk: [32]u8,
    out: *[]u8,
): usize? {
    let bin: [42]u8 = [0; 42];
    bin[0] = 'R';
    bin[1] = 'W';
    let k: usize = 0;
    while (k < 8) {
        bin[2 + k] = key_id[k];
        k += 1;
    }
    let p: usize = 0;
    while (p < 32) {
        bin[10 + p] = pk[p];
        p += 1;
    }

    let pos: usize = 0;
    if (!appendBytes(out, *pos, MINISIGN_UNTRUSTED_PREFIX)) return null;
    if (!appendBytes(out, *pos, comment)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    if (!b64Encode(bin[0..42], out, *pos)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    return pos;
}

/// 零分配格式化 4 行 `.minisig` Minisign 签名文件
pub fn formatMinisignSignature(
    untrusted: []const u8,
    is_prehashed: bool,
    key_id: [8]u8,
    sig: [64]u8,
    trusted: []const u8,
    global_sig: [64]u8,
    out: *[]u8,
): usize? {
    let bin: [74]u8 = [0; 74];
    bin[0] = 'E';
    bin[1] = if (is_prehashed) 'D' else 'd';
    let k: usize = 0;
    while (k < 8) {
        bin[2 + k] = key_id[k];
        k += 1;
    }
    let s: usize = 0;
    while (s < 64) {
        bin[10 + s] = sig[s];
        s += 1;
    }

    let pos: usize = 0;
    if (!appendBytes(out, *pos, MINISIGN_UNTRUSTED_PREFIX)) return null;
    if (!appendBytes(out, *pos, untrusted)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    if (!b64Encode(bin[0..74], out, *pos)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    if (!appendBytes(out, *pos, MINISIGN_TRUSTED_PREFIX)) return null;
    if (!appendBytes(out, *pos, trusted)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    if (!b64Encode(global_sig[0..64], out, *pos)) return null;
    if (!appendBytes(out, *pos, "\n")) return null;
    return pos;
}
