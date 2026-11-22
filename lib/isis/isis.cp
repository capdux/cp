// Origin: Pure-CP Stdlib (Zero-Alloc ISO/IEC 10589 / RFC 1195 IS-IS Parser) | Created: 2026-11-22

/// IS-IS Intradomain Routing Protocol Discriminator (NLPID = 0x83)
pub const ISIS_NLPID: u8 = 0x83;

/// IS-IS 公共报头固定长度（8 字节）
pub const ISIS_COMMON_HEADER_SIZE: usize = 8;

/// IS-IS 协议版本号 (1)
pub const ISIS_VERSION: u8 = 1;

/// IS-IS LSP 完整固定报头长度（8B 公共头 + 19B LSP 固定体 = 27 字节，默认 6B System ID）
pub const ISIS_LSP_HEADER_SIZE: usize = 27;

// ── PDU Type 常量（公共报头字节 4 低 5 位）──
pub const PDU_L1_LAN_IIH: u8 = 15;
pub const PDU_L2_LAN_IIH: u8 = 16;
pub const PDU_P2P_IIH: u8 = 17;
pub const PDU_L1_LSP: u8 = 18;
pub const PDU_L2_LSP: u8 = 20;
pub const PDU_L1_CSNP: u8 = 24;
pub const PDU_L2_CSNP: u8 = 25;
pub const PDU_L1_PSNP: u8 = 26;
pub const PDU_L2_PSNP: u8 = 27;

// ── 常用 TLV Code 常量 ──
pub const TLV_AREA_ADDR: u8 = 1;
pub const TLV_IS_NEIGHBORS: u8 = 2;
pub const TLV_PADDING: u8 = 8;
pub const TLV_AUTH: u8 = 10;
pub const TLV_EXT_IS_REACH: u8 = 22;
pub const TLV_PROTOCOLS: u8 = 129;
pub const TLV_IP_INTF_ADDR: u8 = 132;
pub const TLV_EXT_IP_REACH: u8 = 135;
pub const TLV_HOSTNAME: u8 = 137;

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

/// ISO 8473 / RFC 1008 Fletcher-16 校验和累加（mod 255，返回 `(c1 << 8) | c0`）
/// 对已填入合法校验和的 LSP 切片（从 LSP ID 偏移 12 至 PDU 末尾）调用将精确返回 `0`。
pub fn isisFletcher16(data: []const u8): u16 {
    let c0: u32 = 0;
    let c1: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        c0 = (c0 + (data[i] as u32)) % 255;
        c1 = (c1 + c0) % 255;
        i += 1;
    }
    return (((c1 & 0xFF) as u16) << 8) | ((c0 & 0xFF) as u16);
}

/// 按 ISO 8473 / ISO 10589 公式计算 LSP 两字节校验和
/// `lsp_from_id` 为从 LSP ID（原报文偏移 12）开始至 PDU 末尾的切片，校验和槽位位于 `lsp_from_id[12..14]`。
pub fn isisLspChecksum(lsp_from_id: []const u8): u16 {
    if (lsp_from_id.len < 15) {
        return 0;
    }
    const cksum_off: usize = 12;
    let c0: i64 = 0;
    let c1: i64 = 0;
    let i: usize = 0;
    while (i < lsp_from_id.len) {
        let b: i64 = 0;
        if (i != cksum_off && i != cksum_off + 1) {
            b = lsp_from_id[i] as i64;
        }
        c0 = (c0 + b) % 255;
        c1 = (c1 + c0) % 255;
        i += 1;
    }
    const n_len: i64 = lsp_from_id.len as i64;
    const k: i64 = cksum_off as i64;

    let x: i64 = ((n_len - k - 1) * c0 - c1) % 255;
    if (x <= 0) {
        x += 255;
    }
    let y: i64 = (c1 - (n_len - k) * c0) % 255;
    if (y <= 0) {
        y += 255;
    }
    return (((x as u16) & 0xFF) << 8) | ((y as u16) & 0xFF);
}

/// 验证 LSP 切片（从偏移 12 LSP ID 至 PDU 末尾）的 ISO 8473 Fletcher-16 校验和
pub fn verifyIsisLspChecksum(lsp_from_id: []const u8): bool {
    if (lsp_from_id.len < 15) {
        return false;
    }
    return isisFletcher16(lsp_from_id) == 0;
}

/// IS-IS 8 字节公共报头
pub struct IsisCommonHeader {
    nlpid: u8,
    len_indicator: u8,
    version_ext: u8,
    id_len: u8,
    pdu_type: u8,
    version: u8,
    reserved: u8,
    max_area_addrs: u8,

    /// 返回实际 System ID 字节长度（0 表示默认 6 字节，1..8 表示原值）
    fn systemIdLen(): u8 {
        if (_.id_len == 0) {
            return 6;
        }
        return _.id_len;
    }
}

/// IS-IS LSP 固定体解析结果（ISO/IEC 10589 §9.8 / §9.9）
pub struct IsisLspHeader {
    pdu_len: u16,
    remaining_lifetime: u16,
    lsp_id: [8]u8,
    seq_num: u32,
    checksum: u16,
    type_block: u8,

    /// Partition Repair 位 (bit 7)
    fn isPartitionRepair(): bool {
        return (_.type_block & 0x80) != 0;
    }

    /// LSP Database Overload 位 (bit 2, OL)
    fn isOverload(): bool {
        return (_.type_block & 0x04) != 0;
    }

    /// IS Type（低 2 位：1 = Level 1, 3 = Level 1/2）
    fn isType(): u8 {
        return _.type_block & 0x03;
    }
}

/// 单条 IS-IS TLV 零拷贝视图：`[1B type, 1B length, length B value]`
pub struct IsisTlv {
    tlv_type: u8,
    length: u8,
    value: []const u8,
}

/// IS-IS TLV 区零分配迭代器
pub struct IsisTlvIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(tlv_bytes: []const u8): IsisTlvIter {
        return IsisTlvIter {
            data: tlv_bytes,
            pos: 0,
            had_error: false,
        };
    }

    /// 零拷贝读取下一条 TLV；遇到截断时置 `had_error = true` 并终止
    fn next(*_): IsisTlv? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 2) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const tlv_type: u8 = _.data[_.pos];
        const length: u8 = _.data[_.pos + 1];
        const val_start: usize = _.pos + 2;
        const val_len: usize = length as usize;
        if (_.data.len - val_start < val_len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + val_len];
        _.pos = val_start + val_len;
        return IsisTlv {
            tlv_type: tlv_type,
            length: length,
            value: val,
        };
    }
}

/// IS-IS 完整报文零拷贝视图
pub struct IsisPacket {
    common: IsisCommonHeader,
    fixed_body: []const u8,
    tlv_bytes: []const u8,
    raw_len: usize,

    /// 返回 TLV 零分配迭代器
    fn tlvs(): IsisTlvIter {
        return IsisTlvIter.init(_.tlv_bytes);
    }

    /// 按 TLV Code 查找第一条匹配的 TLV
    fn findTlv(code: u8): IsisTlv? {
        let it = _.tlvs();
        while (tlv := it.next()) {
            if (tlv.tlv_type == code) {
                return tlv;
            }
        }
        return null;
    }

    /// 便捷提取 Dynamic Hostname (RFC 5301, TLV 137)
    fn hostname(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_HOSTNAME)) {
            return tlv.value;
        }
        return null;
    }
}

/// 快速嗅探字节流是否满足 IS-IS 公共报头与长度约束
pub fn isIsis(data: []const u8): bool {
    if (data.len < ISIS_COMMON_HEADER_SIZE) {
        return false;
    }
    if (data[0] != ISIS_NLPID) {
        return false;
    }
    const len_ind: usize = data[1] as usize;
    if (len_ind < ISIS_COMMON_HEADER_SIZE || len_ind > data.len) {
        return false;
    }
    if (data[2] != ISIS_VERSION || data[5] != ISIS_VERSION || data[6] != 0) {
        return false;
    }
    if (data[3] > 8) {
        return false;
    }
    return true;
}

/// 零拷贝解析 IS-IS 公共报头、固定体与 TLV 区
pub fn parseIsis(data: []const u8): IsisPacket? {
    if (!isIsis(data)) {
        return null;
    }
    const len_ind: usize = data[1] as usize;
    const pdu_type: u8 = data[4] & 0x1F;

    return IsisPacket {
        common: IsisCommonHeader {
            nlpid: data[0],
            len_indicator: data[1],
            version_ext: data[2],
            id_len: data[3],
            pdu_type: pdu_type,
            version: data[5],
            reserved: data[6],
            max_area_addrs: data[7],
        },
        fixed_body: data[ISIS_COMMON_HEADER_SIZE..len_ind],
        tlv_bytes: data[len_ind..data.len],
        raw_len: data.len,
    };
}

/// 从已解析的 `IsisPacket` 提取 L1/L2 LSP 固定报头（严格校验 PDU Type 18/20、`len_indicator == 27`、`27 <= pdu_len <= raw_len`）
pub fn parseIsisLsp(pkt: IsisPacket): IsisLspHeader? {
    if (pkt.common.pdu_type != PDU_L1_LSP && pkt.common.pdu_type != PDU_L2_LSP) {
        return null;
    }
    if ((pkt.common.len_indicator as usize) != ISIS_LSP_HEADER_SIZE || pkt.fixed_body.len < 19) {
        return null;
    }
    const fb = pkt.fixed_body;
    const pdu_len: u16 = readBeU16(fb, 0);
    if ((pdu_len as usize) < ISIS_LSP_HEADER_SIZE || (pdu_len as usize) > pkt.raw_len) {
        return null;
    }
    const lsp_id: [8]u8 = [
        fb[4], fb[5], fb[6], fb[7],
        fb[8], fb[9], fb[10], fb[11],
    ];
    return IsisLspHeader {
        pdu_len: pdu_len,
        remaining_lifetime: readBeU16(fb, 2),
        lsp_id: lsp_id,
        seq_num: readBeU32(fb, 12),
        checksum: readBeU16(fb, 16),
        type_block: fb[18],
    };
}
