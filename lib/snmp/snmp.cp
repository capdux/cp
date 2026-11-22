// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1157/3416 SNMP v1/v2c PDU & VarBind Parser) | Created: 2026-11-22

/// SNMP 标准请求/响应 UDP 端口
pub const SNMP_PORT: u16 = 161;

/// SNMP Trap / Inform 通知 UDP 端口
pub const SNMP_TRAP_PORT: u16 = 162;

/// SNMP 协议版本：SNMPv1 (RFC 1157)
pub const SNMP_V1: u8 = 0;

/// SNMP 协议版本：SNMPv2c (RFC 1901 / RFC 3416)
pub const SNMP_V2C: u8 = 1;

/// ASN.1 BER 通用构造标签 SEQUENCE
const TAG_SEQUENCE: u8 = 0x30;

/// SNMP PDU 类型枚举（ASN.1 Context-Specific Constructed Tags）
pub enum SnmpPduType: u8 {
    unknown = 0,
    get_request = 0xA0,
    get_next_request = 0xA1,
    get_response = 0xA2,
    set_request = 0xA3,
    v1_trap = 0xA4,
    get_bulk_request = 0xA5,
    inform_request = 0xA6,
    v2_trap = 0xA7,
    report = 0xA8,
}

/// SNMP VarBind 值标签枚举（RFC 1155 / RFC 2578 / RFC 3416）
pub enum SnmpValueTag: u8 {
    integer = 0x02,
    octet_string = 0x04,
    null_val = 0x05,
    oid = 0x06,
    ip_address = 0x40,
    counter32 = 0x41,
    gauge32 = 0x42,
    timeticks = 0x43,
    opaque_val = 0x44,
    counter64 = 0x46,
    no_such_object = 0x80,
    no_such_instance = 0x81,
    end_of_mib_view = 0x82,
}

/// 内部 ASN.1 BER 确定长度 TLV 视图
struct BerTlv {
    tag: u8,
    value: []const u8,
    next_pos: usize,
}

/// 读取单个 ASN.1 BER 确定长度 TLV（零拷贝、禁止不定长与高位多字节标签）
fn readBerTlv(data: []const u8, offset: usize): BerTlv? {
    if (offset >= data.len) {
        return null;
    }
    const tag: u8 = data[offset];
    // SNMP v1/v2c 仅使用单字节标签，拒绝高标签号形式 (tag & 0x1F == 0x1F)
    if ((tag & 0x1F) == 0x1F) {
        return null;
    }
    if (offset + 1 >= data.len) {
        return null;
    }
    const len_byte: u8 = data[offset + 1];
    let length: usize = 0;
    let val_start: usize = offset + 2;

    if ((len_byte & 0x80) == 0) {
        length = len_byte as usize;
    } else {
        const num_octets: usize = (len_byte & 0x7F) as usize;
        // 拒绝不定长度 (0x80) 及超过 4 字节的异常长度字段
        if (num_octets == 0 || num_octets > 4) {
            return null;
        }
        if (data.len - val_start < num_octets) {
            return null;
        }
        let i: usize = 0;
        while (i < num_octets) {
            length = (length << 8) | (data[val_start + i] as usize);
            i += 1;
        }
        val_start += num_octets;
    }

    if (data.len - val_start < length) {
        return null;
    }

    return BerTlv {
        tag: tag,
        value: data[val_start..val_start + length],
        next_pos: val_start + length,
    };
}

/// 解码 ASN.1 BER 有符号整数（二补码大端，1..8 字节）
fn decodeBerI64(raw: []const u8): i64? {
    if (raw.len == 0 || raw.len > 8) {
        return null;
    }
    const is_neg: bool = (raw[0] & 0x80) != 0;
    let acc: u64 = 0;
    if (is_neg) {
        acc = 0xFFFFFFFFFFFFFFFF;
    }
    let i: usize = 0;
    while (i < raw.len) {
        acc = (acc << 8) | (raw[i] as u64);
        i += 1;
    }
    return acc as i64;
}

/// 解码 ASN.1 BER 无符号整数（Counter32 / Gauge32 / TimeTicks / Counter64）
/// 允许 BER 符号位填充的 1 字节前导 0x00（Counter64 最多 9 字节）
fn decodeBerU64(raw: []const u8): u64? {
    if (raw.len == 0 || raw.len > 9) {
        return null;
    }
    // 拒绝负数（最高位为 1）
    if ((raw[0] & 0x80) != 0) {
        return null;
    }
    let start: usize = 0;
    while (start + 1 < raw.len && raw[start] == 0x00) {
        start += 1;
    }
    if (raw.len - start > 8) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = start;
    while (i < raw.len) {
        acc = (acc << 8) | (raw[i] as u64);
        i += 1;
    }
    return acc;
}

/// 将原始 PDU 标签字节映射为 SnmpPduType
fn mapPduType(tag: u8): SnmpPduType {
    if (tag == 0xA0) { return .get_request; }
    if (tag == 0xA1) { return .get_next_request; }
    if (tag == 0xA2) { return .get_response; }
    if (tag == 0xA3) { return .set_request; }
    if (tag == 0xA4) { return .v1_trap; }
    if (tag == 0xA5) { return .get_bulk_request; }
    if (tag == 0xA6) { return .inform_request; }
    if (tag == 0xA7) { return .v2_trap; }
    if (tag == 0xA8) { return .report; }
    return .unknown;
}

/// 写入十进制无符号整数到目标切片缓冲区
fn writeDecimalU64(out: *[]u8, pos: *usize, val: u64): bool {
    let digits: [20]u8 = [0; 20];
    let count: usize = 0;
    let n: u64 = val;
    if (n == 0) {
        digits[0] = 48; // '0'
        count = 1;
    } else {
        while (n > 0) {
            digits[count] = (48 + (n % 10)) as u8;
            n = n / 10;
            count += 1;
        }
    }
    if (out.*.len - pos.* < count) {
        return false;
    }
    let i: usize = 0;
    while (i < count) {
        out.*[pos.*] = digits[count - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// 将 ASN.1 BER 编码的 OID 字节流格式化为点分十进制字符串（如 `"1.3.6.1.2.1.1.1.0"`）
/// 成功返回写入字节数；OID 非法、截断、溢出或输出缓冲不足时返回 `null`。
pub fn formatSnmpOid(oid_bytes: []const u8, out: *[]u8): usize? {
    if (oid_bytes.len == 0) {
        return null;
    }

    // 1. 解析首子标识（支持 X.690 完整的首子标识 Base-128 VLQ，覆盖 arc0 = 0/1/2）
    let idx: usize = 0;
    if (oid_bytes[0] == 0x80) {
        return null;
    }
    let first_subid: u64 = 0;
    let finished: bool = false;
    while (idx < oid_bytes.len) {
        const b: u8 = oid_bytes[idx];
        idx += 1;
        if (first_subid > 0x01FFFFFFFFFFFFFF) {
            return null;
        }
        first_subid = (first_subid << 7) | ((b & 0x7F) as u64);
        if ((b & 0x80) == 0) {
            finished = true;
            break;
        }
    }
    if (!finished) {
        return null;
    }

    let arc0: u64 = 0;
    let arc1: u64 = 0;
    if (first_subid < 40) {
        arc0 = 0;
        arc1 = first_subid;
    } else if (first_subid < 80) {
        arc0 = 1;
        arc1 = first_subid - 40;
    } else {
        arc0 = 2;
        arc1 = first_subid - 80;
    }

    let pos: usize = 0;
    if (!writeDecimalU64(out, *pos, arc0)) {
        return null;
    }
    if (pos >= out.*.len) {
        return null;
    }
    out.*[pos] = 46; // '.'
    pos += 1;
    if (!writeDecimalU64(out, *pos, arc1)) {
        return null;
    }

    // 2. 依次解析剩余 Base-128 VLQ 子标识
    while (idx < oid_bytes.len) {
        // 拒绝非最小编码（首字节为 0x80）
        if (oid_bytes[idx] == 0x80) {
            return null;
        }
        let subid: u64 = 0;
        let sub_done: bool = false;
        while (idx < oid_bytes.len) {
            const b: u8 = oid_bytes[idx];
            idx += 1;
            if (subid > 0x01FFFFFFFFFFFFFF) {
                return null;
            }
            subid = (subid << 7) | ((b & 0x7F) as u64);
            if ((b & 0x80) == 0) {
                sub_done = true;
                break;
            }
        }
        if (!sub_done) {
            return null;
        }
        if (pos >= out.*.len) {
            return null;
        }
        out.*[pos] = 46; // '.'
        pos += 1;
        if (!writeDecimalU64(out, *pos, subid)) {
            return null;
        }
    }

    return pos;
}

/// 单个 SNMP VarBind 零拷贝视图：`SEQUENCE { oid OBJECT IDENTIFIER, value ANY }`
pub struct SnmpVarBind {
    oid_bytes: []const u8,
    value_tag: u8,
    value_raw: []const u8,

    /// 若值标签为 INTEGER (0x02)，解码有符号 64 位整数
    fn asI64(): i64? {
        if (_.value_tag != (SnmpValueTag.integer as u8)) {
            return null;
        }
        return decodeBerI64(_.value_raw);
    }

    /// 若值标签为 Counter32 / Gauge32 / TimeTicks / Counter64，解码无符号 64 位整数
    fn asU64(): u64? {
        if (
            _.value_tag != (SnmpValueTag.counter32 as u8) &&
            _.value_tag != (SnmpValueTag.gauge32 as u8) &&
            _.value_tag != (SnmpValueTag.timeticks as u8) &&
            _.value_tag != (SnmpValueTag.counter64 as u8)
        ) {
            return null;
        }
        return decodeBerU64(_.value_raw);
    }

    /// 若值标签为 OCTET STRING (0x04)，返回原始字节切片
    fn asOctetString(): ([]const u8)? {
        if (_.value_tag != (SnmpValueTag.octet_string as u8)) {
            return null;
        }
        return _.value_raw;
    }

    /// 若值标签为 IpAddress (0x40) 且长度为 4，返回 IPv4 四字节数组
    fn asIp4(): ([4]u8)? {
        if (_.value_tag != (SnmpValueTag.ip_address as u8) || _.value_raw.len != 4) {
            return null;
        }
        let ip: [4]u8 = [
            _.value_raw[0],
            _.value_raw[1],
            _.value_raw[2],
            _.value_raw[3],
        ];
        return ip;
    }

    /// 判断值是否为 NULL (0x05)
    fn isNull(): bool {
        return _.value_tag == (SnmpValueTag.null_val as u8) && _.value_raw.len == 0;
    }

    /// 判断值是否为 SNMPv2 异常哨兵（noSuchObject / noSuchInstance / endOfMibView）
    fn isException(): bool {
        return _.value_tag == (SnmpValueTag.no_such_object as u8) ||
            _.value_tag == (SnmpValueTag.no_such_instance as u8) ||
            _.value_tag == (SnmpValueTag.end_of_mib_view as u8);
    }
}

/// SNMP VarBindList 零分配迭代器
pub struct SnmpVarBindIter {
    data: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(varbinds_der: []const u8): SnmpVarBindIter {
        return SnmpVarBindIter {
            data: varbinds_der,
            pos: 0,
            had_error: false,
        };
    }

    /// 读取下一条 VarBind；遇到截断或格式错误时设置 `had_error = true` 并终止迭代
    fn next(*_): SnmpVarBind? {
        if (_.pos >= _.data.len) {
            return null;
        }
        const seq_opt = readBerTlv(_.data, _.pos);
        if (seq_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const seq = seq_opt ?? unreachable;
        if (seq.tag != TAG_SEQUENCE) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        // VarBind 内第 1 项：OBJECT IDENTIFIER (0x06)
        const oid_opt = readBerTlv(seq.value, 0);
        if (oid_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const oid_tlv = oid_opt ?? unreachable;
        if (oid_tlv.tag != (SnmpValueTag.oid as u8) || oid_tlv.value.len == 0) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        // VarBind 内第 2 项：Value (ANY)
        const val_opt = readBerTlv(seq.value, oid_tlv.next_pos);
        if (val_opt == null) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }
        const val_tlv = val_opt ?? unreachable;
        // 严禁单条 VarBind 内存在未解析尾随字节
        if (val_tlv.next_pos != seq.value.len) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        _.pos = seq.next_pos;
        return SnmpVarBind {
            oid_bytes: oid_tlv.value,
            value_tag: val_tlv.tag,
            value_raw: val_tlv.value,
        };
    }
}

/// SNMP v1 / v2c 完整报文零拷贝解析结果
pub struct SnmpMessage {
    version: u8,
    community: []const u8,
    pdu_type: SnmpPduType,
    raw_pdu_tag: u8,

    // 标准 PDU 字段（GetBulkRequest 中 error_status 为 non_repeaters，error_index 为 max_repetitions）
    request_id: i64,
    error_status: i64,
    error_index: i64,

    // SNMP v1 Trap PDU (0xA4) 专属字段
    enterprise_oid: []const u8,
    agent_addr: [4]u8,
    generic_trap: i64,
    specific_trap: i64,
    timestamp: u64,

    // VarBindList 内部 DER 字节切片（不含外层 0x30 SEQUENCE 头）
    varbinds_der: []const u8,

    /// 返回 VarBindList 零分配迭代器
    fn varbinds(): SnmpVarBindIter {
        return SnmpVarBindIter.init(_.varbinds_der);
    }

    /// 便捷读取：GetBulkRequest 的 non_repeaters（别名 error_status）
    fn nonRepeaters(): i64 {
        return _.error_status;
    }

    /// 便捷读取：GetBulkRequest 的 max_repetitions（别名 error_index）
    fn maxRepetitions(): i64 {
        return _.error_index;
    }
}

/// 解析单条完整 SNMP v1 / v2c ASN.1 BER 报文（零堆分配、拒绝尾随垃圾与畸形长度）
pub fn parseSnmpMessage(data: []const u8): SnmpMessage? {
    // 1. 外层 SEQUENCE { version, community, pdu }
    const outer = readBerTlv(data, 0) ?? return null;
    if (outer.tag != TAG_SEQUENCE) {
        return null;
    }
    // 拒绝尾随垃圾字节
    if (outer.next_pos != data.len) {
        return null;
    }

    const body = outer.value;

    // 2. version INTEGER (0 = SNMPv1, 1 = SNMPv2c)
    const ver_tlv = readBerTlv(body, 0) ?? return null;
    if (ver_tlv.tag != (SnmpValueTag.integer as u8)) {
        return null;
    }
    const ver_i64 = decodeBerI64(ver_tlv.value) ?? return null;
    if (ver_i64 != 0 && ver_i64 != 1) {
        return null;
    }
    const version: u8 = ver_i64 as u8;

    // 3. community OCTET STRING
    const comm_tlv = readBerTlv(body, ver_tlv.next_pos) ?? return null;
    if (comm_tlv.tag != (SnmpValueTag.octet_string as u8)) {
        return null;
    }

    // 4. PDU [CONTEXT]
    const pdu_tlv = readBerTlv(body, comm_tlv.next_pos) ?? return null;
    // 拒绝外层 SEQUENCE 内尾随字节
    if (pdu_tlv.next_pos != body.len) {
        return null;
    }

    const pdu_type = mapPduType(pdu_tlv.tag);
    if (pdu_type == .unknown) {
        return null;
    }

    const pdu_body = pdu_tlv.value;

    // 5A. SNMP v1 Trap PDU (0xA4)
    if (pdu_type == .v1_trap) {
        if (version != SNMP_V1) {
            return null;
        }
        // enterprise OBJECT IDENTIFIER
        const ent_tlv = readBerTlv(pdu_body, 0) ?? return null;
        if (ent_tlv.tag != (SnmpValueTag.oid as u8) || ent_tlv.value.len == 0) {
            return null;
        }
        // agent-addr IpAddress (4 bytes)
        const addr_tlv = readBerTlv(pdu_body, ent_tlv.next_pos) ?? return null;
        if (addr_tlv.tag != (SnmpValueTag.ip_address as u8) || addr_tlv.value.len != 4) {
            return null;
        }
        // generic-trap INTEGER
        const gen_tlv = readBerTlv(pdu_body, addr_tlv.next_pos) ?? return null;
        if (gen_tlv.tag != (SnmpValueTag.integer as u8)) {
            return null;
        }
        const generic_trap = decodeBerI64(gen_tlv.value) ?? return null;

        // specific-trap INTEGER
        const spec_tlv = readBerTlv(pdu_body, gen_tlv.next_pos) ?? return null;
        if (spec_tlv.tag != (SnmpValueTag.integer as u8)) {
            return null;
        }
        const specific_trap = decodeBerI64(spec_tlv.value) ?? return null;

        // time-stamp TimeTicks
        const ts_tlv = readBerTlv(pdu_body, spec_tlv.next_pos) ?? return null;
        if (ts_tlv.tag != (SnmpValueTag.timeticks as u8)) {
            return null;
        }
        const timestamp = decodeBerU64(ts_tlv.value) ?? return null;

        // variable-bindings SEQUENCE
        const vb_tlv = readBerTlv(pdu_body, ts_tlv.next_pos) ?? return null;
        if (vb_tlv.tag != TAG_SEQUENCE || vb_tlv.next_pos != pdu_body.len) {
            return null;
        }

        return SnmpMessage {
            version: version,
            community: comm_tlv.value,
            pdu_type: pdu_type,
            raw_pdu_tag: pdu_tlv.tag,
            request_id: 0,
            error_status: 0,
            error_index: 0,
            enterprise_oid: ent_tlv.value,
            agent_addr: [
                addr_tlv.value[0],
                addr_tlv.value[1],
                addr_tlv.value[2],
                addr_tlv.value[3],
            ],
            generic_trap: generic_trap,
            specific_trap: specific_trap,
            timestamp: timestamp,
            varbinds_der: vb_tlv.value,
        };
    }

    // 5B. 标准 PDU (0xA0..0xA3, 0xA5..0xA8)
    // request-id INTEGER
    const req_tlv = readBerTlv(pdu_body, 0) ?? return null;
    if (req_tlv.tag != (SnmpValueTag.integer as u8)) {
        return null;
    }
    const request_id = decodeBerI64(req_tlv.value) ?? return null;

    // error-status (或 non-repeaters) INTEGER
    const err_tlv = readBerTlv(pdu_body, req_tlv.next_pos) ?? return null;
    if (err_tlv.tag != (SnmpValueTag.integer as u8)) {
        return null;
    }
    const error_status = decodeBerI64(err_tlv.value) ?? return null;

    // error-index (或 max-repetitions) INTEGER
    const idx_tlv = readBerTlv(pdu_body, err_tlv.next_pos) ?? return null;
    if (idx_tlv.tag != (SnmpValueTag.integer as u8)) {
        return null;
    }
    const error_index = decodeBerI64(idx_tlv.value) ?? return null;

    // variable-bindings SEQUENCE
    const vb_tlv = readBerTlv(pdu_body, idx_tlv.next_pos) ?? return null;
    if (vb_tlv.tag != TAG_SEQUENCE || vb_tlv.next_pos != pdu_body.len) {
        return null;
    }

    const empty_oid: []const u8 = data[0..0];
    return SnmpMessage {
        version: version,
        community: comm_tlv.value,
        pdu_type: pdu_type,
        raw_pdu_tag: pdu_tlv.tag,
        request_id: request_id,
        error_status: error_status,
        error_index: error_index,
        enterprise_oid: empty_oid,
        agent_addr: [0, 0, 0, 0],
        generic_trap: 0,
        specific_trap: 0,
        timestamp: 0,
        varbinds_der: vb_tlv.value,
    };
}
