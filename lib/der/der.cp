// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ITU-T X.690 ASN.1 DER Strict Codec)

/// ASN.1 Universal Tag 常量
pub const TAG_BOOLEAN: u8 = 0x01;
pub const TAG_INTEGER: u8 = 0x02;
pub const TAG_BIT_STRING: u8 = 0x03;
pub const TAG_OCTET_STRING: u8 = 0x04;
pub const TAG_NULL: u8 = 0x05;
pub const TAG_OID: u8 = 0x06;
pub const TAG_UTF8_STRING: u8 = 0x0C;
pub const TAG_PRINTABLE_STRING: u8 = 0x13;
pub const TAG_IA5_STRING: u8 = 0x16;
pub const TAG_UTC_TIME: u8 = 0x17;
pub const TAG_GENERALIZED_TIME: u8 = 0x18;
pub const TAG_SEQUENCE: u8 = 0x30;
pub const TAG_SET: u8 = 0x31;

/// Tag Class 与 Constructed 位掩码
pub const CLASS_MASK: u8 = 0xC0;
pub const CONSTRUCTED_MASK: u8 = 0x20;

/// 零拷贝 DER TLV 解析结果
pub struct DerTlv {
    tag: u8,
    is_constructed: bool,
    value: []const u8,
    raw: []const u8,
    total_bytes: usize,
}

/// 严格 DER BIT STRING 视图（`unused_bits <= 7` 且末字节未用低位全为 0）
pub struct DerBitString {
    unused_bits: u8,
    bytes: []const u8,
}

/// 严格 DER TLV 零分配解析器（拒绝不定长 `0x80`、非最小长度与越界）
pub fn parseDerTlv(data: []const u8): DerTlv? {
    if (data.len < 2) {
        return null;
    }

    const first_tag: u8 = data[0];
    const is_constructed: bool = (first_tag & CONSTRUCTED_MASK) != 0;
    let pos: usize = 1;

    // 高编号多字节 Tag（低 5 位全 1）
    if ((first_tag & 0x1F) == 0x1F) {
        if (pos >= data.len || data[pos] == 0x80) {
            return null;
        }
        let tag_num: u64 = 0;
        let tag_bytes: usize = 0;
        let done: bool = false;
        while (pos < data.len && tag_bytes < 5) {
            const b: u8 = data[pos];
            pos += 1;
            tag_bytes += 1;
            tag_num = (tag_num << 7) | ((b & 0x7F) as u64);
            if ((b & 0x80) == 0) {
                done = true;
                break;
            }
        }
        if (!done || tag_num < 31) {
            return null;
        }
    }

    if (pos >= data.len) {
        return null;
    }

    // 严格 DER 长度解码
    const len_first: u8 = data[pos];
    pos += 1;

    let val_len_u64: u64 = 0;
    if ((len_first & 0x80) == 0) {
        val_len_u64 = len_first as u64;
    } else {
        if (len_first == 0x80) {
            // DER 严禁不定长编码
            return null;
        }
        const num_octets: usize = (len_first & 0x7F) as usize;
        if (num_octets == 0 || num_octets > 8) {
            return null;
        }
        if (data.len - pos < num_octets) {
            return null;
        }
        if (data[pos] == 0x00) {
            // DER 严禁长度字段前导零
            return null;
        }
        let k: usize = 0;
        while (k < num_octets) {
            val_len_u64 = (val_len_u64 << 8) | (data[pos + k] as u64);
            k += 1;
        }
        pos += num_octets;
        if (val_len_u64 < 128) {
            // DER 严禁使用长形式表示 < 128 的长度
            return null;
        }
    }

    const rem_u64: u64 = (data.len - pos) as u64;
    if (val_len_u64 > rem_u64) {
        return null;
    }

    const val_len: usize = val_len_u64 as usize;
    const total: usize = pos + val_len;
    return DerTlv {
        tag: first_tag,
        is_constructed: is_constructed,
        value: data[pos..total],
        raw: data[0..total],
        total_bytes: total,
    };
}

fn isCanonicalDerInteger(v: []const u8): bool {
    if (v.len == 0) {
        return false;
    }
    if (v.len > 1) {
        if (v[0] == 0x00 && (v[1] & 0x80) == 0) {
            return false;
        }
        if (v[0] == 0xFF && (v[1] & 0x80) != 0) {
            return false;
        }
    }
    return true;
}

/// 零拷贝严格 DER 只读游标
pub struct DerReader {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): DerReader {
        return DerReader { data: data, pos: 0 };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn isEmpty(): bool {
        return _.pos >= _.data.len;
    }

    fn nextTlv(*_): DerTlv? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        _.pos += tlv.total_bytes;
        return tlv;
    }

    fn readBool(*_): bool? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_BOOLEAN || tlv.is_constructed || tlv.value.len != 1) {
            return null;
        }
        const b = tlv.value[0];
        if (b != 0x00 && b != 0xFF) {
            return null;
        }
        _.pos += tlv.total_bytes;
        return b == 0xFF;
    }

    fn readU64(*_): u64? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_INTEGER || tlv.is_constructed) {
            return null;
        }
        const v = tlv.value;
        if (!isCanonicalDerInteger(v)) {
            return null;
        }
        if ((v[0] & 0x80) != 0) {
            return null;
        }
        let start: usize = 0;
        if (v[0] == 0x00 && v.len > 1) {
            start = 1;
        }
        if (v.len - start > 8) {
            return null;
        }
        let acc: u64 = 0;
        let i: usize = start;
        while (i < v.len) {
            acc = (acc << 8) | (v[i] as u64);
            i += 1;
        }
        _.pos += tlv.total_bytes;
        return acc;
    }

    fn readI64(*_): i64? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_INTEGER || tlv.is_constructed) {
            return null;
        }
        const v = tlv.value;
        if (!isCanonicalDerInteger(v) || v.len > 8) {
            return null;
        }
        let acc: u64 = if ((v[0] & 0x80) != 0) 0xFFFFFFFFFFFFFFFF else 0;
        let i: usize = 0;
        while (i < v.len) {
            acc = (acc << 8) | (v[i] as u64);
            i += 1;
        }
        _.pos += tlv.total_bytes;
        const result: i64 = @bitCast(acc);
        return result;
    }

    fn readNull(*_): bool {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return false;
        if (tlv.tag != TAG_NULL || tlv.is_constructed || tlv.value.len != 0) {
            return false;
        }
        _.pos += tlv.total_bytes;
        return true;
    }

    fn readOctetString(*_): ([]const u8)? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_OCTET_STRING || tlv.is_constructed) {
            return null;
        }
        _.pos += tlv.total_bytes;
        return tlv.value;
    }

    fn readOid(*_): ([]const u8)? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_OID || tlv.is_constructed || tlv.value.len == 0) {
            return null;
        }
        _.pos += tlv.total_bytes;
        return tlv.value;
    }

    fn readBitString(*_): DerBitString? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_BIT_STRING || tlv.is_constructed || tlv.value.len == 0) {
            return null;
        }
        const unused: u8 = tlv.value[0];
        if (unused > 7) {
            return null;
        }
        if (tlv.value.len == 1) {
            if (unused != 0) {
                return null;
            }
            _.pos += tlv.total_bytes;
            return DerBitString { unused_bits: 0, bytes: tlv.value[1..1] };
        }
        if (unused > 0) {
            const last: u8 = tlv.value[tlv.value.len - 1];
            const shift: u3 = unused as u3;
            const mask: u8 = ((1 as u8) << shift) - 1;
            if ((last & mask) != 0) {
                return null;
            }
        }
        _.pos += tlv.total_bytes;
        return DerBitString {
            unused_bits: unused,
            bytes: tlv.value[1..tlv.value.len],
        };
    }

    fn readSequence(*_): DerReader? {
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != TAG_SEQUENCE || !tlv.is_constructed) {
            return null;
        }
        _.pos += tlv.total_bytes;
        return DerReader.init(tlv.value);
    }

    fn readContextExplicit(*_, ctx_tag_num: u8): DerReader? {
        if (ctx_tag_num > 30) {
            return null;
        }
        const expected: u8 = (0xA0 as u8) | ctx_tag_num;
        const tlv = parseDerTlv(_.data[_.pos.._.data.len]) ?? return null;
        if (tlv.tag != expected || !tlv.is_constructed) {
            return null;
        }
        _.pos += tlv.total_bytes;
        return DerReader.init(tlv.value);
    }
}

// ---------------------------------------------------------------------------
// OID 点分十进制格式化
// ---------------------------------------------------------------------------

fn appendU64Decimal(out: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [20]u8 = [0; 20];
    let n: usize = 0;
    if (val == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        let v: u64 = val;
        while (v > 0) {
            tmp[n] = ((v % 10) as u8) + '0';
            v = v / 10;
            n += 1;
        }
    }
    if (out.*.len - pos.* < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out.*[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

/// 将 DER 编码的 OID 内容字节格式化为点分十进制字符串（如 `"1.2.840.113549.1.1.11"`）
/// 严格拒绝 base-128 首字节 `0x80` 非规范填充与截断；缓冲区不足返回 `null`
pub fn formatDerOid(oid_bytes: []const u8, out: *[]u8): usize? {
    if (oid_bytes.len == 0) {
        return null;
    }

    let i: usize = 0;
    let out_pos: usize = 0;

    // 解码第一个 base-128 子标识符（包含 arc0 * 40 + arc1）
    if (oid_bytes[i] == 0x80) {
        return null;
    }
    let first_subid: u64 = 0;
    let first_done: bool = false;
    while (i < oid_bytes.len) {
        const b: u8 = oid_bytes[i];
        i += 1;
        if (first_subid > (0xFFFFFFFFFFFFFFFF >> 7)) {
            return null;
        }
        first_subid = (first_subid << 7) | ((b & 0x7F) as u64);
        if ((b & 0x80) == 0) {
            first_done = true;
            break;
        }
    }
    if (!first_done) {
        return null;
    }

    const arc0: u64 = if (first_subid < 40) 0 else if (first_subid < 80) 1 else 2;
    const arc1: u64 = if (arc0 == 0) first_subid else if (arc0 == 1) first_subid - 40 else first_subid - 80;

    if (!appendU64Decimal(out, *out_pos, arc0)) {
        return null;
    }
    if (out_pos >= out.*.len) {
        return null;
    }
    out.*[out_pos] = '.';
    out_pos += 1;
    if (!appendU64Decimal(out, *out_pos, arc1)) {
        return null;
    }

    // 解码后续 base-128 子标识符
    while (i < oid_bytes.len) {
        if (oid_bytes[i] == 0x80) {
            return null;
        }
        let arc: u64 = 0;
        let done: bool = false;
        while (i < oid_bytes.len) {
            const b: u8 = oid_bytes[i];
            i += 1;
            if (arc > (0xFFFFFFFFFFFFFFFF >> 7)) {
                return null;
            }
            arc = (arc << 7) | ((b & 0x7F) as u64);
            if ((b & 0x80) == 0) {
                done = true;
                break;
            }
        }
        if (!done) {
            return null;
        }
        if (out_pos >= out.*.len) {
            return null;
        }
        out.*[out_pos] = '.';
        out_pos += 1;
        if (!appendU64Decimal(out, *out_pos, arc)) {
            return null;
        }
    }

    return out_pos;
}

// ---------------------------------------------------------------------------
// 严格 DER 写入器（溢出原子失败，不破坏已写位置）
// ---------------------------------------------------------------------------

fn derLengthEncodedSize(len: usize): usize {
    if (len < 128) {
        return 1;
    }
    let n: usize = 0;
    let v: usize = len;
    while (v > 0) {
        n += 1;
        v = v >> 8;
    }
    return 1 + n;
}

/// 零分配严格 DER 编码写入器（写入固定缓冲，溢出原子失败）
pub struct DerWriter {
    buf: []u8,
    pos: usize,

    static fn init(buf: []u8): DerWriter {
        return DerWriter { buf: buf, pos: 0 };
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn writeSequenceHeader(*_, content_len: usize): bool {
        return _.writeTagAndLen(TAG_SEQUENCE, content_len);
    }

    fn writeTagAndLen(*_, tag: u8, content_len: usize): bool {
        const len_sz = derLengthEncodedSize(content_len);
        const need = 1 + len_sz;
        if (_.buf.len - _.pos < need) {
            return false;
        }
        _.buf[_.pos] = tag;
        _.pos += 1;
        if (content_len < 128) {
            _.buf[_.pos] = content_len as u8;
            _.pos += 1;
        } else {
            const num_octets = len_sz - 1;
            _.buf[_.pos] = (0x80 | num_octets) as u8;
            _.pos += 1;
            let k: usize = num_octets;
            while (k > 0) {
                k -= 1;
                const shift: u6 = (k * 8) as u6;
                _.buf[_.pos] = ((content_len >> shift) & 0xFF) as u8;
                _.pos += 1;
            }
        }
        return true;
    }

    fn writeTlv(*_, tag: u8, value: []const u8): bool {
        const len_sz = derLengthEncodedSize(value.len);
        const total = 1 + len_sz + value.len;
        if (_.buf.len - _.pos < total) {
            return false;
        }
        if (!_.writeTagAndLen(tag, value.len)) {
            return false;
        }
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + i] = value[i];
            i += 1;
        }
        _.pos += value.len;
        return true;
    }

    fn writeBool(*_, val: bool): bool {
        let b: [1]u8 = [if (val) 0xFF else 0x00];
        return _.writeTlv(TAG_BOOLEAN, b[0..1]);
    }

    fn writeNull(*_): bool {
        let empty: [0]u8 = [];
        return _.writeTlv(TAG_NULL, empty[0..0]);
    }

    fn writeOctetString(*_, data: []const u8): bool {
        return _.writeTlv(TAG_OCTET_STRING, data);
    }

    fn writeUtf8String(*_, text: []const u8): bool {
        return _.writeTlv(TAG_UTF8_STRING, text);
    }

    fn writeU64(*_, val: u64): bool {
        let raw: [9]u8 = [0; 9];
        if (val == 0) {
            raw[0] = 0;
            return _.writeTlv(TAG_INTEGER, raw[0..1]);
        }
        let tmp: [8]u8 = [0; 8];
        let n: usize = 0;
        let v: u64 = val;
        while (v > 0) {
            tmp[n] = (v & 0xFF) as u8;
            v = v >> 8;
            n += 1;
        }
        let out_len: usize = 0;
        if ((tmp[n - 1] & 0x80) != 0) {
            raw[0] = 0x00;
            out_len = 1;
        }
        let i: usize = 0;
        while (i < n) {
            raw[out_len + i] = tmp[n - 1 - i];
            i += 1;
        }
        out_len += n;
        return _.writeTlv(TAG_INTEGER, raw[0..out_len]);
    }
}
