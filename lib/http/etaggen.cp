// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (HTTP Strong & Weak ETag Generator)

const FNV1A_64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV1A_64_PRIME: u64 = 0x100000001b3;
const HEX_LOWER: []const u8 = "0123456789abcdef";

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeHexU64Min(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) {
        return writeByte(out, pos, '0');
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        const nibble: usize = (v & 0x0f) as usize;
        tmp[n] = HEX_LOWER[nibble];
        n += 1;
        v >>= 4;
    }
    if (out.len - pos.* < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

fn writeHexU64Fixed16(out: []u8, pos: *usize, val: u64): bool {
    if (out.len - pos.* < 16) {
        return false;
    }
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = ((15 - i) * 4) as u6;
        const nibble: usize = ((val >> shift) & 0x0f) as usize;
        out[pos.* + i] = HEX_LOWER[nibble];
        i += 1;
    }
    pos.* += 16;
    return true;
}

fn isEtagChar(c: u8): bool {
    return c == 0x21 || (c >= 0x23 && c <= 0x7E) || c >= 0x80;
}

fn formatHashEtag(body: []const u8, weak: bool, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (weak) {
        if (!writeSlice(out, &pos, "W/\"")) {
            return null;
        }
    } else {
        if (!writeByte(out, &pos, '"')) {
            return null;
        }
    }

    const hash = fnv1a64(body);
    if (!writeHexU64Fixed16(out, &pos, hash)) {
        return null;
    }
    if (!writeByte(out, &pos, '-')) {
        return null;
    }
    if (!writeHexU64Min(out, &pos, body.len as u64)) {
        return null;
    }
    if (!writeByte(out, &pos, '"')) {
        return null;
    }
    return out[0..pos];
}

/// 标准 64 位 FNV-1a 哈希（offset basis `0xcbf29ce484222325`，prime `0x100000001b3`）
export fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV1A_64_OFFSET;
    let i: usize = 0;
    while (i < data.len) {
        h ^= u64(data[i]);
        h *%= FNV1A_64_PRIME;
        i += 1;
    }
    return h;
}

/// 生成基于静态文件元数据的弱 ETag：`W/"<size_hex>-<mtime_hex>"`
export fn generateWeakStatEtag(size_bytes: u64, mtime_ms: u64, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!writeSlice(out, &pos, "W/\"")) {
        return null;
    }
    if (!writeHexU64Min(out, &pos, size_bytes)) {
        return null;
    }
    if (!writeByte(out, &pos, '-')) {
        return null;
    }
    if (!writeHexU64Min(out, &pos, mtime_ms)) {
        return null;
    }
    if (!writeByte(out, &pos, '"')) {
        return null;
    }
    return out[0..pos];
}

/// 生成基于 FNV-1a 64-bit 内容哈希的强 ETag：`"<16_lower_hex_fnv1a64>-<len_hex>"`
export fn generateStrongHashEtag(body: []const u8, out: []u8): ([]const u8)? {
    return formatHashEtag(body, false, out);
}

/// 生成基于 FNV-1a 64-bit 内容哈希的弱 ETag：`W/"<16_lower_hex_fnv1a64>-<len_hex>"`
export fn generateWeakHashEtag(body: []const u8, out: []u8): ([]const u8)? {
    return formatHashEtag(body, true, out);
}

/// 校验 RFC 9110 §8.8.3 ETag 语法：可选区分大小写的 `W/` 前缀 + 双引号包裹 + 内部仅合法 `etagc` 字符
export fn isValidEtagSyntax(raw: []const u8): bool {
    let s = raw;
    if (s.len >= 2 && s[0] == 'W' && s[1] == '/') {
        s = s[2..];
    }
    if (s.len < 2 || s[0] != '"' || s[s.len - 1] != '"') {
        return false;
    }
    let i: usize = 1;
    while (i < s.len - 1) {
        if (!isEtagChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}
