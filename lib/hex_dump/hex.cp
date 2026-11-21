// Source: Absorbed from /data/dev/bun/src/fmt.zig | Synced: 2026-11-21T14:40:13Z (Hex Encode/Decode & Canonical HexDump Formatter)

const HEX_LOWER: []const u8 = "0123456789abcdef";
const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn isIgnorableSep(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':' || c == '-';
}

fn encodeHexWithAlphabet(bytes: []const u8, out: []u8, alphabet: []const u8): ([]const u8)? {
    if (bytes.len > out.len / 2) {
        return null;
    }
    let i: usize = 0;
    while (i < bytes.len) {
        const b = bytes[i];
        out[i * 2] = alphabet[usize(b >> 4)];
        out[i * 2 + 1] = alphabet[usize(b & 0x0F)];
        i += 1;
    }
    return out[0..bytes.len * 2];
}

/// 将字节切片编码为小写十六进制字符串（零堆分配）
export fn encodeHexLower(bytes: []const u8, out: []u8): ([]const u8)? {
    return encodeHexWithAlphabet(bytes, out, HEX_LOWER);
}

/// 将字节切片编码为大写十六进制字符串（零堆分配）
export fn encodeHexUpper(bytes: []const u8, out: []u8): ([]const u8)? {
    return encodeHexWithAlphabet(bytes, out, HEX_UPPER);
}

/// 严格解码十六进制字符串（拒绝奇数长度、空白与任何非十六进制字符）
export fn decodeHexStrict(hex: []const u8, out: []u8): ([]const u8)? {
    if ((hex.len & 1) != 0) {
        return null;
    }
    const byte_len = hex.len / 2;
    if (out.len < byte_len) {
        return null;
    }
    let i: usize = 0;
    while (i < byte_len) {
        const hi = hexNibble(hex[i * 2]) ?? return null;
        const lo = hexNibble(hex[i * 2 + 1]) ?? return null;
        out[i] = (hi << 4) | lo;
        i += 1;
    }
    return out[0..byte_len];
}

/// 宽松解码十六进制字符串（忽略空格/冒号/连字符/换行分隔符，拒绝非法字符与奇数半字节）
export fn decodeHex(hex: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let has_hi = false;
    let hi_val: u8 = 0;

    for (c of hex) {
        if (isIgnorableSep(c)) {
            continue;
        }
        const nib = hexNibble(c) ?? return null;
        if (!has_hi) {
            hi_val = nib;
            has_hi = true;
        } else {
            if (pos >= out.len) {
                return null;
            }
            out[pos] = (hi_val << 4) | nib;
            pos += 1;
            has_hi = false;
        }
    }

    if (has_hi) {
        return null;
    }
    return out[0..pos];
}

/// 格式化单行规范化 `hexdump -C`（最多取 16 字节，不足 16 字节自动空格对齐）
export fn formatHexDumpLine(bytes: []const u8, base_offset: usize, out: []u8): ([]const u8)? {
    const chunk_len: usize = if (bytes.len > 16) 16 else bytes.len;
    // 8 (offset) + 2 ("  ") + 16*3 (hex) + 1 (mid space) + 1 (pre-pipe space) + 2 ("||") + chunk_len
    const needed: usize = usize(62) + chunk_len;
    if (out.len < needed) {
        return null;
    }

    let pos: usize = 0;

    // 1. 8 位小写十六进制偏移量（取低 32 位）
    const off32: u32 = @truncate(base_offset);
    let shift: u6 = 28;
    let d: usize = 0;
    while (d < 8) {
        const nib: usize = usize((off32 >> @intCast(shift)) & 0xF);
        out[pos] = HEX_LOWER[nib];
        pos += 1;
        if (shift >= 4) {
            shift -= 4;
        }
        d += 1;
    }

    out[pos] = ' ';
    out[pos + 1] = ' ';
    pos += 2;

    // 2. 16 字节十六进制列（每 8 字节之间多加 1 空格）
    let i: usize = 0;
    while (i < 16) {
        if (i == 8) {
            out[pos] = ' ';
            pos += 1;
        }
        if (i < chunk_len) {
            const b = bytes[i];
            out[pos] = HEX_LOWER[usize(b >> 4)];
            out[pos + 1] = HEX_LOWER[usize(b & 0x0F)];
            out[pos + 2] = ' ';
        } else {
            out[pos] = ' ';
            out[pos + 1] = ' ';
            out[pos + 2] = ' ';
        }
        pos += 3;
        i += 1;
    }

    out[pos] = ' ';
    out[pos + 1] = '|';
    pos += 2;

    // 3. 可打印 ASCII 列（非可打印字符显示 '.'）
    let j: usize = 0;
    while (j < chunk_len) {
        const b = bytes[j];
        out[pos] = if (b >= 0x20 && b <= 0x7E) b else '.';
        pos += 1;
        j += 1;
    }

    out[pos] = '|';
    pos += 1;
    return out[0..pos];
}

/// 格式化完整多行规范化 `hexdump -C` 输出（每行 16 字节，行间以 `\n` 分隔）
export fn formatHexDump(bytes: []const u8, out: []u8): ([]const u8)? {
    if (bytes.len == 0) {
        return out[0..0];
    }

    let offset: usize = 0;
    let pos: usize = 0;

    while (offset < bytes.len) {
        if (offset > 0) {
            if (pos >= out.len) {
                return null;
            }
            out[pos] = '\n';
            pos += 1;
        }
        const remaining = bytes.len - offset;
        const step: usize = if (remaining > 16) 16 else remaining;
        const line = formatHexDumpLine(bytes[offset..offset + step], offset, out[pos..]) ?? return null;
        pos += line.len;
        offset += step;
    }

    return out[0..pos];
}
