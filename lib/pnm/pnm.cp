// Origin: Pure-CP Stdlib (Zero-Alloc Netpbm PNM Image Codec) | Created: 2026-11-22

/// Netpbm P1–P6 图像格式类型
pub enum PnmFormat {
    p1_pbm_ascii,
    p2_pgm_ascii,
    p3_ppm_ascii,
    p4_pbm_raw,
    p5_pgm_raw,
    p6_ppm_raw,
}

/// Netpbm 图像文件头
pub struct PnmHeader {
    format: PnmFormat,
    width: u32,
    height: u32,
    maxval: u16,
    data_offset: usize,

    /// 颜色通道数（P1/P2/P4/P5 为 `1`，P3/P6 为 `3`）
    fn channels(): u8 {
        if (_.format == PnmFormat.p3_ppm_ascii || _.format == PnmFormat.p6_ppm_raw) {
            return 3;
        }
        return 1;
    }

    /// 是否为二进制像素格式（P4/P5/P6）
    fn isBinary(): bool {
        return _.format == PnmFormat.p4_pbm_raw
            || _.format == PnmFormat.p5_pgm_raw
            || _.format == PnmFormat.p6_ppm_raw;
    }

    /// 防溢出计算二进制格式（P4/P5/P6）像素区字节数；ASCII 格式或溢出时返回 `null`
    fn expectedRawBytes(): usize? {
        if (!_.isBinary() || _.width == 0 || _.height == 0) {
            return null;
        }
        const MAX_SAFE_BYTES: u64 = 0x7FFF_FFFF_FFFF_FFFF;
        if (_.format == PnmFormat.p4_pbm_raw) {
            const row_bytes = (u64(_.width) + 7) / 8;
            if (u64(_.height) > MAX_SAFE_BYTES / row_bytes) {
                return null;
            }
            return usize(row_bytes * u64(_.height));
        }
        const bytes_per_sample: u64 = if (_.maxval > 255) 2 else 1;
        const per_pixel = u64(_.channels()) * bytes_per_sample;
        const w = u64(_.width);
        const h = u64(_.height);
        if (w > MAX_SAFE_BYTES / h) {
            return null;
        }
        const pixels = w * h;
        if (pixels > MAX_SAFE_BYTES / per_pixel) {
            return null;
        }
        return usize(pixels * per_pixel);
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// 跳过空白与 `# ... \n` 注释行
fn skipWsAndComments(data: []const u8, start: usize): usize {
    let i = start;
    while (i < data.len) {
        if (isSpace(data[i])) {
            i += 1;
        } else if (data[i] == '#') {
            while (i < data.len && data[i] != '\n') {
                i += 1;
            }
        } else {
            break;
        }
    }
    return i;
}

struct ParsedU32 {
    val: u32,
    next_pos: usize,
}

fn parsePositiveU32(data: []const u8, start: usize): ParsedU32? {
    let i = start;
    if (i >= data.len || !isDigit(data[i])) {
        return null;
    }
    let acc: u64 = 0;
    while (i < data.len && isDigit(data[i])) {
        acc = acc * 10 + u64(data[i] - '0');
        if (acc > 0xFFFF_FFFF) {
            return null;
        }
        i += 1;
    }
    if (acc == 0) {
        return null;
    }
    return ParsedU32 {
        val: u32(acc),
        next_pos: i,
    };
}

/// 解析 Netpbm P1–P6 文件头（跳过 `#` 注释与空白，严格在末字段后仅消耗单个空白符以保护二进制像素首字节）
pub fn parsePnmHeader(data: []const u8): PnmHeader? {
    if (data.len < 3 || data[0] != 'P') {
        return null;
    }

    let format = PnmFormat.p1_pbm_ascii;
    const d = data[1];
    if (d == '1') {
        format = PnmFormat.p1_pbm_ascii;
    } else if (d == '2') {
        format = PnmFormat.p2_pgm_ascii;
    } else if (d == '3') {
        format = PnmFormat.p3_ppm_ascii;
    } else if (d == '4') {
        format = PnmFormat.p4_pbm_raw;
    } else if (d == '5') {
        format = PnmFormat.p5_pgm_raw;
    } else if (d == '6') {
        format = PnmFormat.p6_ppm_raw;
    } else {
        return null;
    }

    // 魔数后必须有空白或 `#` 注释分隔
    if (!isSpace(data[2]) && data[2] != '#') {
        return null;
    }

    let pos = skipWsAndComments(data, 2);

    // 读取 width
    let width: u32 = 0;
    if (pw := parsePositiveU32(data, pos)) {
        width = pw.val;
        pos = pw.next_pos;
    } else {
        return null;
    }
    if (pos >= data.len || (!isSpace(data[pos]) && data[pos] != '#')) {
        return null;
    }
    pos = skipWsAndComments(data, pos);

    // 读取 height
    let height: u32 = 0;
    if (ph := parsePositiveU32(data, pos)) {
        height = ph.val;
        pos = ph.next_pos;
    } else {
        return null;
    }

    let maxval: u16 = 1;
    const has_maxval = format != PnmFormat.p1_pbm_ascii && format != PnmFormat.p4_pbm_raw;
    if (has_maxval) {
        if (pos >= data.len || (!isSpace(data[pos]) && data[pos] != '#')) {
            return null;
        }
        pos = skipWsAndComments(data, pos);
        if (pm := parsePositiveU32(data, pos)) {
            if (pm.val > 65535) {
                return null;
            }
            maxval = u16(pm.val);
            pos = pm.next_pos;
        } else {
            return null;
        }
    }

    // 最后一个头字段后必须紧跟至少一个空白字符，且仅消耗单个空白字符（保护二进制像素首字节）
    if (pos >= data.len || !isSpace(data[pos])) {
        return null;
    }
    pos += 1;

    return PnmHeader {
        format: format,
        width: width,
        height: height,
        maxval: maxval,
        data_offset: pos,
    };
}

/// 对 P5/P6 图像校验像素负载长度足够后返回零拷贝原始像素切片
pub fn pnmRawPixels(data: []const u8): ([]const u8)? {
    let hdr = PnmHeader {
        format: PnmFormat.p1_pbm_ascii,
        width: 0,
        height: 0,
        maxval: 1,
        data_offset: 0,
    };
    if (h := parsePnmHeader(data)) {
        hdr = h;
    } else {
        return null;
    }

    if (hdr.format != PnmFormat.p5_pgm_raw && hdr.format != PnmFormat.p6_ppm_raw) {
        return null;
    }

    let expected: usize = 0;
    if (ex := hdr.expectedRawBytes()) {
        expected = ex;
    } else {
        return null;
    }

    if (hdr.data_offset > data.len || expected > data.len - hdr.data_offset) {
        return null;
    }
    return data[hdr.data_offset .. hdr.data_offset + expected];
}

fn writeU32Dec(out: []u8, pos: usize, val: u32): usize? {
    let tmp: [10]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = u8('0' + (v % 10));
            v = v / 10;
            n += 1;
        }
    }
    if (pos > out.len || n > out.len - pos) {
        return null;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

fn encodePnmBinary(
    magic_digit: u8,
    channels: u8,
    pixels: []const u8,
    width: u32,
    height: u32,
    out: []u8,
): ([]const u8)? {
    if (width == 0 || height == 0) {
        return null;
    }
    const expected_u64 = u64(width) * u64(height) * u64(channels);
    if (expected_u64 != u64(pixels.len)) {
        return null;
    }

    let p: usize = 0;
    if (out.len < 3) return null;
    out[0] = 'P';
    out[1] = magic_digit;
    out[2] = '\n';
    p = 3;

    if (p_w := writeU32Dec(out, p, width)) {
        p = p_w;
    } else {
        return null;
    }
    if (p >= out.len) return null;
    out[p] = ' ';
    p += 1;

    if (p_h := writeU32Dec(out, p, height)) {
        p = p_h;
    } else {
        return null;
    }

    // 写入 "\n255\n" (5 字节)
    if (p > out.len || 5 > out.len - p) return null;
    out[p] = '\n';
    out[p + 1] = '2';
    out[p + 2] = '5';
    out[p + 3] = '5';
    out[p + 4] = '\n';
    p += 5;

    if (pixels.len > out.len - p) {
        return null;
    }
    let i: usize = 0;
    while (i < pixels.len) {
        out[p + i] = pixels[i];
        i += 1;
    }
    p += pixels.len;
    return out[0..p];
}

/// 零堆分配编码 PPM P6 (`P6\n<w> <h>\n255\n<rgb>`)；参数非法或缓冲不足返回 `null`
pub fn encodePpmP6(rgb: []const u8, width: u32, height: u32, out: []u8): ([]const u8)? {
    return encodePnmBinary('6', 3, rgb, width, height, out);
}

/// 零堆分配编码 PGM P5 (`P5\n<w> <h>\n255\n<gray>`)；参数非法或缓冲不足返回 `null`
pub fn encodePgmP5(gray: []const u8, width: u32, height: u32, out: []u8): ([]const u8)? {
    return encodePnmBinary('5', 1, gray, width, height, out);
}
