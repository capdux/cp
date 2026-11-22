// Origin: Pure-CP Stdlib (Zero-Alloc Amazon Mobipocket/Kindle .mobi Parser) | Created: 2026-11-22

/// PalmDB / MOBI / EXTH 格式常量
pub const PDB_HEADER_SIZE: usize = 78;
pub const PDB_RECORD_ENTRY_SIZE: usize = 8;
pub const MOBI_TYPE_SIG: []const u8 = "BOOK";
pub const MOBI_CREATOR_SIG: []const u8 = "MOBI";
pub const MOBI_MAGIC: []const u8 = "MOBI";
pub const EXTH_MAGIC: []const u8 = "EXTH";

/// 常用 EXTH 元数据记录类型 ID
pub const EXTH_AUTHOR: u32 = 100;
pub const EXTH_PUBLISHER: u32 = 101;
pub const EXTH_DESCRIPTION: u32 = 103;
pub const EXTH_ISBN: u32 = 104;
pub const EXTH_SUBJECT: u32 = 105;
pub const EXTH_PUB_DATE: u32 = 106;
pub const EXTH_ASIN: u32 = 113;
pub const EXTH_UPDATED_TITLE: u32 = 503;
pub const EXTH_LANGUAGE: u32 = 524;

/// PalmDOC / MOBI 压缩算法枚举
pub enum MobiCompression: u16 {
    unknown = 0,
    none = 1,
    palmdoc = 2,
    huff_cdic = 17480,
}

/// MOBI 文本编码枚举
pub enum MobiEncoding: u32 {
    unknown = 0,
    cp1252 = 1252,
    utf8 = 65001,
}

pub fn decodeMobiCompression(code: u16): MobiCompression {
    if (code == 1) return MobiCompression.none;
    if (code == 2) return MobiCompression.palmdoc;
    if (code == 17480) return MobiCompression.huff_cdic;
    return MobiCompression.unknown;
}

pub fn decodeMobiEncoding(code: u32): MobiEncoding {
    if (code == 1252) return MobiEncoding.cp1252;
    if (code == 65001) return MobiEncoding.utf8;
    return MobiEncoding.unknown;
}

/// 78 字节 PalmDB 容器头解析结果
pub struct PdbHeader {
    name: []const u8,
    attributes: u16,
    version: u16,
    creation_time: u32,
    modification_time: u32,
    type_id: [4]u8,
    creator_id: [4]u8,
    record_count: u16,

    /// 获取第 `idx` 个 PalmDB 记录在文件中的起始偏移
    fn recordOffset(data: []const u8, idx: usize): usize? {
        if (idx >= (_.record_count as usize)) return null;
        const entry_off = PDB_HEADER_SIZE + idx * PDB_RECORD_ENTRY_SIZE;
        if (entry_off > data.len || data.len - entry_off < PDB_RECORD_ENTRY_SIZE) return null;
        const off = readU32BE(data, entry_off) as usize;
        if (off > data.len) return null;
        return off;
    }

    /// 零拷贝提取第 `idx` 个 PalmDB 记录切片（严格校验相邻记录偏移单调递增）
    fn recordSlice(data: []const u8, idx: usize): ([]const u8)? {
        let start: usize = 0;
        if (s := _.recordOffset(data, idx)) {
            start = s;
        } else {
            return null;
        }

        let end: usize = data.len;
        if (idx + 1 < (_.record_count as usize)) {
            if (e := _.recordOffset(data, idx + 1)) {
                if (e < start) return null;
                end = e;
            } else {
                return null;
            }
        }
        return data[start..end];
    }
}

/// Record 0 中的 PalmDOC + MOBI 头部解析结果
pub struct MobiHeader {
    compression: MobiCompression,
    text_length: u32,
    text_record_count: u16,
    record_size: u16,
    encryption_type: u16,
    header_len: u32,
    mobi_type: u32,
    text_encoding: MobiEncoding,
    unique_id: u32,
    file_version: u32,
    full_name: []const u8,
    locale: u32,
    exth_flags: u32,
    has_exth: bool,
    exth_offset: usize,
    rec0_end: usize,
}

/// 单条 EXTH 元数据记录零拷贝视图
pub struct ExthRecord {
    record_type: u32,
    data: []const u8,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimCString(raw: []const u8): []const u8 {
    let i: usize = 0;
    while (i < raw.len && raw[i] != 0) {
        i += 1;
    }
    return raw[0..i];
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

/// 校验字节切片是否为合法 MOBI (`"BOOKMOBI"`) 或 PalmDOC (`"TEXtREAd"`) 文件
pub fn isMobi(data: []const u8): bool {
    if (data.len < PDB_HEADER_SIZE + PDB_RECORD_ENTRY_SIZE) return false;
    const sig = data[60..68];
    return strEq(sig, "BOOKMOBI") || strEq(sig, "TEXtREAd");
}

/// 解析 78 字节 PalmDB 头
pub fn parsePdbHeader(data: []const u8): PdbHeader? {
    if (data.len < PDB_HEADER_SIZE) return null;

    let type_id: [4]u8 = [data[60], data[61], data[62], data[63]];
    let creator_id: [4]u8 = [data[64], data[65], data[66], data[67]];
    const record_count = readU16BE(data, 76);

    const table_end: usize = PDB_HEADER_SIZE + (record_count as usize) * PDB_RECORD_ENTRY_SIZE;
    if (table_end > data.len) return null;

    return PdbHeader {
        name: trimCString(data[0..32]),
        attributes: readU16BE(data, 32),
        version: readU16BE(data, 34),
        creation_time: readU32BE(data, 36),
        modification_time: readU32BE(data, 40),
        type_id: type_id,
        creator_id: creator_id,
        record_count: record_count,
    };
}

/// 解析 Record 0 中的 PalmDOC 头 + MOBI 头（提取编码、全名与 EXTH 偏移）
pub fn parseMobiHeader(data: []const u8): MobiHeader? {
    if (!isMobi(data)) return null;

    let pdb = PdbHeader {
        name: "",
        attributes: 0,
        version: 0,
        creation_time: 0,
        modification_time: 0,
        type_id: [0, 0, 0, 0],
        creator_id: [0, 0, 0, 0],
        record_count: 0,
    };
    if (p := parsePdbHeader(data)) {
        pdb = p;
    } else {
        return null;
    }
    if (pdb.record_count == 0) return null;

    let rec0_start: usize = 0;
    if (s := pdb.recordOffset(data, 0)) {
        rec0_start = s;
    } else {
        return null;
    }

    let rec0: []const u8 = "";
    if (r := pdb.recordSlice(data, 0)) {
        rec0 = r;
    } else {
        return null;
    }

    // PalmDOC (16B) + minimal MOBI header up to exth_flags (116B) = 132B
    if (rec0.len < 132) return null;
    if (!strEq(rec0[16..20], MOBI_MAGIC)) return null;

    const header_len = readU32BE(rec0, 20);
    if (header_len < 116) return null;
    if ((header_len as u64) > ((rec0.len - 16) as u64)) return null;

    const full_name_off = readU32BE(rec0, 84) as u64;
    const full_name_len = readU32BE(rec0, 88) as u64;
    let full_name: []const u8 = "";
    if (full_name_len > 0) {
        const rec0_len_u64 = rec0.len as u64;
        if (full_name_off > rec0_len_u64 || full_name_len > rec0_len_u64 - full_name_off) {
            return null;
        }
        const fn_start = full_name_off as usize;
        const fn_end = (full_name_off + full_name_len) as usize;
        full_name = rec0[fn_start..fn_end];
    }

    const exth_flags = readU32BE(rec0, 128);
    const has_exth = (exth_flags & 0x40) != 0;
    const exth_offset = rec0_start + 16 + (header_len as usize);

    return MobiHeader {
        compression: decodeMobiCompression(readU16BE(rec0, 0)),
        text_length: readU32BE(rec0, 4),
        text_record_count: readU16BE(rec0, 8),
        record_size: readU16BE(rec0, 10),
        encryption_type: readU16BE(rec0, 12),
        header_len: header_len,
        mobi_type: readU32BE(rec0, 24),
        text_encoding: decodeMobiEncoding(readU32BE(rec0, 28)),
        unique_id: readU32BE(rec0, 32),
        file_version: readU32BE(rec0, 36),
        full_name: full_name,
        locale: readU32BE(rec0, 92),
        exth_flags: exth_flags,
        has_exth: has_exth,
        exth_offset: exth_offset,
        rec0_end: rec0_start + rec0.len,
    };
}

/// 零堆分配 EXTH 元数据记录迭代器
pub struct ExthIter {
    data: []const u8,
    pos: usize,
    limit: usize,
    index: u32 = 0,
    count: u32,
    had_error: bool = false,

    static fn init(data: []const u8, header: MobiHeader): ExthIter {
        if (!header.has_exth) {
            return ExthIter {
                data: data,
                pos: 0,
                limit: 0,
                index: 0,
                count: 0,
                had_error: false,
            };
        }

        const off = header.exth_offset;
        const rec_end = if (header.rec0_end <= data.len) header.rec0_end else data.len;
        if (off > rec_end || rec_end - off < 12) {
            return ExthIter {
                data: data,
                pos: 0,
                limit: 0,
                index: 0,
                count: 0,
                had_error: true,
            };
        }
        if (!strEq(data[off .. off + 4], EXTH_MAGIC)) {
            return ExthIter {
                data: data,
                pos: 0,
                limit: 0,
                index: 0,
                count: 0,
                had_error: true,
            };
        }

        const exth_len = readU32BE(data, off + 4);
        const rec_count = readU32BE(data, off + 8);
        if (exth_len < 12 || (exth_len as u64) > ((rec_end - off) as u64)) {
            return ExthIter {
                data: data,
                pos: 0,
                limit: 0,
                index: 0,
                count: 0,
                had_error: true,
            };
        }

        return ExthIter {
            data: data,
            pos: off + 12,
            limit: off + (exth_len as usize),
            index: 0,
            count: rec_count,
            had_error: false,
        };
    }

    fn next(*_): ExthRecord? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.limit - _.pos < 8) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const rtype = readU32BE(_.data, off);
        const total_len = readU32BE(_.data, off + 4);
        if (total_len < 8 || (total_len as u64) > ((_.limit - off) as u64)) {
            _.had_error = true;
            return null;
        }

        const total_usize = total_len as usize;
        const payload = _.data[off + 8 .. off + total_usize];
        _.pos = off + total_usize;
        _.index += 1;

        return ExthRecord {
            record_type: rtype,
            data: payload,
        };
    }
}

/// 在 EXTH 元数据区按 `record_type` 查找首个匹配记录的零拷贝切片
pub fn findExthRecord(data: []const u8, header: MobiHeader, record_type: u32): ([]const u8)? {
    let it = ExthIter.init(data, header);
    while (rec := it.next()) {
        if (rec.record_type == record_type) return rec.data;
    }
    return null;
}
