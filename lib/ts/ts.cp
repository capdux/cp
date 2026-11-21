// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO/IEC 13818-1 MPEG-TS Parser)

/// 标准 MPEG-TS 分组字节长度（188B）与 M2TS 分组长度（192B）
pub const TS_PACKET_SIZE: usize = 188;
pub const M2TS_PACKET_SIZE: usize = 192;

/// MPEG-TS 同步字节（0x47 / 'G'）
pub const TS_SYNC_BYTE: u8 = 0x47;

/// 标准保留 PID 常量
pub const TS_PID_PAT: u16 = 0x0000;
pub const TS_PID_CAT: u16 = 0x0001;
pub const TS_PID_NULL: u16 = 0x1FFF;

/// 常用音视频基本流类型常量（PMT stream_type）
pub const STREAM_TYPE_MPEG2_VIDEO: u8 = 0x02;
pub const STREAM_TYPE_AAC_ADTS: u8 = 0x0F;
pub const STREAM_TYPE_H264: u8 = 0x1B;
pub const STREAM_TYPE_HEVC: u8 = 0x24;
pub const STREAM_TYPE_AC3: u8 = 0x81;

/// 零拷贝 188 字节 MPEG-TS 分组解析结果
pub struct TsPacket {
    transport_error: bool,
    payload_unit_start: bool,
    transport_priority: bool,
    pid: u16,
    scrambling_control: u8,
    has_adaptation: bool,
    has_payload: bool,
    continuity_counter: u8,
    discontinuity: bool,
    random_access: bool,
    pcr_base: u64?,
    pcr_ext: u16?,
    payload: []const u8,

    /// 将 33-bit 90kHz PCR 基准时钟换算为毫秒（无 PCR 时返回 `null`）
    fn pcr_ms(): u64? {
        if (base := _.pcr_base) {
            return base / 90;
        }
        return null;
    }
}

/// 零堆分配 MPEG-TS / M2TS 分组流式迭代器
pub struct TsPacketIter {
    data: []const u8,
    pos: usize = 0,
    stride: usize = TS_PACKET_SIZE,
    pkt_off: usize = 0,

    static fn init(data: []const u8): TsPacketIter {
        if (isStandardTs(data)) {
            return TsPacketIter {
                data: data,
                pos: 0,
                stride: TS_PACKET_SIZE,
                pkt_off: 0,
            };
        }
        if (isM2ts(data)) {
            return TsPacketIter {
                data: data,
                pos: 0,
                stride: M2TS_PACKET_SIZE,
                pkt_off: 4,
            };
        }
        return TsPacketIter {
            data: data,
            pos: 0,
            stride: TS_PACKET_SIZE,
            pkt_off: 0,
        };
    }

    /// 解析下一个 188B TS 分组；遇到流尾、截断或非法同步字节时安全返回 `null`
    fn next(*_): TsPacket? {
        if (_.data.len - _.pos < _.stride) {
            _.pos = _.data.len;
            return null;
        }
        const start = _.pos + _.pkt_off;
        const pkt_slice = _.data[start .. start + TS_PACKET_SIZE];
        _.pos += _.stride;
        return parseTsPacket(pkt_slice);
    }
}

/// PAT 节目映射表条目（节目号 -> PMT PID）
pub struct PatEntry {
    program_number: u16,
    pmt_pid: u16,
}

/// 零堆分配 PAT（Program Association Table，`table_id == 0x00`）条目迭代器
pub struct PatIter {
    data: []const u8,
    pos: usize = 0,
    end: usize = 0,
    valid: bool = false,

    /// 从带 1B `pointer_field` 的 TS 负载初始化 PAT 迭代器；表头非法或越界时返回空迭代器
    static fn init(payload: []const u8): PatIter {
        if (payload.len < 13) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }
        const ptr: usize = payload[0] as usize;
        const sec_start: usize = ptr + 1;
        if (payload.len - sec_start < 12) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }
        if (payload[sec_start] != 0x00) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }

        const sec_len: usize = (((payload[sec_start + 1] & 0x0F) as usize) << 8)
            | (payload[sec_start + 2] as usize);
        if (sec_len < 9 || sec_len > 1021) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }

        const sec_end: usize = sec_start + 3 + sec_len;
        if (sec_end > payload.len) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }

        const entries_start: usize = sec_start + 8;
        const entries_end: usize = sec_end - 4;
        if (entries_start > entries_end) {
            return PatIter { data: payload, pos: 0, end: 0, valid: false };
        }

        return PatIter {
            data: payload,
            pos: entries_start,
            end: entries_end,
            valid: true,
        };
    }

    /// 返回下一条 `PatEntry`；迭代完毕或遇到不足 4 字节尾数据时返回 `null`
    fn next(*_): PatEntry? {
        if (!_.valid || _.end - _.pos < 4) return null;
        const prog_num = readU16BE(_.data, _.pos);
        const pmt_pid: u16 = (((_.data[_.pos + 2] & 0x1F) as u16) << 8)
            | (_.data[_.pos + 3] as u16);
        _.pos += 4;
        return PatEntry {
            program_number: prog_num,
            pmt_pid: pmt_pid,
        };
    }
}

/// PMT 基本流描述条目
pub struct PmtStream {
    stream_type: u8,
    elementary_pid: u16,
    es_info_len: u16,
}

/// 零堆分配 PMT 基本流列表迭代器
pub struct PmtStreamIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(streams_data: []const u8): PmtStreamIter {
        return PmtStreamIter {
            data: streams_data,
            pos: 0,
        };
    }

    /// 读取下一条基本流定义；到达末尾或 `es_info_len` 越界时安全返回 `null`
    fn next(*_): PmtStream? {
        if (_.data.len - _.pos < 5) {
            _.pos = _.data.len;
            return null;
        }
        const stream_type = _.data[_.pos];
        const elem_pid: u16 = (((_.data[_.pos + 1] & 0x1F) as u16) << 8)
            | (_.data[_.pos + 2] as u16);
        const es_info_len: u16 = (((_.data[_.pos + 3] & 0x0F) as u16) << 8)
            | (_.data[_.pos + 4] as u16);

        const step: usize = (es_info_len as usize) + 5;
        if (step > _.data.len - _.pos) {
            _.pos = _.data.len;
            return null;
        }
        _.pos += step;
        return PmtStream {
            stream_type: stream_type,
            elementary_pid: elem_pid,
            es_info_len: es_info_len,
        };
    }
}

/// 解析后的 PMT（Program Map Table，`table_id == 0x02`）头部与基本流切片视图
pub struct PmtHeader {
    program_number: u16,
    version_number: u8,
    current_next: bool,
    pcr_pid: u16,
    program_info_len: u16,
    streams_data: []const u8,

    /// 创建零拷贝基本流迭代器
    fn streams(): PmtStreamIter {
        return PmtStreamIter.init(_.streams_data);
    }
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn isStandardTs(data: []const u8): bool {
    if (data.len < TS_PACKET_SIZE) return false;
    if (data[0] != TS_SYNC_BYTE) return false;
    if (data.len >= TS_PACKET_SIZE * 2) {
        return data[TS_PACKET_SIZE] == TS_SYNC_BYTE;
    }
    return true;
}

fn isM2ts(data: []const u8): bool {
    if (data.len < M2TS_PACKET_SIZE) return false;
    if (data[4] != TS_SYNC_BYTE) return false;
    if (data.len >= M2TS_PACKET_SIZE * 2) {
        return data[M2TS_PACKET_SIZE + 4] == TS_SYNC_BYTE;
    }
    return true;
}

/// 探测缓冲区是否为标准 188B MPEG-TS 或 192B M2TS 流（含双包同步字校验）
pub fn isMpegTs(data: []const u8): bool {
    return isStandardTs(data) || isM2ts(data);
}

/// 解析单个 188 字节 MPEG-TS 分组（含 Adaptation Field、33-bit PCR 与零拷贝 payload）
pub fn parseTsPacket(pkt: []const u8): TsPacket? {
    if (pkt.len < TS_PACKET_SIZE) return null;
    if (pkt[0] != TS_SYNC_BYTE) return null;

    const b1 = pkt[1];
    const b2 = pkt[2];
    const b3 = pkt[3];

    const transport_error: bool = (b1 & 0x80) != 0;
    const payload_unit_start: bool = (b1 & 0x40) != 0;
    const transport_priority: bool = (b1 & 0x20) != 0;
    const pid: u16 = (((b1 & 0x1F) as u16) << 8) | (b2 as u16);

    const scrambling_control: u8 = (b3 >> 6) & 0x03;
    const has_adaptation: bool = (b3 & 0x20) != 0;
    const has_payload: bool = (b3 & 0x10) != 0;
    const continuity_counter: u8 = b3 & 0x0F;

    let discontinuity: bool = false;
    let random_access: bool = false;
    let pcr_base: u64? = null;
    let pcr_ext: u16? = null;
    let payload_off: usize = 4;

    if (has_adaptation) {
        const adapt_len: usize = pkt[4] as usize;
        // 4B header + 1B length + adapt_len <= 188 => adapt_len <= 183
        if (adapt_len > 183) return null;
        if (adapt_len >= 1) {
            const flags = pkt[5];
            discontinuity = (flags & 0x80) != 0;
            random_access = (flags & 0x40) != 0;
            const has_pcr: bool = (flags & 0x10) != 0;
            if (has_pcr) {
                if (adapt_len < 7) return null;
                const p0: u64 = pkt[6] as u64;
                const p1: u64 = pkt[7] as u64;
                const p2: u64 = pkt[8] as u64;
                const p3: u64 = pkt[9] as u64;
                const p4: u64 = pkt[10] as u64;
                const p5: u16 = pkt[11] as u16;
                pcr_base = (p0 << 25) | (p1 << 17) | (p2 << 9) | (p3 << 1) | ((p4 >> 7) & 0x01);
                pcr_ext = (((pkt[10] & 0x01) as u16) << 8) | p5;
            }
        }
        payload_off = adapt_len + 5;
    }

    const payload = if (has_payload) pkt[payload_off..TS_PACKET_SIZE] else pkt[TS_PACKET_SIZE..TS_PACKET_SIZE];

    return TsPacket {
        transport_error: transport_error,
        payload_unit_start: payload_unit_start,
        transport_priority: transport_priority,
        pid: pid,
        scrambling_control: scrambling_control,
        has_adaptation: has_adaptation,
        has_payload: has_payload,
        continuity_counter: continuity_counter,
        discontinuity: discontinuity,
        random_access: random_access,
        pcr_base: pcr_base,
        pcr_ext: pcr_ext,
        payload: payload,
    };
}

/// 解析带 1B `pointer_field` 的 PMT 负载，返回 `PmtHeader` 与基本流迭代器视图
pub fn parsePmt(payload: []const u8): PmtHeader? {
    if (payload.len < 17) return null;

    const ptr: usize = payload[0] as usize;
    const sec_start: usize = ptr + 1;
    if (payload.len - sec_start < 16) return null;
    if (payload[sec_start] != 0x02) return null;

    const sec_len: usize = (((payload[sec_start + 1] & 0x0F) as usize) << 8)
        | (payload[sec_start + 2] as usize);
    if (sec_len < 13 || sec_len > 1021) return null;

    const sec_end: usize = sec_start + 3 + sec_len;
    if (sec_end > payload.len) return null;

    const program_number = readU16BE(payload, sec_start + 3);
    const ver_byte = payload[sec_start + 5];
    const version_number: u8 = (ver_byte >> 1) & 0x1F;
    const current_next: bool = (ver_byte & 0x01) != 0;

    const pcr_pid: u16 = (((payload[sec_start + 8] & 0x1F) as u16) << 8)
        | (payload[sec_start + 9] as u16);
    const program_info_len: u16 = (((payload[sec_start + 10] & 0x0F) as u16) << 8)
        | (payload[sec_start + 11] as u16);

    const streams_start: usize = sec_start + 12 + (program_info_len as usize);
    const streams_end: usize = sec_end - 4;
    if (streams_start > streams_end) return null;

    return PmtHeader {
        program_number: program_number,
        version_number: version_number,
        current_next: current_next,
        pcr_pid: pcr_pid,
        program_info_len: program_info_len,
        streams_data: payload[streams_start..streams_end],
    };
}
