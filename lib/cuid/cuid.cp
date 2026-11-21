// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc CUID / CUID2 Codec & Validator)

/// CUID 版本枚举
pub enum CuidVersion: u8 {
    unknown = 0,
    v1 = 1,
    v2 = 2,
}

const BASE36_ALPHABET: []const u8 = "0123456789abcdefghijklmnopqrstuvwxyz";
const U64_MAX_DIV_36: u64 = 512409557603043100;
const U64_MAX_REM_36: u64 = 15;

fn isLowerAlpha(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn isBase36Lower(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z');
}

fn base36Digit(c: u8): u64? {
    if (c >= '0' && c <= '9') {
        return (c - '0') as u64;
    }
    if (c >= 'a' && c <= 'z') {
        return ((c - 'a') + 10) as u64;
    }
    return null;
}

fn writeBase36Fixed(val: u64, dst: []u8) {
    let v = val;
    let i = dst.len;
    while (i > 0) {
        i -= 1;
        const rem: usize = (v % 36) as usize;
        dst[i] = BASE36_ALPHABET[rem];
        v = v / 36;
    }
}

/// 校验是否为合法 25 字节 CUID v1（以 `'c'` 开头，其余 24 字节均为小写 base36 `[0-9a-z]`）
pub fn isValidCuid1(s: []const u8): bool {
    if (s.len != 25 || s[0] != 'c') {
        return false;
    }
    let i: usize = 1;
    while (i < 25) {
        if (!isBase36Lower(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 校验是否为合法 CUID2（长度 `2..=32`、首字节小写字母 `[a-z]`、其余为小写 base36 `[0-9a-z]`）
pub fn isValidCuid2(s: []const u8): bool {
    if (s.len < 2 || s.len > 32) {
        return false;
    }
    if (!isLowerAlpha(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isBase36Lower(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 自动识别 CUID 版本（25 字节 `'c'` 开头优先识别为 `v1`，其余合法 CUID2 识别为 `v2`）
pub fn detectCuidVersion(s: []const u8): CuidVersion {
    if (isValidCuid1(s)) {
        return CuidVersion.v1;
    }
    if (isValidCuid2(s)) {
        return CuidVersion.v2;
    }
    return CuidVersion.unknown;
}

/// 零堆分配将 `u64` 编码为小写 base36 字符串；输出缓冲不足返回 `null`
pub fn encodeBase36(val: u64, out: []u8): ([]const u8)? {
    if (val == 0) {
        if (out.len < 1) {
            return null;
        }
        out[0] = '0';
        return out[0..1];
    }

    let tmp: [16]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        const rem: usize = (v % 36) as usize;
        tmp[n] = BASE36_ALPHABET[rem];
        n += 1;
        v = v / 36;
    }

    if (out.len < n) {
        return null;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = tmp[n - 1 - i];
        i += 1;
    }
    return out[0..n];
}

/// 零堆分配解析小写 base36 字符串为 `u64`；空串、非法字符或 u64 溢出返回 `null`
pub fn decodeBase36(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const d = base36Digit(s[i]) ?? return null;
        if (acc > U64_MAX_DIV_36 || (acc == U64_MAX_DIV_36 && d > U64_MAX_REM_36)) {
            return null;
        }
        acc = acc * 36 + d;
        i += 1;
    }
    return acc;
}

fn isAllBase36Lower(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (!isBase36Lower(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn hashFingerprint(fp: []const u8): u64 {
    let h: u64 = 2166136261;
    let i: usize = 0;
    while (i < fp.len) {
        h = (h ^ (fp[i] as u64)) *% 16777619;
        i += 1;
    }
    return h;
}

/// 按经典 CUID v1 25 字节定长结构 `'c' + ts(8) + cnt(4) + fp(4) + rand(8)` 确定性编码
/// - 若 `fingerprint` 非空且全为小写 base36，则右对齐/左补 `'0'`（超过 4 字节取末 4 字节）写入 `fp(4)`；
/// - 否则对 `fingerprint` 做确定性 FNV-1a 哈希并编码为 4 字节 base36；
/// - 输出缓冲小于 25 字节返回 `null`。
pub fn formatCuid1(
    timestamp_ms: u64,
    counter: u32,
    fingerprint: []const u8,
    random_val: u64,
    out: []u8,
): ([]const u8)? {
    if (out.len < 25) {
        return null;
    }
    out[0] = 'c';
    writeBase36Fixed(timestamp_ms, out[1..9]);
    writeBase36Fixed(counter as u64, out[9..13]);

    if (fingerprint.len > 0 && isAllBase36Lower(fingerprint)) {
        if (fingerprint.len >= 4) {
            const start = fingerprint.len - 4;
            let i: usize = 0;
            while (i < 4) {
                out[13 + i] = fingerprint[start + i];
                i += 1;
            }
        } else {
            const pad = 4 - fingerprint.len;
            let i: usize = 0;
            while (i < pad) {
                out[13 + i] = '0';
                i += 1;
            }
            let j: usize = 0;
            while (j < fingerprint.len) {
                out[13 + pad + j] = fingerprint[j];
                j += 1;
            }
        }
    } else {
        writeBase36Fixed(hashFingerprint(fingerprint), out[13..17]);
    }

    writeBase36Fixed(random_val, out[17..25]);
    return out[0..25];
}

/// 从合法 25 字节 CUID v1 字符串提取毫秒级时间戳；格式非法返回 `null`
pub fn parseCuid1Timestamp(s: []const u8): u64? {
    if (!isValidCuid1(s)) {
        return null;
    }
    return decodeBase36(s[1..9]);
}
