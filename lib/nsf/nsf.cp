// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc NES Sound Format NSF & NSFe Parser)

/// 标准 NSF 文件魔数（5 字节："NESM\x1a"）
pub const NSF_MAGIC: []const u8 = "NESM\x1a";

/// 标准 NSF 文件头大小（128 字节）
pub const NSF_HEADER_SIZE: usize = 128;

/// NSFe 分块容器魔数（4 字节："NSFE"）
pub const NSFE_MAGIC: []const u8 = "NSFE";

/// NSF 扩展音源芯片位掩码
pub const NSF_CHIP_VRC6: u8 = 0x01;
pub const NSF_CHIP_VRC7: u8 = 0x02;
pub const NSF_CHIP_FDS: u8 = 0x04;
pub const NSF_CHIP_MMC5: u8 = 0x08;
pub const NSF_CHIP_NAMCO163: u8 = 0x10;
pub const NSF_CHIP_SUNSOFT5B: u8 = 0x20;

/// 播放制式枚举
pub enum NsfRegion: u8 {
    ntsc = 0,
    pal = 1,
    dual = 2,
    unknown = 255,
}

/// 容器格式类型枚举
pub enum NsfFormat: u8 {
    unknown = 0,
    nsf = 1,
    nsfe = 2,
}

/// 标准 128 字节 NSF 1.0/2.0 文件头（字符串与程序区零拷贝指向原始缓冲区）
pub struct NsfHeader {
    version: u8,
    total_songs: u8,
    starting_song: u8,
    load_addr: u16,
    init_addr: u16,
    play_addr: u16,
    song_name: []const u8,
    artist: []const u8,
    copyright: []const u8,
    ntsc_speed_us: u16,
    pal_speed_us: u16,
    bankswitch_init: [8]u8,
    region: NsfRegion,
    expansion_chips: u8,
    program_data: []const u8,

    /// 检测 8 个 bank 寄存器初值是否任一非 0（启用 Bankswitching）
    pub fn usesBankswitching(): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.bankswitch_init[i] != 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 检测是否启用指定扩展音源芯片位掩码
    pub fn hasChip(mask: u8): bool {
        return (_.expansion_chips & mask) != 0;
    }

    /// 返回 NTSC 播放帧率（Hz）
    pub fn ntscFps(): f64 {
        if (_.ntsc_speed_us == 0) {
            return 0.0;
        }
        return 1000000.0 / f64(_.ntsc_speed_us);
    }

    /// 返回 PAL 播放帧率（Hz）
    pub fn palFps(): f64 {
        if (_.pal_speed_us == 0) {
            return 0.0;
        }
        return 1000000.0 / f64(_.pal_speed_us);
    }
}

/// NSFe 单个分块视图（零拷贝负载切片）
pub struct NsfeChunk {
    chunk_id: [4]u8,
    payload: []const u8,
}

/// NSFe "INFO" 块核心元信息
pub struct NsfeInfo {
    load_addr: u16,
    init_addr: u16,
    play_addr: u16,
    region: NsfRegion,
    expansion_chips: u8,
    total_songs: u8,
    starting_song: u8,
}

// ---------------------------------------------------------------------------
// 内部小端序与字节辅助函数
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

fn trimNul(s: []const u8): []const u8 {
    let end: usize = 0;
    while (end < s.len && s[end] != 0) {
        end += 1;
    }
    return s[0..end];
}

fn decodeRegion(flags: u8): NsfRegion {
    if ((flags & 0x02) != 0) {
        return NsfRegion.dual;
    }
    if ((flags & 0x01) != 0) {
        return NsfRegion.pal;
    }
    return NsfRegion.ntsc;
}

// ---------------------------------------------------------------------------
// 1. 格式探测
// ---------------------------------------------------------------------------

/// 探测缓冲区是标准 NSF、NSFe 还是未知格式
pub fn detectNsfFormat(data: []const u8): NsfFormat {
    if (data.len >= NSF_HEADER_SIZE && eqlBytes(data[0..5], NSF_MAGIC)) {
        return NsfFormat.nsf;
    }
    if (data.len >= 4 && eqlBytes(data[0..4], NSFE_MAGIC)) {
        return NsfFormat.nsfe;
    }
    return NsfFormat.unknown;
}

/// 判断是否为标准 128 字节 NSF 头格式
pub fn isNsf(data: []const u8): bool {
    return detectNsfFormat(data) == NsfFormat.nsf;
}

/// 判断是否为 NSFe 分块容器格式
pub fn isNsfe(data: []const u8): bool {
    return detectNsfFormat(data) == NsfFormat.nsfe;
}

// ---------------------------------------------------------------------------
// 2. 128 字节 NSF 1.0/2.0 头解析
// ---------------------------------------------------------------------------

/// 零分配解析标准 128 字节 NSF 头并校验版本号、曲目索引与加载地址范围
pub fn parseNsfHeader(data: []const u8): NsfHeader? {
    if (!isNsf(data)) {
        return null;
    }
    const version = data[0x05];
    if (version != 1 && version != 2) {
        return null;
    }
    const total_songs = data[0x06];
    const starting_song = data[0x07];
    if (total_songs == 0 || starting_song == 0 || starting_song > total_songs) {
        return null;
    }

    const load_addr = readU16LE(data, 0x08);
    const init_addr = readU16LE(data, 0x0A);
    const play_addr = readU16LE(data, 0x0C);
    const expansion_chips = data[0x7B];

    const min_load: u16 = if ((expansion_chips & NSF_CHIP_FDS) != 0) 0x6000 else 0x8000;
    if (load_addr < min_load) {
        return null;
    }

    const song_name = trimNul(data[0x0E..0x2E]);
    const artist = trimNul(data[0x2E..0x4E]);
    const copyright = trimNul(data[0x4E..0x6E]);
    const ntsc_speed_us = readU16LE(data, 0x6E);
    const pal_speed_us = readU16LE(data, 0x78);

    let banks: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) {
        banks[i] = data[0x70 + i];
        i += 1;
    }

    return NsfHeader {
        version: version,
        total_songs: total_songs,
        starting_song: starting_song,
        load_addr: load_addr,
        init_addr: init_addr,
        play_addr: play_addr,
        song_name: song_name,
        artist: artist,
        copyright: copyright,
        ntsc_speed_us: ntsc_speed_us,
        pal_speed_us: pal_speed_us,
        bankswitch_init: banks,
        region: decodeRegion(data[0x7A]),
        expansion_chips: expansion_chips,
        program_data: data[NSF_HEADER_SIZE..data.len],
    };
}

// ---------------------------------------------------------------------------
// 3. NSFe 分块容器零分配迭代与 INFO 块解析
// ---------------------------------------------------------------------------

/// 零分配 NSFe 分块迭代器（`[4B LE size, 4B ASCII id, payload]` 直至 `"NEND"`）
pub struct NsfeChunkIter {
    data: []const u8,
    pos: usize,
    done: bool,
    malformed: bool,

    pub fn init(data: []const u8): NsfeChunkIter {
        if (!isNsfe(data)) {
            return NsfeChunkIter {
                data: data,
                pos: data.len,
                done: true,
                malformed: true,
            };
        }
        return NsfeChunkIter {
            data: data,
            pos: 4,
            done: false,
            malformed: false,
        };
    }

    pub fn next(*_): NsfeChunk? {
        if (_.done || _.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 8) {
            _.malformed = true;
            _.done = true;
            return null;
        }
        const chunk_size = readU32LE(_.data, _.pos);
        const id: [4]u8 = [
            _.data[_.pos + 4],
            _.data[_.pos + 5],
            _.data[_.pos + 6],
            _.data[_.pos + 7],
        ];
        const payload_start = _.pos + 8;
        if (u64(chunk_size) > u64(_.data.len - payload_start)) {
            _.malformed = true;
            _.done = true;
            return null;
        }
        const payload_end = payload_start + usize(chunk_size);
        _.pos = payload_end;

        if (id[0] == 'N' && id[1] == 'E' && id[2] == 'N' && id[3] == 'D') {
            _.done = true;
        }

        return NsfeChunk {
            chunk_id: id,
            payload: _.data[payload_start..payload_end],
        };
    }
}

/// 按 4 字节块 ID 零分配查找 NSFe 分块负载切片
pub fn findNsfeChunk(data: []const u8, id_4b: []const u8): ([]const u8)? {
    if (id_4b.len != 4) {
        return null;
    }
    let it = NsfeChunkIter.init(data);
    while (chunk := it.next()) {
        if (chunk.chunk_id[0] == id_4b[0] &&
            chunk.chunk_id[1] == id_4b[1] &&
            chunk.chunk_id[2] == id_4b[2] &&
            chunk.chunk_id[3] == id_4b[3])
        {
            return chunk.payload;
        }
    }
    return null;
}

/// 解析 NSFe `"INFO"` 块核心字段（兼容最小 8 字节与完整 10 字节格式）
pub fn parseNsfeInfo(info_payload: []const u8): NsfeInfo? {
    if (info_payload.len < 8) {
        return null;
    }
    const load_addr = readU16LE(info_payload, 0);
    const init_addr = readU16LE(info_payload, 2);
    const play_addr = readU16LE(info_payload, 4);
    const region = decodeRegion(info_payload[6]);
    const expansion_chips = info_payload[7];

    const min_load: u16 = if ((expansion_chips & NSF_CHIP_FDS) != 0) 0x6000 else 0x8000;
    if (load_addr < min_load) {
        return null;
    }

    let total_songs: u8 = 1;
    let starting_song: u8 = 0;
    if (info_payload.len >= 9) {
        total_songs = info_payload[8];
        if (total_songs == 0) {
            return null;
        }
    }
    if (info_payload.len >= 10) {
        starting_song = info_payload[9];
        if (starting_song >= total_songs) {
            return null;
        }
    }

    return NsfeInfo {
        load_addr: load_addr,
        init_addr: init_addr,
        play_addr: play_addr,
        region: region,
        expansion_chips: expansion_chips,
        total_songs: total_songs,
        starting_song: starting_song,
    };
}
