// Origin: Pure-CP Stdlib (Zero-Alloc VGM 1.00-1.71 Chiptune & GD3 Parser) | Created: 2026-11-22

/// VGM 文件头魔数（4 字节）
pub const VGM_MAGIC: []const u8 = "Vgm ";

/// GD3 标签块魔数（4 字节）
pub const VGM_GD3_MAGIC: []const u8 = "Gd3 ";

/// VGM 最小头长度（64 字节）
pub const VGM_MIN_HEADER_SIZE: usize = 64;

/// VGM 固定采样率（44100 Hz）
pub const VGM_SAMPLE_RATE: u32 = 44100;

/// 常用 VGM 命令字节常量
pub const CMD_WAIT_N: u8 = 0x61;
pub const CMD_WAIT_60HZ: u8 = 0x62;
pub const CMD_WAIT_50HZ: u8 = 0x63;
pub const CMD_END: u8 = 0x66;
pub const CMD_DATA_BLOCK: u8 = 0x67;
pub const CMD_WAIT_SHORT_START: u8 = 0x70;

/// VGM 1.00–1.71 文件头解析结果
pub struct VgmHeader {
    eof_offset: u32,
    version_bcd: u32,
    sn76489_clock: u32,
    ym2413_clock: u32,
    ym2612_clock: u32,
    ym2151_clock: u32,
    gd3_abs_offset: usize?,
    total_samples: u32,
    loop_abs_offset: usize?,
    loop_samples: u32,
    rate_hz: u32,
    data_offset: usize,

    /// 解码 BCD 主版本号（例如 0x00000171 -> 1）
    pub fn versionMajor(): u32 {
        const d0 = (_.version_bcd >> 8) & 0x0F;
        const d1 = (_.version_bcd >> 12) & 0x0F;
        return d1 * 10 + d0;
    }

    /// 解码 BCD 次版本号（例如 0x00000171 -> 71）
    pub fn versionMinor(): u32 {
        const d0 = _.version_bcd & 0x0F;
        const d1 = (_.version_bcd >> 4) & 0x0F;
        return d1 * 10 + d0;
    }

    /// 返回总时长（毫秒）
    pub fn durationMs(): f64 {
        return f64(_.total_samples) * 1000.0 / 44100.0;
    }
}

/// GD3 标签块解析结果（所有字段为零拷贝 UTF-16LE 字节切片，不含结尾 \x00\x00）
pub struct VgmGd3 {
    version: u32,
    track_name_en: []const u8,
    track_name_jp: []const u8,
    game_name_en: []const u8,
    game_name_jp: []const u8,
    system_name_en: []const u8,
    system_name_jp: []const u8,
    author_en: []const u8,
    author_jp: []const u8,
    release_date: []const u8,
    vgm_creator: []const u8,
    notes: []const u8,
}

/// VGM 命令流单遍扫描统计结果
pub struct VgmCmdStats {
    command_count: u32,
    computed_wait_samples: u64,
    data_block_count: u32,
    has_end: bool,
}

// ---------------------------------------------------------------------------
// 内部小端序读取与字节比较辅助函数
// ---------------------------------------------------------------------------

fn readU16LE(data: []const u8, offset: usize): u16 {
    const lo = u16(data[offset]);
    const hi = u16(data[offset + 1]);
    return lo | (hi << 8);
}

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

// ---------------------------------------------------------------------------
// 1. 格式识别与文件头解析
// ---------------------------------------------------------------------------

/// 校验数据长度 >= 64 且首 4 字节为 `"Vgm "`
pub fn isVgm(data: []const u8): bool {
    if (data.len < VGM_MIN_HEADER_SIZE) {
        return false;
    }
    return eqlBytes(data[0..4], VGM_MAGIC);
}

/// 零分配解析 VGM 1.00–1.71 文件头，严格校验相对偏移与数据区边界
pub fn parseVgmHeader(data: []const u8): VgmHeader? {
    if (!isVgm(data)) {
        return null;
    }
    const eof_offset = readU32LE(data, 0x04);
    const version_bcd = readU32LE(data, 0x08);
    const sn76489_clock = readU32LE(data, 0x0C);
    const ym2413_clock = readU32LE(data, 0x10);
    const gd3_rel = readU32LE(data, 0x14);
    const total_samples = readU32LE(data, 0x18);
    const loop_rel = readU32LE(data, 0x1C);
    const loop_samples = readU32LE(data, 0x20);
    const rate_hz = readU32LE(data, 0x24);
    const ym2612_clock = readU32LE(data, 0x2C);
    const ym2151_clock = readU32LE(data, 0x30);

    let gd3_abs: usize? = null;
    if (gd3_rel != 0) {
        const abs_u64 = u64(0x14) + u64(gd3_rel);
        if (abs_u64 > u64(data.len)) {
            return null;
        }
        gd3_abs = usize(abs_u64);
    }

    let loop_abs: usize? = null;
    if (loop_rel != 0) {
        const abs_u64 = u64(0x1C) + u64(loop_rel);
        if (abs_u64 > u64(data.len)) {
            return null;
        }
        loop_abs = usize(abs_u64);
    }

    let data_offset: usize = 0x40;
    if (version_bcd >= 0x00000150) {
        const data_rel = readU32LE(data, 0x34);
        if (data_rel != 0) {
            const abs_u64 = u64(0x34) + u64(data_rel);
            if (abs_u64 < 0x40 || abs_u64 > u64(data.len)) {
                return null;
            }
            data_offset = usize(abs_u64);
        }
    }
    if (data_offset < 0x40 || data_offset > data.len) {
        return null;
    }

    return VgmHeader {
        eof_offset: eof_offset,
        version_bcd: version_bcd,
        sn76489_clock: sn76489_clock,
        ym2413_clock: ym2413_clock,
        ym2612_clock: ym2612_clock,
        ym2151_clock: ym2151_clock,
        gd3_abs_offset: gd3_abs,
        total_samples: total_samples,
        loop_abs_offset: loop_abs,
        loop_samples: loop_samples,
        rate_hz: rate_hz,
        data_offset: data_offset,
    };
}

// ---------------------------------------------------------------------------
// 2. GD3 UTF-16LE 标签块解析与零分配 UTF-8 解码
// ---------------------------------------------------------------------------

struct Utf16Field {
    slice: []const u8,
    next_pos: usize,
}

fn nextUtf16LeZ(buf: []const u8, pos: usize): Utf16Field {
    let i = pos;
    while (i + 2 <= buf.len) {
        if (buf[i] == 0 && buf[i + 1] == 0) {
            return Utf16Field {
                slice: buf[pos..i],
                next_pos: i + 2,
            };
        }
        i += 2;
    }
    return Utf16Field {
        slice: buf[pos..buf.len],
        next_pos: buf.len,
    };
}

/// 零分配解析 GD3 标签块，切分 11 个以 `\x00\x00` 结尾的 UTF-16LE 字段
pub fn parseVgmGd3(data: []const u8, header: VgmHeader): VgmGd3? {
    const off = header.gd3_abs_offset ?? return null;
    if (off + 12 > data.len) {
        return null;
    }
    if (!eqlBytes(data[off .. off + 4], VGM_GD3_MAGIC)) {
        return null;
    }
    const ver = readU32LE(data, off + 4);
    const len = readU32LE(data, off + 8);
    if (u64(len) > u64(data.len - (off + 12))) {
        return null;
    }
    const body = data[off + 12 .. off + 12 + usize(len)];

    const f0 = nextUtf16LeZ(body, 0);
    const f1 = nextUtf16LeZ(body, f0.next_pos);
    const f2 = nextUtf16LeZ(body, f1.next_pos);
    const f3 = nextUtf16LeZ(body, f2.next_pos);
    const f4 = nextUtf16LeZ(body, f3.next_pos);
    const f5 = nextUtf16LeZ(body, f4.next_pos);
    const f6 = nextUtf16LeZ(body, f5.next_pos);
    const f7 = nextUtf16LeZ(body, f6.next_pos);
    const f8 = nextUtf16LeZ(body, f7.next_pos);
    const f9 = nextUtf16LeZ(body, f8.next_pos);
    const f10 = nextUtf16LeZ(body, f9.next_pos);

    return VgmGd3 {
        version: ver,
        track_name_en: f0.slice,
        track_name_jp: f1.slice,
        game_name_en: f2.slice,
        game_name_jp: f3.slice,
        system_name_en: f4.slice,
        system_name_jp: f5.slice,
        author_en: f6.slice,
        author_jp: f7.slice,
        release_date: f8.slice,
        vgm_creator: f9.slice,
        notes: f10.slice,
    };
}

/// 零堆分配将 UTF-16LE 字节切片解码为 UTF-8/ASCII 写入 `out`，溢出返回 `null`
pub fn decodeUtf16LeToUtf8(utf16le: []const u8, out: []u8): usize? {
    let in_pos: usize = 0;
    let out_pos: usize = 0;
    while (in_pos + 2 <= utf16le.len) {
        const unit0 = readU16LE(utf16le, in_pos);
        in_pos += 2;
        let cp = u32(unit0);
        if (unit0 >= 0xD800 && unit0 <= 0xDBFF && in_pos + 2 <= utf16le.len) {
            const unit1 = readU16LE(utf16le, in_pos);
            if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                in_pos += 2;
                cp = ((u32(unit0 - 0xD800) << 10) | u32(unit1 - 0xDC00)) + 0x10000;
            }
        }
        if (cp < 0x80) {
            if (out_pos + 1 > out.len) return null;
            out[out_pos] = cp as u8;
            out_pos += 1;
        } else if (cp < 0x800) {
            if (out_pos + 2 > out.len) return null;
            out[out_pos] = (0xC0 | ((cp >> 6) & 0x1F)) as u8;
            out[out_pos + 1] = (0x80 | (cp & 0x3F)) as u8;
            out_pos += 2;
        } else if (cp < 0x10000) {
            if (out_pos + 3 > out.len) return null;
            out[out_pos] = (0xE0 | ((cp >> 12) & 0x0F)) as u8;
            out[out_pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[out_pos + 2] = (0x80 | (cp & 0x3F)) as u8;
            out_pos += 3;
        } else {
            if (out_pos + 4 > out.len) return null;
            out[out_pos] = (0xF0 | ((cp >> 18) & 0x07)) as u8;
            out[out_pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
            out[out_pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[out_pos + 3] = (0x80 | (cp & 0x3F)) as u8;
            out_pos += 4;
        }
    }
    return out_pos;
}

// ---------------------------------------------------------------------------
// 3. 命令流单遍扫描与统计
// ---------------------------------------------------------------------------

/// 单遍扫描 VGM 命令区直至 `0x66` (`CMD_END`)，支持跳过 `0x67` Data Block、
/// 2/3/4/5 字节芯片写命令与所有等待命令，累计命令数与采样等待总数
pub fn scanVgmCommands(data: []const u8, header: VgmHeader): VgmCmdStats {
    let stats = VgmCmdStats {
        command_count: 0,
        computed_wait_samples: 0,
        data_block_count: 0,
        has_end: false,
    };

    let limit = data.len;
    if (gd3_off := header.gd3_abs_offset) {
        if (gd3_off >= header.data_offset && gd3_off < limit) {
            limit = gd3_off;
        }
    }

    let pos = header.data_offset;
    while (pos < limit) {
        const op = data[pos];

        if (op == CMD_END) {
            stats.command_count += 1;
            stats.has_end = true;
            break;
        }
        if (op == CMD_WAIT_N) {
            if (pos + 3 > limit) break;
            const wait_n = readU16LE(data, pos + 1);
            stats.computed_wait_samples += u64(wait_n);
            stats.command_count += 1;
            pos += 3;
            continue;
        }
        if (op == CMD_WAIT_60HZ) {
            stats.computed_wait_samples += 735;
            stats.command_count += 1;
            pos += 1;
            continue;
        }
        if (op == CMD_WAIT_50HZ) {
            stats.computed_wait_samples += 882;
            stats.command_count += 1;
            pos += 1;
            continue;
        }
        if (op == CMD_DATA_BLOCK) {
            // 0x67 0x66 <type:1B> <size:4B LE> <data:size B>
            if (pos + 7 > limit || data[pos + 1] != 0x66) {
                break;
            }
            const blk_size = readU32LE(data, pos + 3);
            if (u64(blk_size) > u64(limit - (pos + 7))) {
                break;
            }
            stats.data_block_count += 1;
            stats.command_count += 1;
            pos += 7 + usize(blk_size);
            continue;
        }
        if (op == 0x68) {
            // PCM RAM write (12 bytes)
            if (pos + 12 > limit) break;
            stats.command_count += 1;
            pos += 12;
            continue;
        }
        if (op >= 0x70 && op <= 0x7F) {
            stats.computed_wait_samples += u64(op & 0x0F) + 1;
            stats.command_count += 1;
            pos += 1;
            continue;
        }
        if (op >= 0x80 && op <= 0x8F) {
            stats.computed_wait_samples += u64(op & 0x0F);
            stats.command_count += 1;
            pos += 1;
            continue;
        }
        if (op == 0x4F || op == 0x50 || (op >= 0x30 && op <= 0x3F)) {
            if (pos + 2 > limit) break;
            stats.command_count += 1;
            pos += 2;
            continue;
        }
        if ((op >= 0x40 && op <= 0x4E) || (op >= 0x51 && op <= 0x5F) || (op >= 0xA0 && op <= 0xBF)) {
            if (pos + 3 > limit) break;
            stats.command_count += 1;
            pos += 3;
            continue;
        }
        if (op >= 0xC0 && op <= 0xDF) {
            if (pos + 4 > limit) break;
            stats.command_count += 1;
            pos += 4;
            continue;
        }
        if (op == 0xE0 || op == 0xE1) {
            if (pos + 5 > limit) break;
            stats.command_count += 1;
            pos += 5;
            continue;
        }
        if (op == 0x90 || op == 0x91 || op == 0x95) {
            if (pos + 5 > limit) break;
            stats.command_count += 1;
            pos += 5;
            continue;
        }
        if (op == 0x92) {
            if (pos + 6 > limit) break;
            stats.command_count += 1;
            pos += 6;
            continue;
        }
        if (op == 0x93) {
            if (pos + 11 > limit) break;
            stats.command_count += 1;
            pos += 11;
            continue;
        }
        if (op == 0x94) {
            if (pos + 2 > limit) break;
            stats.command_count += 1;
            pos += 2;
            continue;
        }

        // 未知命令字节：安全停止
        break;
    }

    return stats;
}
