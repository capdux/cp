// Origin: Pure-CP Stdlib (Zero-Alloc Intel HEX I8HEX/I16HEX/I32HEX Firmware Codec) | Created: 2026-11-22

/// Intel HEX 记录类型 (I8HEX / I16HEX / I32HEX)
pub enum IhexRecordType: u8 {
    data = 0,
    eof = 1,
    ext_segment_addr = 2,
    start_segment_addr = 3,
    ext_linear_addr = 4,
    start_linear_addr = 5,
    unknown = 255,
}

/// 将原始记录类型字节转换为 `IhexRecordType`
pub fn ihexRecordTypeFromByte(b: u8): IhexRecordType {
    if (b == 0) return IhexRecordType.data;
    if (b == 1) return IhexRecordType.eof;
    if (b == 2) return IhexRecordType.ext_segment_addr;
    if (b == 3) return IhexRecordType.start_segment_addr;
    if (b == 4) return IhexRecordType.ext_linear_addr;
    if (b == 5) return IhexRecordType.start_linear_addr;
    return IhexRecordType.unknown;
}

/// 单条已解析的 Intel HEX 记录（零堆分配，载荷定长栈缓冲 `[255]u8`）
pub struct IhexRecord {
    byte_count: u8,
    address: u16,
    record_type: IhexRecordType,
    data: [255]u8,
    checksum: u8,

    /// 获取索引 `idx` 处的数据字节（越界返回 `null`）
    pub fn dataByte(idx: usize): u8? {
        if (idx >= usize(_.byte_count)) return null;
        return _.data[idx];
    }

    /// 返回当前记录有效载荷切片 `data[0..byte_count]`
    pub fn dataSlice(): []const u8 {
        return _.data[0..usize(_.byte_count)];
    }
}

/// 附带 32-bit 绝对地址的数据记录块
pub struct IhexChunk {
    abs_address: u32,
    record: IhexRecord,
}

/// Intel HEX 固件镜像统计汇总
pub struct IhexStats {
    data_records: usize = 0,
    total_bytes: usize = 0,
    min_addr: u32 = 0,
    max_addr: u32 = 0,
    has_eof: bool = false,
    entry_addr: u32? = null,
}

fn parseHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseHexByte(hi: u8, lo: u8): u8? {
    const h = parseHexNibble(hi) ?? return null;
    const l = parseHexNibble(lo) ?? return null;
    return (h << 4) | l;
}

fn hexUpperDigit(v: u8): u8 {
    const d = v & 0x0F;
    if (d < 10) return '0' + d;
    return 'A' + (d - 10);
}

fn writeHexByte(out: []u8, pos: usize, b: u8) {
    out[pos] = hexUpperDigit(b >> 4);
    out[pos + 1] = hexUpperDigit(b & 0x0F);
}

/// 计算 Intel HEX 二补码校验和：`(~sum + 1) & 0xFF`
pub fn ihexChecksum(byte_count: u8, address: u16, rec_type: u8, payload: []const u8): u8 {
    let sum: u32 = u32(byte_count) + u32((address >> 8) & 0xFF) + u32(address & 0xFF) + u32(rec_type);
    let i: usize = 0;
    while (i < payload.len) {
        sum += u32(payload[i]);
        i += 1;
    }
    return ((256 - (sum & 0xFF)) & 0xFF) as u8;
}

/// 严格解析单条 Intel HEX 记录行（校验 `:` 前缀、偶数十六进制、精确长度、二补码校验和与类型约束）
pub fn parseIhexLine(line: []const u8): IhexRecord? {
    // 去除行尾 `\r` / `\n`
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    const s = line[0..end];
    if (s.len < 11 || s[0] != ':') return null;
    // 除 `:` 外必须为偶数个十六进制字符
    if (((s.len - 1) & 1) != 0) return null;

    const byte_count = parseHexByte(s[1], s[2]) ?? return null;
    const expected_len: usize = usize(byte_count) * 2 + 11;
    if (s.len != expected_len) return null;

    const addr_hi = parseHexByte(s[3], s[4]) ?? return null;
    const addr_lo = parseHexByte(s[5], s[6]) ?? return null;
    const address: u16 = (u16(addr_hi) << 8) | u16(addr_lo);

    const rec_type_byte = parseHexByte(s[7], s[8]) ?? return null;
    const rec_type = ihexRecordTypeFromByte(rec_type_byte);
    if (rec_type == IhexRecordType.unknown) return null;

    // 按 Intel HEX 规范校验记录类型与长度/地址约束
    if (rec_type == IhexRecordType.eof) {
        if (byte_count != 0) return null;
    } else if (rec_type == IhexRecordType.ext_segment_addr) {
        if (byte_count != 2 || address != 0) return null;
    } else if (rec_type == IhexRecordType.start_segment_addr) {
        if (byte_count != 4 || address != 0) return null;
    } else if (rec_type == IhexRecordType.ext_linear_addr) {
        if (byte_count != 2 || address != 0) return null;
    } else if (rec_type == IhexRecordType.start_linear_addr) {
        if (byte_count != 4 || address != 0) return null;
    }

    let rec = IhexRecord {
        byte_count: byte_count,
        address: address,
        record_type: rec_type,
        data: [0; 255],
        checksum: 0,
    };

    let i: usize = 0;
    const n = usize(byte_count);
    while (i < n) {
        const idx = 9 + i * 2;
        rec.data[i] = parseHexByte(s[idx], s[idx + 1]) ?? return null;
        i += 1;
    }

    const chk_idx = 9 + n * 2;
    const checksum = parseHexByte(s[chk_idx], s[chk_idx + 1]) ?? return null;
    const expected_chk = ihexChecksum(byte_count, address, rec_type_byte, rec.data[0..n]);
    if (checksum != expected_chk) return null;

    rec.checksum = checksum;
    return rec;
}

/// 零堆分配 Intel HEX 32-bit 绝对地址流式迭代器
pub struct IhexIter {
    text: []const u8,
    pos: usize = 0,
    upper_linear_base: u32 = 0,
    segment_base: u32 = 0,
    start_linear_address: u32? = null,
    start_segment_cs: u16? = null,
    start_segment_ip: u16? = null,
    has_eof: bool = false,
    had_error: bool = false,

    pub static fn init(text: []const u8): IhexIter {
        return IhexIter {
            text: text,
            pos: 0,
            upper_linear_base: 0,
            segment_base: 0,
            start_linear_address: null,
            start_segment_cs: null,
            start_segment_ip: null,
            has_eof: false,
            had_error: false,
        };
    }

    pub fn next(*_): IhexChunk? {
        if (_.has_eof || _.had_error) return null;

        while (_.pos < _.text.len) {
            const line_start = _.pos;
            while (_.pos < _.text.len && _.text[_.pos] != '\n') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.text.len && _.text[_.pos] == '\n') {
                _.pos += 1;
            }

            // 跳过纯空行（仅含 `\r` 或空）
            let trimmed_end = line_end;
            while (trimmed_end > line_start && _.text[trimmed_end - 1] == '\r') {
                trimmed_end -= 1;
            }
            if (trimmed_end == line_start) {
                continue;
            }

            const line = _.text[line_start..trimmed_end];
            const rec_opt = parseIhexLine(line);
            if (rec_opt == null) {
                _.had_error = true;
                return null;
            }
            const rec = rec_opt ?? return null;

            if (rec.record_type == IhexRecordType.data) {
                const abs_addr: u32 = _.upper_linear_base + _.segment_base + u32(rec.address);
                return IhexChunk {
                    abs_address: abs_addr,
                    record: rec,
                };
            } else if (rec.record_type == IhexRecordType.eof) {
                _.has_eof = true;
                return null;
            } else if (rec.record_type == IhexRecordType.ext_linear_addr) {
                const upper: u32 = (u32(rec.data[0]) << 8) | u32(rec.data[1]);
                _.upper_linear_base = upper << 16;
                _.segment_base = 0;
            } else if (rec.record_type == IhexRecordType.ext_segment_addr) {
                const seg: u32 = (u32(rec.data[0]) << 8) | u32(rec.data[1]);
                _.segment_base = seg << 4;
                _.upper_linear_base = 0;
            } else if (rec.record_type == IhexRecordType.start_linear_addr) {
                const entry: u32 = (u32(rec.data[0]) << 24) |
                    (u32(rec.data[1]) << 16) |
                    (u32(rec.data[2]) << 8) |
                    u32(rec.data[3]);
                _.start_linear_address = entry;
            } else if (rec.record_type == IhexRecordType.start_segment_addr) {
                _.start_segment_cs = (u16(rec.data[0]) << 8) | u16(rec.data[1]);
                _.start_segment_ip = (u16(rec.data[2]) << 8) | u16(rec.data[3]);
            }
        }

        return null;
    }
}

/// 零堆分配编码单条 Intel HEX 记录（输出大写 `:LLAAAATT...CC\n`；载荷超 255 字节或缓冲区不足返回 `null`）
pub fn encodeIhexLine(address: u16, rec_type: IhexRecordType, payload: []const u8, out: []u8): usize? {
    if (payload.len > 255) return null;
    if (rec_type == IhexRecordType.unknown) return null;

    const needed: usize = payload.len * 2 + 12;
    if (out.len < needed) return null;

    const ll: u8 = payload.len as u8;
    const tt: u8 = rec_type as u8;
    const chk = ihexChecksum(ll, address, tt, payload);

    out[0] = ':';
    writeHexByte(out, 1, ll);
    writeHexByte(out, 3, ((address >> 8) & 0xFF) as u8);
    writeHexByte(out, 5, (address & 0xFF) as u8);
    writeHexByte(out, 7, tt);

    let i: usize = 0;
    while (i < payload.len) {
        writeHexByte(out, 9 + i * 2, payload[i]);
        i += 1;
    }

    const chk_pos = 9 + payload.len * 2;
    writeHexByte(out, chk_pos, chk);
    out[chk_pos + 2] = '\n';

    return needed;
}

/// 统计 Intel HEX 固件镜像的数据记录数、总字节数、地址范围 `[min_addr, max_addr]`、EOF 与入口地址
pub fn summarizeIhex(text: []const u8): IhexStats {
    let stats = IhexStats {};
    let it = IhexIter.init(text);
    let has_data = false;

    while (chunk := it.next()) {
        stats.data_records += 1;
        const n = usize(chunk.record.byte_count);
        stats.total_bytes += n;
        if (n > 0) {
            const start_addr = chunk.abs_address;
            const end_addr = chunk.abs_address + u32(n - 1);
            if (!has_data) {
                stats.min_addr = start_addr;
                stats.max_addr = end_addr;
                has_data = true;
            } else {
                if (start_addr < stats.min_addr) stats.min_addr = start_addr;
                if (end_addr > stats.max_addr) stats.max_addr = end_addr;
            }
        }
    }

    stats.has_eof = it.has_eof && !it.had_error;
    stats.entry_addr = it.start_linear_address;
    return stats;
}
