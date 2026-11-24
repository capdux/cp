// Origin: Pure-CP Stdlib (Zero-Alloc RFC 2865 / 2866 RADIUS Protocol Codec) | Created: 2026-11-22

pub const RADIUS_HEADER_SIZE: usize = 20;
pub const RADIUS_MAX_PACKET_LEN: usize = 4096;
pub const RADIUS_AUTH_PORT: u16 = 1812;
pub const RADIUS_ACCT_PORT: u16 = 1813;

// 常用 RFC 2865 / 2866 / 3579 属性类型常量
pub const ATTR_USER_NAME: u8 = 1;
pub const ATTR_USER_PASSWORD: u8 = 2;
pub const ATTR_NAS_IP_ADDRESS: u8 = 4;
pub const ATTR_NAS_PORT: u8 = 5;
pub const ATTR_SERVICE_TYPE: u8 = 6;
pub const ATTR_FRAMED_IP_ADDRESS: u8 = 8;
pub const ATTR_REPLY_MESSAGE: u8 = 18;
pub const ATTR_STATE: u8 = 24;
pub const ATTR_CLASS: u8 = 25;
pub const ATTR_VENDOR_SPECIFIC: u8 = 26;
pub const ATTR_SESSION_TIMEOUT: u8 = 27;
pub const ATTR_CALLED_STATION_ID: u8 = 30;
pub const ATTR_CALLING_STATION_ID: u8 = 31;
pub const ATTR_NAS_IDENTIFIER: u8 = 32;
pub const ATTR_ACCT_STATUS_TYPE: u8 = 40;
pub const ATTR_ACCT_SESSION_ID: u8 = 44;
pub const ATTR_MESSAGE_AUTHENTICATOR: u8 = 80;

/// RFC 2865 / 2866 / 5176 RADIUS 报文类型码
pub enum RadiusCode: u8 {
    unknown = 0,
    access_request = 1,
    access_accept = 2,
    access_reject = 3,
    accounting_request = 4,
    accounting_response = 5,
    access_challenge = 11,
    status_server = 12,
    status_client = 13,
    disconnect_request = 40,
    disconnect_ack = 41,
    disconnect_nak = 42,
    coa_request = 43,
    coa_ack = 44,
    coa_nak = 45,
}

/// 将原始字节安全映射为 RadiusCode（未知值映射为 unknown）
pub fn radiusCodeFromU8(raw: u8): RadiusCode {
    if (raw == 1) return RadiusCode.access_request;
    if (raw == 2) return RadiusCode.access_accept;
    if (raw == 3) return RadiusCode.access_reject;
    if (raw == 4) return RadiusCode.accounting_request;
    if (raw == 5) return RadiusCode.accounting_response;
    if (raw == 11) return RadiusCode.access_challenge;
    if (raw == 12) return RadiusCode.status_server;
    if (raw == 13) return RadiusCode.status_client;
    if (raw == 40) return RadiusCode.disconnect_request;
    if (raw == 41) return RadiusCode.disconnect_ack;
    if (raw == 42) return RadiusCode.disconnect_nak;
    if (raw == 43) return RadiusCode.coa_request;
    if (raw == 44) return RadiusCode.coa_ack;
    if (raw == 45) return RadiusCode.coa_nak;
    return RadiusCode.unknown;
}

/// 单条 RADIUS AVP 属性（零拷贝切片）
pub struct RadiusAttr {
    attr_type: u8,
    value: []const u8,

    /// 若属性长度为 4 字节，按大端序解码为 u32
    fn asU32(): u32? {
        if (_.value.len != 4) {
            return null;
        }
        return readU32Be(_.value, 0);
    }
}

/// Vendor-Specific Attribute (Type 26) 解码出的单个子属性（零拷贝切片）
pub struct RadiusVsa {
    vendor_id: u32,
    vendor_type: u8,
    value: []const u8,
}

/// 已解析的 RADIUS 报文（20B 报头 + 零拷贝 AVP 区段切片）
pub struct RadiusPacket {
    code: RadiusCode,
    raw_code: u8,
    identifier: u8,
    length: u16,
    authenticator: [16]u8,
    attrs_slice: []const u8,

    /// 返回全部 AVP 属性的零分配迭代器
    fn attrs(): RadiusAttrIter {
        return RadiusAttrIter.init(_.attrs_slice);
    }

    /// 查找首个匹配指定属性类型的值切片
    fn findAttr(attr_type: u8): ([]const u8)? {
        let it = _.attrs();
        while (a := it.next()) {
            if (a.attr_type == attr_type) {
                return a.value;
            }
        }
        return null;
    }

    /// 查找首个匹配指定属性类型的 4B 大端序 u32 值
    fn findU32Attr(attr_type: u8): u32? {
        const val = _.findAttr(attr_type) ?? return null;
        if (val.len != 4) {
            return null;
        }
        return readU32Be(val, 0);
    }
}

// ---------------------------------------------------------------------------
// 内部大端序辅助函数
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    return (u16(buf[off]) << 8) | u16(buf[off + 1]);
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    return (u32(buf[off]) << 24) |
        (u32(buf[off + 1]) << 16) |
        (u32(buf[off + 2]) << 8) |
        u32(buf[off + 3]);
}

// ---------------------------------------------------------------------------
// 公开 API：报文解析、AVP 迭代与 VSA 迭代
// ---------------------------------------------------------------------------

/// 解析 RADIUS 二进制报文，严格校验 20 <= length <= data.len && length <= 4096
pub fn parseRadiusPacket(data: []const u8): RadiusPacket? {
    if (data.len < RADIUS_HEADER_SIZE) {
        return null;
    }
    const raw_code = data[0];
    const identifier = data[1];
    const length = readU16Be(data, 2);
    const len_usize = usize(length);

    if (len_usize < RADIUS_HEADER_SIZE || len_usize > data.len || len_usize > RADIUS_MAX_PACKET_LEN) {
        return null;
    }

    let auth: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        auth[i] = data[4 + i];
        i += 1;
    }

    return RadiusPacket{
        code: radiusCodeFromU8(raw_code),
        raw_code: raw_code,
        identifier: identifier,
        length: length,
        authenticator: auth,
        attrs_slice: data[RADIUS_HEADER_SIZE..len_usize],
    };
}

/// 零分配 AVP 属性迭代器：[1B type, 1B len >= 2, (len - 2)B value]
pub struct RadiusAttrIter {
    data: []const u8,
    pos: usize,
    malformed: bool,

    static fn init(attrs_slice: []const u8): RadiusAttrIter {
        return RadiusAttrIter{
            data: attrs_slice,
            pos: 0,
            malformed: false,
        };
    }

    fn next(*_): RadiusAttr? {
        if (_.malformed || _.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 2) {
            _.malformed = true;
            return null;
        }
        const attr_type = _.data[_.pos];
        const attr_len = usize(_.data[_.pos + 1]);
        if (attr_len < 2 || attr_len > _.data.len - _.pos) {
            _.malformed = true;
            return null;
        }
        const val_start = _.pos + 2;
        const val_end = _.pos + attr_len;
        _.pos = val_end;

        return RadiusAttr{
            attr_type: attr_type,
            value: _.data[val_start..val_end],
        };
    }
}

/// 零分配 Vendor-Specific Attribute (Type 26) 子 TLV 迭代器
pub struct RadiusVsaIter {
    data: []const u8,
    pos: usize,
    vendor_id: u32,
    malformed: bool,

    fn init(vsa_value: []const u8): RadiusVsaIter {
        if (vsa_value.len < 4) {
            return RadiusVsaIter{
                data: vsa_value,
                pos: vsa_value.len,
                vendor_id: 0,
                malformed: true,
            };
        }
        return RadiusVsaIter{
            data: vsa_value,
            pos: 4,
            vendor_id: readU32Be(vsa_value, 0),
            malformed: false,
        };
    }

    fn next(*_): RadiusVsa? {
        if (_.malformed || _.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 2) {
            _.malformed = true;
            return null;
        }
        const vtype = _.data[_.pos];
        const vlen = usize(_.data[_.pos + 1]);
        if (vlen < 2 || vlen > _.data.len - _.pos) {
            _.malformed = true;
            return null;
        }
        const val_start = _.pos + 2;
        const val_end = _.pos + vlen;
        _.pos = val_end;

        return RadiusVsa{
            vendor_id: _.vendor_id,
            vendor_type: vtype,
            value: _.data[val_start..val_end],
        };
    }
}

// ---------------------------------------------------------------------------
// 公开 API：零分配 RADIUS 编码器
// ---------------------------------------------------------------------------

/// 零分配 RADIUS 报文编码器（直接写入目标缓冲区并自动回填 2B BE 总长度）
pub struct RadiusWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,

    fn init(buf: []u8, code: RadiusCode, identifier: u8, authenticator: [16]u8): RadiusWriter {
        if (buf.len < RADIUS_HEADER_SIZE) {
            return RadiusWriter{
                buf: buf,
                pos: 0,
                overflow: true,
            };
        }
        buf[0] = code as u8;
        buf[1] = identifier;
        buf[2] = 0;
        buf[3] = 20;
        let i: usize = 0;
        while (i < 16) {
            buf[4 + i] = authenticator[i];
            i += 1;
        }
        return RadiusWriter{
            buf: buf,
            pos: RADIUS_HEADER_SIZE,
            overflow: false,
        };
    }

    /// 写入单条原始字节 AVP 属性（value.len <= 253）
    fn writeAttr(*_, attr_type: u8, value: []const u8): bool {
        if (_.overflow || value.len > 253) {
            _.overflow = true;
            return false;
        }
        const avp_len = 2 + value.len;
        if (avp_len > _.buf.len - _.pos || _.pos + avp_len > RADIUS_MAX_PACKET_LEN) {
            _.overflow = true;
            return false;
        }
        _.buf[_.pos] = attr_type;
        _.buf[_.pos + 1] = avp_len as u8;
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + 2 + i] = value[i];
            i += 1;
        }
        _.pos += avp_len;
        return true;
    }

    /// 写入 4 字节大端序 u32 AVP 属性
    fn writeU32Attr(*_, attr_type: u8, v: u32): bool {
        let tmp: [4]u8 = [
            ((v >> 24) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        return _.writeAttr(attr_type, tmp[0..4]);
    }

    /// 写入单条 Vendor-Specific Attribute (Type 26) 包含 4B Enterprise Number + 1 个子 TLV
    fn writeVsa(*_, vendor_id: u32, vtype: u8, value: []const u8): bool {
        if (_.overflow || value.len > 247) {
            _.overflow = true;
            return false;
        }
        const sub_len = 2 + value.len;
        const outer_len = 2 + 4 + sub_len;
        if (outer_len > _.buf.len - _.pos || _.pos + outer_len > RADIUS_MAX_PACKET_LEN) {
            _.overflow = true;
            return false;
        }
        _.buf[_.pos] = ATTR_VENDOR_SPECIFIC;
        _.buf[_.pos + 1] = outer_len as u8;
        _.buf[_.pos + 2] = ((vendor_id >> 24) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((vendor_id >> 16) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((vendor_id >> 8) & 0xFF) as u8;
        _.buf[_.pos + 5] = (vendor_id & 0xFF) as u8;
        _.buf[_.pos + 6] = vtype;
        _.buf[_.pos + 7] = sub_len as u8;
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + 8 + i] = value[i];
            i += 1;
        }
        _.pos += outer_len;
        return true;
    }

    /// 回填 2B 大端序总长度并返回已编码的完整报文切片；溢出时返回 null
    fn finish(*_): ([]const u8)? {
        if (_.overflow || _.pos < RADIUS_HEADER_SIZE || _.pos > RADIUS_MAX_PACKET_LEN) {
            return null;
        }
        const total = _.pos as u16;
        _.buf[2] = ((total >> 8) & 0xFF) as u8;
        _.buf[3] = (total & 0xFF) as u8;
        return _.buf[0.._.pos];
    }
}
