// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8926 Geneve Tunneling Encapsulation Codec) | Created: 2026-11-22

/// Geneve 固定报头最小字节数（不含可变长度选项）
pub const GENEVE_MIN_HEADER_SIZE: usize = 8;

/// Geneve IANA 默认 UDP 端口
pub const GENEVE_DEFAULT_PORT: u16 = 6081;

/// Geneve 协议版本号（RFC 8926 唯一合法版本 0）
pub const GENEVE_VERSION: u8 = 0;

/// Geneve 24-bit VNI 最大值 (0x00FFFFFF)
pub const GENEVE_MAX_VNI: u32 = 0x00FFFFFF;

/// Geneve 选项区最大总字节数 (63 * 4 = 252 字节)
pub const GENEVE_MAX_OPTIONS_BYTES: usize = 252;

/// Geneve 单条选项数据最大字节数 (31 * 4 = 124 字节)
pub const GENEVE_MAX_SINGLE_OPT_DATA: usize = 124;

/// Geneve 报头标志：OAM 控制报文位 (bit 7)
pub const GENEVE_FLAG_OAM: u8 = 0x80;

/// Geneve 报头标志：包含关键选项位 C-flag (bit 6)
pub const GENEVE_FLAG_CRITICAL: u8 = 0x40;

/// Geneve 选项 Type 字段关键位 (bit 7)
pub const GENEVE_OPT_CRITICAL_BIT: u8 = 0x80;

/// 常用内层协议 EtherType：Transparent Ethernet Bridging (TEB)
pub const PROTO_ETHERNET: u16 = 0x6558;

/// 常用内层协议 EtherType：IPv4
pub const PROTO_IPV4: u16 = 0x0800;

/// 常用内层协议 EtherType：IPv6
pub const PROTO_IPV6: u16 = 0x86DD;

/// Geneve 8 字节固定报头
pub struct GeneveHeader {
    version: u8,
    opt_len_words: u8,
    flags: u8,
    protocol_type: u16,
    vni: u32,
    reserved: u8,

    /// 是否为 OAM 报文（bit 7）
    fn isOam(): bool {
        return (_.flags & GENEVE_FLAG_OAM) != 0;
    }

    /// 是否包含关键选项（bit 6, C-flag）
    fn hasCriticalOptions(): bool {
        return (_.flags & GENEVE_FLAG_CRITICAL) != 0;
    }

    /// 返回包含选项区在内的完整 Geneve 报头字节长度（8 + opt_len_words * 4）
    fn headerBytes(): usize {
        return GENEVE_MIN_HEADER_SIZE + ((_.opt_len_words as usize) * 4);
    }
}

/// Geneve 单条 4B 对齐 TLV 选项（零拷贝视图）
pub struct GeneveOption {
    opt_class: u16,
    opt_type: u8,
    len_words: u8,
    data: []const u8,

    /// 是否为关键选项（opt_type bit 7）
    fn isCritical(): bool {
        return (_.opt_type & GENEVE_OPT_CRITICAL_BIT) != 0;
    }

    /// 返回剥离关键位后的 7-bit 选项类型号
    fn typeWithoutCritical(): u8 {
        return _.opt_type & 0x7F;
    }
}

/// Geneve TLV 选项区零分配迭代器
pub struct GeneveOptionIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(options_slice: []const u8): GeneveOptionIter {
        return GeneveOptionIter {
            data: options_slice,
            pos: 0,
            had_error: false,
        };
    }

    /// 读取下一条 TLV 选项；严格校验长度不越界且选项头 3-bit 保留位为 0
    fn next(*_): GeneveOption? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 4) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const opt_class: u16 = ((_.data[_.pos] as u16) << 8) | (_.data[_.pos + 1] as u16);
        const opt_type: u8 = _.data[_.pos + 2];
        const len_byte: u8 = _.data[_.pos + 3];

        // 严格校验高 3 位保留位必须为 0
        if ((len_byte & 0xE0) != 0) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const len_words: u8 = len_byte & 0x1F;
        const data_len: usize = (len_words as usize) * 4;
        const val_start: usize = _.pos + 4;
        if (_.data.len - val_start < data_len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const val = _.data[val_start..val_start + data_len];
        _.pos = val_start + data_len;
        return GeneveOption {
            opt_class: opt_class,
            opt_type: opt_type,
            len_words: len_words,
            data: val,
        };
    }
}

/// Geneve 完整封装报文零拷贝视图
pub struct GenevePacket {
    header: GeneveHeader,
    options_slice: []const u8,
    payload: []const u8,

    /// 返回 TLV 选项区零分配迭代器
    fn options(): GeneveOptionIter {
        return GeneveOptionIter.init(_.options_slice);
    }

    /// 返回包含选项区在内的完整 Geneve 报头总字节数
    fn headerBytes(): usize {
        return _.header.headerBytes();
    }
}

/// 快速嗅探字节流是否满足最小 Geneve 报文头部格式（version == 0 且选项区不越界）
pub fn isGeneve(data: []const u8): bool {
    if (data.len < GENEVE_MIN_HEADER_SIZE) {
        return false;
    }
    const version: u8 = (data[0] >> 6) & 0x03;
    if (version != GENEVE_VERSION) {
        return false;
    }
    const opt_len_bytes: usize = ((data[0] & 0x3F) as usize) * 4;
    if (GENEVE_MIN_HEADER_SIZE + opt_len_bytes > data.len) {
        return false;
    }
    return true;
}

/// 解析单条 RFC 8926 Geneve 报文（零拷贝、严格校验 version == 0 与选项区边界）
pub fn parseGeneve(data: []const u8): GenevePacket? {
    if (data.len < GENEVE_MIN_HEADER_SIZE) {
        return null;
    }
    const b0: u8 = data[0];
    const version: u8 = (b0 >> 6) & 0x03;
    if (version != GENEVE_VERSION) {
        return null;
    }
    const opt_len_words: u8 = b0 & 0x3F;
    const opt_len_bytes: usize = (opt_len_words as usize) * 4;
    const hdr_total: usize = GENEVE_MIN_HEADER_SIZE + opt_len_bytes;
    if (hdr_total > data.len) {
        return null;
    }

    const flags: u8 = data[1];
    const proto_type: u16 = ((data[2] as u16) << 8) | (data[3] as u16);
    const vni: u32 = ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32);
    const reserved: u8 = data[7];

    return GenevePacket {
        header: GeneveHeader {
            version: version,
            opt_len_words: opt_len_words,
            flags: flags,
            protocol_type: proto_type,
            vni: vni,
            reserved: reserved,
        },
        options_slice: data[GENEVE_MIN_HEADER_SIZE..hdr_total],
        payload: data[hdr_total..data.len],
    };
}

/// Geneve 零分配报文编码器
pub struct GeneveWriter {
    buf: []u8,
    pos: usize,
    has_critical: bool,

    /// 初始化编码器并写入 8 字节基础报头；缓冲不足或 VNI 超过 24-bit 时返回 `null`
    static fn init(buf: []u8, proto_type: u16, vni: u32, is_oam: bool): GeneveWriter? {
        if (buf.len < GENEVE_MIN_HEADER_SIZE || vni > GENEVE_MAX_VNI) {
            return null;
        }
        buf[0] = 0; // version=0, opt_len_words=0 (回填)
        buf[1] = if (is_oam) { GENEVE_FLAG_OAM } else { 0 };
        buf[2] = ((proto_type >> 8) & 0xFF) as u8;
        buf[3] = (proto_type & 0xFF) as u8;
        buf[4] = ((vni >> 16) & 0xFF) as u8;
        buf[5] = ((vni >> 8) & 0xFF) as u8;
        buf[6] = (vni & 0xFF) as u8;
        buf[7] = 0; // reserved = 0

        return GeneveWriter {
            buf: buf,
            pos: GENEVE_MIN_HEADER_SIZE,
            has_critical: false,
        };
    }

    /// 追加一条 4B 对齐 TLV 选项；自动校验 4B 倍数、单选项 <= 124B、总选项 <= 252B，并在关键选项时置位 C-flag
    fn writeOption(*_, opt_class: u16, opt_type: u8, opt_data: []const u8): bool {
        if ((opt_data.len % 4) != 0 || opt_data.len > GENEVE_MAX_SINGLE_OPT_DATA) {
            return false;
        }
        const opt_total: usize = opt_data.len + 4;
        const cur_opt_bytes: usize = _.pos - GENEVE_MIN_HEADER_SIZE;
        if (cur_opt_bytes + opt_total > GENEVE_MAX_OPTIONS_BYTES) {
            return false;
        }
        if (_.buf.len - _.pos < opt_total) {
            return false;
        }

        const len_words: u8 = (opt_data.len / 4) as u8;
        _.buf[_.pos] = ((opt_class >> 8) & 0xFF) as u8;
        _.buf[_.pos + 1] = (opt_class & 0xFF) as u8;
        _.buf[_.pos + 2] = opt_type;
        _.buf[_.pos + 3] = len_words & 0x1F;
        _.pos += 4;

        let i: usize = 0;
        while (i < opt_data.len) {
            _.buf[_.pos + i] = opt_data[i];
            i += 1;
        }
        _.pos += opt_data.len;

        if ((opt_type & GENEVE_OPT_CRITICAL_BIT) != 0) {
            _.has_critical = true;
            _.buf[1] = _.buf[1] | GENEVE_FLAG_CRITICAL;
        }

        const total_opt_words: u8 = ((_.pos - GENEVE_MIN_HEADER_SIZE) / 4) as u8;
        _.buf[0] = total_opt_words & 0x3F;
        return true;
    }

    /// 写入内层载荷、自动回填 `opt_len_words` 与 C-flag，并返回完整编码切片；缓冲不足时返回 `null`
    fn finish(*_, inner_payload: []const u8): ([]const u8)? {
        if (_.buf.len - _.pos < inner_payload.len) {
            return null;
        }
        const total_opt_words: u8 = ((_.pos - GENEVE_MIN_HEADER_SIZE) / 4) as u8;
        _.buf[0] = total_opt_words & 0x3F;
        if (_.has_critical) {
            _.buf[1] = _.buf[1] | GENEVE_FLAG_CRITICAL;
        }

        let i: usize = 0;
        while (i < inner_payload.len) {
            _.buf[_.pos + i] = inner_payload[i];
            i += 1;
        }
        const total_len: usize = _.pos + inner_payload.len;
        _.pos = total_len;
        return _.buf[0..total_len];
    }
}
