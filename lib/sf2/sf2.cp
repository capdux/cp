// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc SoundFont 2.01/2.04 RIFF sfbk Parser)

/// SoundFont 2.0 Hydra 记录定长常量（字节）
pub const SF2_PRESET_HEADER_SIZE: usize = 38;
pub const SF2_INST_HEADER_SIZE: usize = 22;
pub const SF2_SAMPLE_HEADER_SIZE: usize = 46;
pub const SF2_BAG_SIZE: usize = 4;
pub const SF2_GEN_SIZE: usize = 4;

/// SoundFont 规范版本 (`ifil`)
pub struct Sf2Version {
    major: u16 = 0,
    minor: u16 = 0,
}

/// `LIST/INFO` 零拷贝元数据视图（自动剥离末尾 `\0`）
pub struct Sf2Info {
    version: Sf2Version = Sf2Version {},
    sound_engine: []const u8 = "",
    bank_name: []const u8 = "",
    rom_name: []const u8 = "",
    creation_date: []const u8 = "",
    author: []const u8 = "",
    product: []const u8 = "",
    copyright: []const u8 = "",
    comment: []const u8 = "",
    software: []const u8 = "",
}

/// SoundFont 顶层 `INFO` / `sdta` / `pdta` 零拷贝分段视图
pub struct Sf2Sections {
    info: Sf2Info = Sf2Info {},
    smpl_pcm16: []const u8 = "",
    sm24_lsb: []const u8 = "",
    phdr: []const u8 = "",
    pbag: []const u8 = "",
    inst: []const u8 = "",
    ibag: []const u8 = "",
    shdr: []const u8 = "",
}

/// `phdr` 预设头条目（38 字节）
pub struct Sf2Preset {
    name: []const u8,
    preset: u16,
    bank: u16,
    preset_bag_idx: u16,
}

/// `inst` 乐器头条目（22 字节）
pub struct Sf2Instrument {
    name: []const u8,
    inst_bag_idx: u16,
}

/// `shdr` 采样头条目（46 字节）
pub struct Sf2Sample {
    name: []const u8,
    start: u32,
    end: u32,
    start_loop: u32,
    end_loop: u32,
    sample_rate: u32,
    original_pitch: u8,
    pitch_correction: i8,
    sample_link: u16,
    sample_type: u16,

    /// 零拷贝返回该采样在 `sections.smpl_pcm16` 中对应的 16-bit PCM 字节区（越界或 `start > end` 返回 `null`）
    pub fn samplePcmSlice(sections: Sf2Sections): ([]const u8)? {
        if (_.start > _.end) return null;
        const max_samples = sections.smpl_pcm16.len / 2;
        if (usize(_.end) > max_samples) return null;
        return sections.smpl_pcm16[usize(_.start) * 2 .. usize(_.end) * 2];
    }
}

fn fourccEq(data: []const u8, off: usize, a: u8, b: u8, c: u8, d: u8): bool {
    return data[off] == a && data[off + 1] == b && data[off + 2] == c && data[off + 3] == d;
}

fn readU16Le(data: []const u8, off: usize): u16 {
    return u16(data[off]) | (u16(data[off + 1]) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return u32(data[off]) |
        (u32(data[off + 1]) << 8) |
        (u32(data[off + 2]) << 16) |
        (u32(data[off + 3]) << 24);
}

fn trimZstr(raw: []const u8): []const u8 {
    let i: usize = 0;
    while (i < raw.len && raw[i] != 0) {
        i += 1;
    }
    return raw[0..i];
}

/// 校验 12 字节 RIFF `sfbk` 头且 `8 + size <= data.len`
pub fn isSf2(data: []const u8): bool {
    if (data.len < 12) return false;
    if (!fourccEq(data, 0, 'R', 'I', 'F', 'F')) return false;
    if (!fourccEq(data, 8, 's', 'f', 'b', 'k')) return false;
    const riff_size = usize(readU32Le(data, 4));
    if (riff_size < 4) return false;
    return riff_size <= data.len - 8;
}

fn parseInfoList(body: []const u8, info: *Sf2Info): bool {
    let pos: usize = 0;
    while (pos < body.len) {
        if (8 > body.len - pos) return false;
        const csize = usize(readU32Le(body, pos + 4));
        const dstart = pos + 8;
        if (csize > body.len - dstart) return false;
        const payload = body[dstart .. dstart + csize];

        if (fourccEq(body, pos, 'i', 'f', 'i', 'l')) {
            if (payload.len >= 4) {
                info.version = Sf2Version {
                    major: readU16Le(payload, 0),
                    minor: readU16Le(payload, 2),
                };
            }
        } else if (fourccEq(body, pos, 'i', 's', 'n', 'g')) {
            info.sound_engine = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'N', 'A', 'M')) {
            info.bank_name = trimZstr(payload);
        } else if (fourccEq(body, pos, 'i', 'r', 'o', 'm')) {
            info.rom_name = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'C', 'R', 'D')) {
            info.creation_date = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'E', 'N', 'G')) {
            info.author = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'P', 'R', 'D')) {
            info.product = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'C', 'O', 'P')) {
            info.copyright = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'C', 'M', 'T')) {
            info.comment = trimZstr(payload);
        } else if (fourccEq(body, pos, 'I', 'S', 'F', 'T')) {
            info.software = trimZstr(payload);
        }

        const pad: usize = csize & 1;
        pos = dstart + csize;
        if (pad != 0 && pos < body.len) {
            pos += 1;
        }
    }
    return true;
}

fn parseSdtaList(body: []const u8, sec: *Sf2Sections): bool {
    let pos: usize = 0;
    while (pos < body.len) {
        if (8 > body.len - pos) return false;
        const csize = usize(readU32Le(body, pos + 4));
        const dstart = pos + 8;
        if (csize > body.len - dstart) return false;
        const payload = body[dstart .. dstart + csize];

        if (fourccEq(body, pos, 's', 'm', 'p', 'l')) {
            sec.smpl_pcm16 = payload;
        } else if (fourccEq(body, pos, 's', 'm', '2', '4')) {
            sec.sm24_lsb = payload;
        }

        const pad: usize = csize & 1;
        pos = dstart + csize;
        if (pad != 0 && pos < body.len) {
            pos += 1;
        }
    }
    return true;
}

fn parsePdtaList(body: []const u8, sec: *Sf2Sections): bool {
    let pos: usize = 0;
    while (pos < body.len) {
        if (8 > body.len - pos) return false;
        const csize = usize(readU32Le(body, pos + 4));
        const dstart = pos + 8;
        if (csize > body.len - dstart) return false;
        const payload = body[dstart .. dstart + csize];

        if (fourccEq(body, pos, 'p', 'h', 'd', 'r')) {
            if (payload.len % SF2_PRESET_HEADER_SIZE != 0) return false;
            sec.phdr = payload;
        } else if (fourccEq(body, pos, 'p', 'b', 'a', 'g')) {
            if (payload.len % SF2_BAG_SIZE != 0) return false;
            sec.pbag = payload;
        } else if (fourccEq(body, pos, 'i', 'n', 's', 't')) {
            if (payload.len % SF2_INST_HEADER_SIZE != 0) return false;
            sec.inst = payload;
        } else if (fourccEq(body, pos, 'i', 'b', 'a', 'g')) {
            if (payload.len % SF2_BAG_SIZE != 0) return false;
            sec.ibag = payload;
        } else if (fourccEq(body, pos, 's', 'h', 'd', 'r')) {
            if (payload.len % SF2_SAMPLE_HEADER_SIZE != 0) return false;
            sec.shdr = payload;
        }

        const pad: usize = csize & 1;
        pos = dstart + csize;
        if (pad != 0 && pos < body.len) {
            pos += 1;
        }
    }
    return true;
}

/// 解析 SoundFont 2.0 顶层 `LIST/INFO`、`LIST/sdta`、`LIST/pdta` 分段（非 `sfbk` 或块长异常返回 `null`）
pub fn parseSf2(data: []const u8): Sf2Sections? {
    if (!isSf2(data)) return null;

    const riff_end = 8 + usize(readU32Le(data, 4));
    let sec = Sf2Sections {};
    let pos: usize = 12;

    while (pos < riff_end) {
        if (8 > riff_end - pos) return null;
        const csize = usize(readU32Le(data, pos + 4));
        const dstart = pos + 8;
        if (csize > riff_end - dstart) return null;

        if (fourccEq(data, pos, 'L', 'I', 'S', 'T')) {
            if (csize < 4) return null;
            const list_body = data[dstart + 4 .. dstart + csize];
            if (fourccEq(data, dstart, 'I', 'N', 'F', 'O')) {
                if (!parseInfoList(list_body, *sec.info)) return null;
            } else if (fourccEq(data, dstart, 's', 'd', 't', 'a')) {
                if (!parseSdtaList(list_body, *sec)) return null;
            } else if (fourccEq(data, dstart, 'p', 'd', 't', 'a')) {
                if (!parsePdtaList(list_body, *sec)) return null;
            }
        }

        const pad: usize = csize & 1;
        pos = dstart + csize;
        if (pad != 0 && pos < riff_end) {
            pos += 1;
        }
    }

    return sec;
}

/// 零堆分配 `phdr`（38B）预设迭代器（自动剔除末尾 `EOP` 哨兵条目）
pub struct Sf2PresetIter {
    phdr: []const u8,
    idx: usize = 0,
    count: usize,

    pub static fn init(phdr: []const u8): Sf2PresetIter {
        const total = phdr.len / SF2_PRESET_HEADER_SIZE;
        const valid = if (total > 0) total - 1 else 0;
        return Sf2PresetIter {
            phdr: phdr,
            idx: 0,
            count: valid,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn next(*_): Sf2Preset? {
        if (_.idx >= _.count) return null;
        const off = _.idx * SF2_PRESET_HEADER_SIZE;
        _.idx += 1;
        return Sf2Preset {
            name: trimZstr(_.phdr[off .. off + 20]),
            preset: readU16Le(_.phdr, off + 20),
            bank: readU16Le(_.phdr, off + 22),
            preset_bag_idx: readU16Le(_.phdr, off + 24),
        };
    }
}

/// 零堆分配 `inst`（22B）乐器迭代器（自动剔除末尾 `EOI` 哨兵条目）
pub struct Sf2InstIter {
    inst: []const u8,
    idx: usize = 0,
    count: usize,

    pub static fn init(inst: []const u8): Sf2InstIter {
        const total = inst.len / SF2_INST_HEADER_SIZE;
        const valid = if (total > 0) total - 1 else 0;
        return Sf2InstIter {
            inst: inst,
            idx: 0,
            count: valid,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn next(*_): Sf2Instrument? {
        if (_.idx >= _.count) return null;
        const off = _.idx * SF2_INST_HEADER_SIZE;
        _.idx += 1;
        return Sf2Instrument {
            name: trimZstr(_.inst[off .. off + 20]),
            inst_bag_idx: readU16Le(_.inst, off + 20),
        };
    }
}

/// 零堆分配 `shdr`（46B）采样头迭代器（自动剔除末尾 `EOS` 哨兵条目）
pub struct Sf2SampleIter {
    shdr: []const u8,
    idx: usize = 0,
    count: usize,

    pub static fn init(shdr: []const u8): Sf2SampleIter {
        const total = shdr.len / SF2_SAMPLE_HEADER_SIZE;
        const valid = if (total > 0) total - 1 else 0;
        return Sf2SampleIter {
            shdr: shdr,
            idx: 0,
            count: valid,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn next(*_): Sf2Sample? {
        if (_.idx >= _.count) return null;
        const off = _.idx * SF2_SAMPLE_HEADER_SIZE;
        _.idx += 1;
        return Sf2Sample {
            name: trimZstr(_.shdr[off .. off + 20]),
            start: readU32Le(_.shdr, off + 20),
            end: readU32Le(_.shdr, off + 24),
            start_loop: readU32Le(_.shdr, off + 28),
            end_loop: readU32Le(_.shdr, off + 32),
            sample_rate: readU32Le(_.shdr, off + 36),
            original_pitch: _.shdr[off + 40],
            pitch_correction: _.shdr[off + 41] as i8,
            sample_link: readU16Le(_.shdr, off + 42),
            sample_type: readU16Le(_.shdr, off + 44),
        };
    }
}
