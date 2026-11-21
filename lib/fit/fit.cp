// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Garmin/ANT+ FIT Protocol 1.0/2.0 Binary Telemetry Parser)

/// FIT 文件头魔数（偏移 8..12）
pub const FIT_MAGIC: []const u8 = ".FIT";
pub const FIT_HEADER_SIZE_12: u8 = 12;
pub const FIT_HEADER_SIZE_14: u8 = 14;
pub const FIT_MAX_LOCAL_MESGS: usize = 16;
pub const FIT_MAX_FIELDS: usize = 64;

/// 常见 FIT Global Message Numbers
pub const FIT_MESG_FILE_ID: u16 = 0;
pub const FIT_MESG_SESSION: u16 = 18;
pub const FIT_MESG_LAP: u16 = 19;
pub const FIT_MESG_RECORD: u16 = 20;
pub const FIT_MESG_EVENT: u16 = 21;
pub const FIT_MESG_DEVICE_INFO: u16 = 23;
pub const FIT_MESG_ACTIVITY: u16 = 34;

/// FIT 12/14 字节文件头
pub struct FitHeader {
    header_size: u8,
    protocol_version: u8,
    profile_version: u16,
    data_size: u32,
    header_crc: u16,
}

/// FIT 字段定义（3 字节）
pub struct FitFieldDef {
    field_num: u8,
    size: u8,
    base_type: u8,
}

/// FIT 记录帧类型
pub enum FitRecordKind: u8 {
    definition = 0,
    data = 1,
}

/// 单条 FIT Definition / Data Message 零拷贝帧视图
pub struct FitRecord {
    kind: FitRecordKind,
    local_mesg_type: u8,
    global_mesg_num: u16,
    is_big_endian: bool,
    field_count: u8,
    payload: []const u8,
    time_offset: u8? = null,
}

/// FIT 文件统计汇总
pub struct FitStats {
    definition_count: usize = 0,
    data_count: usize = 0,
    record_mesg_count: usize = 0,
    file_id_count: usize = 0,
    crc_valid: bool = false,
}

struct FitLocalSlot {
    defined: bool = false,
    global_mesg_num: u16 = 0,
    is_big_endian: bool = false,
    field_count: u8 = 0,
    data_byte_len: usize = 0,
}

const FIT_CRC_TABLE: [16]u16 = [
    0x0000, 0xCC01, 0xD801, 0x1400,
    0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401,
    0x5000, 0x9C01, 0x8801, 0x4400,
];

fn readU16Le(data: []const u8, off: usize): u16 {
    return u16(data[off]) | (u16(data[off + 1]) << 8);
}

fn readU16Be(data: []const u8, off: usize): u16 {
    return (u16(data[off]) << 8) | u16(data[off + 1]);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return u32(data[off]) |
        (u32(data[off + 1]) << 8) |
        (u32(data[off + 2]) << 16) |
        (u32(data[off + 3]) << 24);
}

/// Garmin / ANT+ FIT 专用 CRC-16（半字节查表法，多项式 `0xA001`）
pub fn fitCrc16(data: []const u8): u16 {
    let crc: u16 = 0;
    let i: usize = 0;
    while (i < data.len) {
        const b = data[i];
        // 低 4 位
        let tmp = FIT_CRC_TABLE[usize(crc & 0x0F)];
        crc = (crc >> 4) & 0x0FFF;
        crc = crc ^ tmp ^ FIT_CRC_TABLE[usize(b & 0x0F)];
        // 高 4 位
        tmp = FIT_CRC_TABLE[usize(crc & 0x0F)];
        crc = (crc >> 4) & 0x0FFF;
        crc = crc ^ tmp ^ FIT_CRC_TABLE[usize((b >> 4) & 0x0F)];
        i += 1;
    }
    return crc;
}

/// 快速魔数与基础头长判定
pub fn isFit(data: []const u8): bool {
    if (data.len < 14) return false;
    const hdr_size = data[0];
    if (hdr_size != FIT_HEADER_SIZE_12 && hdr_size != FIT_HEADER_SIZE_14) return false;
    return data[8] == '.' && data[9] == 'F' && data[10] == 'I' && data[11] == 'T';
}

/// 解析并校验 FIT 12/14 字节文件头（14B 头且 `header_crc != 0` 时自动校验 `fitCrc16(data[0..12])`）
pub fn parseFitHeader(data: []const u8): FitHeader? {
    if (!isFit(data)) return null;

    const header_size = data[0];
    const hlen = usize(header_size);
    if (data.len < hlen + 2) return null;

    const protocol_version = data[1];
    const profile_version = readU16Le(data, 2);
    const data_size = readU32Le(data, 4);

    const dlen = usize(data_size);
    if (dlen > data.len - hlen - 2) return null;

    let header_crc: u16 = 0;
    if (header_size == FIT_HEADER_SIZE_14) {
        header_crc = readU16Le(data, 12);
        if (header_crc != 0) {
            if (fitCrc16(data[0..12]) != header_crc) {
                return null;
            }
        }
    }

    return FitHeader {
        header_size: header_size,
        protocol_version: protocol_version,
        profile_version: profile_version,
        data_size: data_size,
        header_crc: header_crc,
    };
}

/// 校验 FIT 文件尾 2 字节 LE CRC-16 是否等于 `fitCrc16(data[0 .. header_size + data_size])`
pub fn verifyFileCrc(data: []const u8): bool {
    const hdr = parseFitHeader(data) ?? return false;
    const body_end = usize(hdr.header_size) + usize(hdr.data_size);
    const expected = readU16Le(data, body_end);
    return fitCrc16(data[0..body_end]) == expected;
}

/// 零堆分配 FIT Definition / Data Message 帧流迭代器
pub struct FitRecordIter {
    data: []const u8,
    pos: usize,
    end: usize,
    slots: [16]FitLocalSlot,
    had_error: bool = false,

    pub static fn init(data: []const u8): FitRecordIter? {
        const hdr = parseFitHeader(data) ?? return null;
        const start = usize(hdr.header_size);
        const end = start + usize(hdr.data_size);
        let slots: [16]FitLocalSlot = undefined;
        let i: usize = 0;
        while (i < FIT_MAX_LOCAL_MESGS) {
            slots[i] = FitLocalSlot {};
            i += 1;
        }
        return FitRecordIter {
            data: data,
            pos: start,
            end: end,
            slots: slots,
            had_error: false,
        };
    }

    pub fn next(*_): FitRecord? {
        if (_.had_error || _.pos >= _.end) return null;

        const rec_hdr = _.data[_.pos];
        _.pos += 1;

        // 1. Compressed Timestamp Data Message (bit 7 == 1)
        if ((rec_hdr & 0x80) != 0) {
            const local_type: u8 = (rec_hdr >> 5) & 0x03;
            const time_off: u8 = rec_hdr & 0x1F;
            const slot = _.slots[usize(local_type)];
            if (!slot.defined) {
                _.had_error = true;
                return null;
            }
            if (slot.data_byte_len > _.end - _.pos) {
                _.had_error = true;
                return null;
            }
            const payload = _.data[_.pos .. _.pos + slot.data_byte_len];
            _.pos += slot.data_byte_len;
            return FitRecord {
                kind: FitRecordKind.data,
                local_mesg_type: local_type,
                global_mesg_num: slot.global_mesg_num,
                is_big_endian: slot.is_big_endian,
                field_count: slot.field_count,
                payload: payload,
                time_offset: time_off,
            };
        }

        const local_type: u8 = rec_hdr & 0x0F;

        // 2. Definition Message (bit 6 == 1)
        if ((rec_hdr & 0x40) != 0) {
            const has_dev_data = (rec_hdr & 0x20) != 0;
            const body_start = _.pos;
            if (5 > _.end - _.pos) {
                _.had_error = true;
                return null;
            }

            // Byte 0: reserved, Byte 1: architecture (0=LE, 1=BE)
            const arch = _.data[_.pos + 1];
            if (arch > 1) {
                _.had_error = true;
                return null;
            }
            const is_be = arch == 1;
            const global_num = if (is_be) readU16Be(_.data, _.pos + 2) else readU16Le(_.data, _.pos + 2);
            const num_fields = _.data[_.pos + 4];
            _.pos += 5;

            const fields_bytes = usize(num_fields) * 3;
            if (fields_bytes > _.end - _.pos) {
                _.had_error = true;
                return null;
            }

            let data_len: usize = 0;
            let fi: usize = 0;
            while (fi < usize(num_fields)) {
                const fsize = _.data[_.pos + fi * 3 + 1];
                data_len += usize(fsize);
                fi += 1;
            }
            _.pos += fields_bytes;

            // Developer Data Fields (FIT 2.0, bit 5 == 1)
            if (has_dev_data) {
                if (_.pos >= _.end) {
                    _.had_error = true;
                    return null;
                }
                const num_dev_fields = _.data[_.pos];
                _.pos += 1;
                const dev_bytes = usize(num_dev_fields) * 3;
                if (dev_bytes > _.end - _.pos) {
                    _.had_error = true;
                    return null;
                }
                let di: usize = 0;
                while (di < usize(num_dev_fields)) {
                    const dfsize = _.data[_.pos + di * 3 + 1];
                    data_len += usize(dfsize);
                    di += 1;
                }
                _.pos += dev_bytes;
            }

            _.slots[usize(local_type)] = FitLocalSlot {
                defined: true,
                global_mesg_num: global_num,
                is_big_endian: is_be,
                field_count: num_fields,
                data_byte_len: data_len,
            };

            return FitRecord {
                kind: FitRecordKind.definition,
                local_mesg_type: local_type,
                global_mesg_num: global_num,
                is_big_endian: is_be,
                field_count: num_fields,
                payload: _.data[body_start.._.pos],
                time_offset: null,
            };
        }

        // 3. Normal Data Message (bits 7..6 == 00)
        const slot = _.slots[usize(local_type)];
        if (!slot.defined) {
            _.had_error = true;
            return null;
        }
        if (slot.data_byte_len > _.end - _.pos) {
            _.had_error = true;
            return null;
        }
        const payload = _.data[_.pos .. _.pos + slot.data_byte_len];
        _.pos += slot.data_byte_len;
        return FitRecord {
            kind: FitRecordKind.data,
            local_mesg_type: local_type,
            global_mesg_num: slot.global_mesg_num,
            is_big_endian: slot.is_big_endian,
            field_count: slot.field_count,
            payload: payload,
            time_offset: null,
        };
    }
}

/// 统计 FIT 文件的 Definition/Data 消息数、`record` (20) 与 `file_id` (0) 数量及文件 CRC 校验结果
pub fn summarizeFit(data: []const u8): FitStats {
    let stats = FitStats {};
    if (parseFitHeader(data) == null) return stats;

    stats.crc_valid = verifyFileCrc(data);
    let it = FitRecordIter.init(data) ?? return stats;
    while (rec := it.next()) {
        if (rec.kind == FitRecordKind.definition) {
            stats.definition_count += 1;
        } else {
            stats.data_count += 1;
            if (rec.global_mesg_num == FIT_MESG_RECORD) {
                stats.record_mesg_count += 1;
            } else if (rec.global_mesg_num == FIT_MESG_FILE_ID) {
                stats.file_id_count += 1;
            }
        }
    }
    return stats;
}
