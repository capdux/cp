// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4271 BGP-4 Protocol Parser) | Created: 2026-11-22

/// BGP-4 固定 19 字节报头长度（16B Marker + 2B Length + 1B Type）
pub const BGP_HEADER_SIZE: usize = 19;

/// BGP-4 同步标记长度（16 字节全 0xFF）
pub const BGP_MARKER_LEN: usize = 16;

/// RFC 4271 标准最大单报文长度（4096 字节）
pub const BGP_MAX_MESSAGE_LEN: usize = 4096;

/// BGP 默认 TCP 端口
pub const BGP_PORT: u16 = 179;

/// 常用 BGP-4 Path Attribute Type Code
pub const ATTR_ORIGIN: u8 = 1;
pub const ATTR_AS_PATH: u8 = 2;
pub const ATTR_NEXT_HOP: u8 = 3;
pub const ATTR_MULTI_EXIT_DISC: u8 = 4;
pub const ATTR_LOCAL_PREF: u8 = 5;
pub const ATTR_ATOMIC_AGGREGATE: u8 = 6;
pub const ATTR_AGGREGATOR: u8 = 7;
pub const ATTR_COMMUNITIES: u8 = 8;

/// BGP-4 报文类型枚举
pub enum BgpMsgType: u8 {
    unknown = 0,
    open = 1,
    update = 2,
    notification = 3,
    keepalive = 4,
    route_refresh = 5,
}

/// BGP-4 19 字节固定报头
pub struct BgpHeader {
    length: u16,
    msg_type: BgpMsgType,
    raw_type: u8,
}

/// BGP-4 完整单条报文（报头 + 零拷贝载荷切片）
pub struct BgpMessage {
    header: BgpHeader,
    body: []const u8,
    total_bytes: usize,
}

/// BGP-4 OPEN 报文可选参数 TLV（RFC 4271 §4.2）
pub struct BgpOptParam {
    param_type: u8,
    value: []const u8,
}

/// BGP-4 OPEN 可选参数零分配迭代器
pub struct BgpOptParamIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(opt_params_slice: []const u8): BgpOptParamIter {
        return BgpOptParamIter {
            data: opt_params_slice,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): BgpOptParam? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 2) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const ptype: u8 = _.data[_.pos];
        const plen: usize = _.data[_.pos + 1] as usize;
        const val_start: usize = _.pos + 2;
        if (_.data.len - val_start < plen) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + plen];
        _.pos = val_start + plen;
        return BgpOptParam {
            param_type: ptype,
            value: val,
        };
    }
}

/// BGP-4 OPEN 报文体解析结果
pub struct BgpOpen {
    version: u8,
    my_as: u16,
    hold_time: u16,
    bgp_id: [4]u8,
    opt_params_slice: []const u8,

    /// 返回可选能力参数 TLV 零分配迭代器
    fn optParams(): BgpOptParamIter {
        return BgpOptParamIter.init(_.opt_params_slice);
    }
}

/// BGP-4 UPDATE 报文体零拷贝三段切片视图
pub struct BgpUpdate {
    withdrawn_routes: []const u8,
    path_attrs: []const u8,
    nlri: []const u8,
}

/// RFC 4271 压缩 IPv4 CIDR 前缀
pub struct BgpPrefix {
    prefix_len: u8,
    addr: [4]u8,
}

/// 零分配 IPv4 压缩前缀迭代器（适用于 `withdrawn_routes` 与 `nlri`）
pub struct BgpPrefixIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(slice: []const u8): BgpPrefixIter {
        return BgpPrefixIter {
            data: slice,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): BgpPrefix? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const plen: u8 = _.data[_.pos];
        if (plen > 32) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const octets: usize = ((plen as usize) + 7) / 8;
        const val_start: usize = _.pos + 1;
        if (_.data.len - val_start < octets) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        let addr: [4]u8 = [0; 4];
        let i: usize = 0;
        while (i < octets) {
            addr[i] = _.data[val_start + i];
            i += 1;
        }
        _.pos = val_start + octets;
        return BgpPrefix {
            prefix_len: plen,
            addr: addr,
        };
    }
}

/// BGP-4 Path Attribute 零拷贝视图
pub struct BgpPathAttr {
    flags: u8,
    type_code: u8,
    value: []const u8,

    fn isOptional(): bool {
        return (_.flags & 0x80) != 0;
    }

    fn isTransitive(): bool {
        return (_.flags & 0x40) != 0;
    }

    fn isExtendedLen(): bool {
        return (_.flags & 0x10) != 0;
    }
}

/// BGP-4 Path Attribute 零分配迭代器（自动处理标准 1B 与 Extended 2B 长度）
pub struct BgpPathAttrIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(path_attrs: []const u8): BgpPathAttrIter {
        return BgpPathAttrIter {
            data: path_attrs,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): BgpPathAttr? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < 3) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const flags: u8 = _.data[_.pos];
        const type_code: u8 = _.data[_.pos + 1];
        const extended: bool = (flags & 0x10) != 0;
        let len: usize = 0;
        let val_start: usize = 0;

        if (extended) {
            if (_.data.len - _.pos < 4) {
                _.had_error = true;
                _.pos = _.data.len;
                return null;
            }
            len = readU16Be(_.data, _.pos + 2) as usize;
            val_start = _.pos + 4;
        } else {
            len = _.data[_.pos + 2] as usize;
            val_start = _.pos + 3;
        }

        if (_.data.len - val_start < len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const val = _.data[val_start..val_start + len];
        _.pos = val_start + len;
        return BgpPathAttr {
            flags: flags,
            type_code: type_code,
            value: val,
        };
    }
}

/// BGP-4 NOTIFICATION 报文体解析结果
pub struct BgpNotification {
    error_code: u8,
    error_subcode: u8,
    data: []const u8,
}

/// TCP 字节流中多条拼接 BGP 报文零分配迭代器
pub struct BgpStreamIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(data: []const u8): BgpStreamIter {
        return BgpStreamIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): BgpMessage? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const msg_opt = parseBgpMessage(_.data[_.pos.._.data.len]);
        if (msg_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const msg = msg_opt ?? return null;
        _.pos += msg.total_bytes;
        return msg;
    }
}

// ---------------------------------------------------------------------------
// 内部辅助函数
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn decodeMsgType(raw: u8): BgpMsgType {
    return match (raw) {
        1 => .open,
        2 => .update,
        3 => .notification,
        4 => .keepalive,
        5 => .route_refresh,
        else => .unknown,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析单条 BGP-4 报文（严格校验 16B 全 0xFF Marker、19 <= length <= 4096 与 KEEPALIVE 定长 19B）
pub fn parseBgpMessage(data: []const u8): BgpMessage? {
    if (data.len < BGP_HEADER_SIZE) {
        return null;
    }
    let i: usize = 0;
    while (i < BGP_MARKER_LEN) {
        if (data[i] != 0xFF) {
            return null;
        }
        i += 1;
    }

    const length: u16 = readU16Be(data, 16);
    const len_usize: usize = length as usize;
    if (len_usize < BGP_HEADER_SIZE || len_usize > BGP_MAX_MESSAGE_LEN || len_usize > data.len) {
        return null;
    }

    const raw_type: u8 = data[18];
    const msg_type = decodeMsgType(raw_type);
    if (msg_type == .unknown) {
        return null;
    }
    if (msg_type == .keepalive && len_usize != BGP_HEADER_SIZE) {
        return null;
    }

    return BgpMessage {
        header: BgpHeader {
            length: length,
            msg_type: msg_type,
            raw_type: raw_type,
        },
        body: data[BGP_HEADER_SIZE..len_usize],
        total_bytes: len_usize,
    };
}

/// 解析 BGP-4 OPEN 报文体（要求 version == 4 且 opt_param_len 精确匹配）
pub fn parseBgpOpen(body: []const u8): BgpOpen? {
    if (body.len < 10) {
        return null;
    }
    const version: u8 = body[0];
    if (version != 4) {
        return null;
    }
    const my_as: u16 = readU16Be(body, 1);
    const hold_time: u16 = readU16Be(body, 3);
    const bgp_id: [4]u8 = [body[5], body[6], body[7], body[8]];
    const opt_len: usize = body[9] as usize;
    if (opt_len + 10 != body.len) {
        return null;
    }

    return BgpOpen {
        version: version,
        my_as: my_as,
        hold_time: hold_time,
        bgp_id: bgp_id,
        opt_params_slice: body[10..body.len],
    };
}

/// 解析 BGP-4 UPDATE 报文体（零拷贝切分 withdrawn_routes / path_attrs / nlri）
pub fn parseBgpUpdate(body: []const u8): BgpUpdate? {
    if (body.len < 4) {
        return null;
    }
    const withdrawn_len: usize = readU16Be(body, 0) as usize;
    if (body.len - 2 < withdrawn_len + 2) {
        return null;
    }
    const pa_len_off: usize = withdrawn_len + 2;
    const total_pa_len: usize = readU16Be(body, pa_len_off) as usize;
    const pa_start: usize = pa_len_off + 2;
    if (body.len - pa_start < total_pa_len) {
        return null;
    }
    const nlri_start: usize = pa_start + total_pa_len;

    return BgpUpdate {
        withdrawn_routes: body[2..pa_len_off],
        path_attrs: body[pa_start..nlri_start],
        nlri: body[nlri_start..body.len],
    };
}

/// 解析 BGP-4 NOTIFICATION 报文体
pub fn parseBgpNotification(body: []const u8): BgpNotification? {
    if (body.len < 2) {
        return null;
    }
    return BgpNotification {
        error_code: body[0],
        error_subcode: body[1],
        data: body[2..body.len],
    };
}
