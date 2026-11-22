// Origin: Pure-CP Stdlib (Zero-Alloc Ethereum RLP Canonical Codec) | Created: 2026-11-22

/// RLP 项类型枚举（字节串或列表）
pub enum RlpKind: u8 {
    bytes = 0,
    list = 1,
}

/// 零拷贝 RLP 解码项视图
pub struct RlpItem {
    kind: RlpKind,
    payload: []const u8,
    total_bytes: usize,
}

/// 零堆分配 RLP 列表子项流式迭代器
pub struct RlpListIter {
    data: []const u8,
    pos: usize = 0,
    failed: bool = false,

    static fn init(list_payload: []const u8): RlpListIter {
        return RlpListIter {
            data: list_payload,
            pos: 0,
            failed: false,
        };
    }

    /// 读取列表中的下一个子项；到达末尾或遇到非法/截断编码时返回 `null`
    fn next(*_): RlpItem? {
        if (_.failed || _.pos >= _.data.len) return null;
        if (item := decodeRlpItem(_.data[_.pos .. _.data.len])) {
            _.pos += item.total_bytes;
            return item;
        }
        _.failed = true;
        _.pos = _.data.len;
        return null;
    }

    /// 检查是否无错误且精确消耗完全部列表负载
    fn isDone(): bool {
        return !_.failed && _.pos == _.data.len;
    }
}

/// 零堆分配 RLP 二进制写入器（写入调用方提供的目标切片）
pub struct RlpWriter {
    buf: []u8,
    pos: usize = 0,

    static fn init(buf: []u8): RlpWriter {
        return RlpWriter {
            buf: buf,
            pos: 0,
        };
    }

    /// 返回已写入的只读字节切片
    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    /// 写入规范 RLP 字节串；目标缓冲区不足时原子返回 `false`
    fn writeBytes(*_, payload: []const u8): bool {
        const need = encodedBytesLen(payload);
        if (need > _.buf.len - _.pos) return false;

        if (payload.len == 1 && payload[0] < 0x80) {
            _.buf[_.pos] = payload[0];
            _.pos += 1;
            return true;
        }

        if (payload.len <= 55) {
            _.buf[_.pos] = (payload.len as u8) + 0x80;
            _.pos += 1;
        } else {
            const len_bytes = uintByteLen(payload.len as u64);
            _.buf[_.pos] = (len_bytes as u8) + 0xB7;
            _.pos += 1;
            writeBigEndianU64(_.buf, _.pos, payload.len as u64, len_bytes);
            _.pos += len_bytes;
        }

        let i: usize = 0;
        while (i < payload.len) {
            _.buf[_.pos + i] = payload[i];
            i += 1;
        }
        _.pos += payload.len;
        return true;
    }

    /// 写入规范无符号大端整数（`0` 编码为 `0x80`，`< 0x80` 编码为单字节自身）
    fn writeU64(*_, value: u64): bool {
        const need = encodedU64Len(value);
        if (need > _.buf.len - _.pos) return false;

        if (value == 0) {
            _.buf[_.pos] = 0x80;
            _.pos += 1;
            return true;
        }
        if (value < 0x80) {
            _.buf[_.pos] = value as u8;
            _.pos += 1;
            return true;
        }

        const n = uintByteLen(value);
        _.buf[_.pos] = (n as u8) + 0x80;
        _.pos += 1;
        writeBigEndianU64(_.buf, _.pos, value, n);
        _.pos += n;
        return true;
    }

    /// 写入规范 RLP 列表头（调用方随后写入总长为 `payload_len` 的编码子项）
    fn writeListHeader(*_, payload_len: usize): bool {
        const need = encodedListHeaderLen(payload_len);
        if (need > _.buf.len - _.pos) return false;

        if (payload_len <= 55) {
            _.buf[_.pos] = (payload_len as u8) + 0xC0;
            _.pos += 1;
            return true;
        }

        const len_bytes = uintByteLen(payload_len as u64);
        _.buf[_.pos] = (len_bytes as u8) + 0xF7;
        _.pos += 1;
        writeBigEndianU64(_.buf, _.pos, payload_len as u64, len_bytes);
        _.pos += len_bytes;
        return true;
    }
}

fn uintByteLen(v: u64): usize {
    if (v == 0) return 0;
    let x = v;
    let n: usize = 0;
    while (x > 0) {
        n += 1;
        x = x >> 8;
    }
    return n;
}

fn writeBigEndianU64(dst: []u8, off: usize, v: u64, n: usize) {
    let i: usize = 0;
    while (i < n) {
        const shift: u6 = ((n - 1 - i) * 8) as u6;
        dst[off + i] = ((v >> shift) & 0xFF) as u8;
        i += 1;
    }
}

fn readCanonicalLen(data: []const u8, len_of_len: usize): usize? {
    if (len_of_len == 0 || len_of_len > 8) return null;
    if (data.len - 1 < len_of_len) return null;
    // 严格规范性：大端长度不得有前导 0x00
    if (data[1] == 0x00) return null;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < len_of_len) {
        acc = (acc << 8) | (data[i + 1] as u64);
        i += 1;
    }
    // 严格规范性：长前缀编码长度必须 >= 56
    if (acc < 56) return null;
    return acc as usize;
}

/// 严格按 Ethereum Yellow Paper 规范性规则解码单个 RLP 项（拒绝一切非规范编码）
pub fn decodeRlpItem(data: []const u8): RlpItem? {
    if (data.len == 0) return null;

    const prefix = data[0];
    if (prefix < 0x80) {
        return RlpItem {
            kind: RlpKind.bytes,
            payload: data[0..1],
            total_bytes: 1,
        };
    }

    if (prefix <= 0xB7) {
        const len: usize = (prefix - 0x80) as usize;
        if (len > data.len - 1) return null;
        // 非规范单字节校验：单字节值 < 0x80 禁止使用 0x81 前缀
        if (len == 1 && data[1] < 0x80) return null;
        return RlpItem {
            kind: RlpKind.bytes,
            payload: data[1 .. len + 1],
            total_bytes: len + 1,
        };
    }

    if (prefix <= 0xBF) {
        const len_of_len: usize = (prefix - 0xB7) as usize;
        const len = readCanonicalLen(data, len_of_len) ?? return null;
        const header_len: usize = len_of_len + 1;
        if (len > data.len - header_len) return null;
        return RlpItem {
            kind: RlpKind.bytes,
            payload: data[header_len .. header_len + len],
            total_bytes: header_len + len,
        };
    }

    if (prefix <= 0xF7) {
        const len: usize = (prefix - 0xC0) as usize;
        if (len > data.len - 1) return null;
        return RlpItem {
            kind: RlpKind.list,
            payload: data[1 .. len + 1],
            total_bytes: len + 1,
        };
    }

    const len_of_len: usize = (prefix - 0xF7) as usize;
    const len = readCanonicalLen(data, len_of_len) ?? return null;
    const header_len: usize = len_of_len + 1;
    if (len > data.len - header_len) return null;
    return RlpItem {
        kind: RlpKind.list,
        payload: data[header_len .. header_len + len],
        total_bytes: header_len + len,
    };
}

/// 严格解码规范 RLP 无符号 64 位整数（`0x80` -> `0`，拒绝前导 `0x00` 与超 8 字节）
pub fn decodeRlpU64(data: []const u8): u64? {
    const item = decodeRlpItem(data) ?? return null;
    if (item.kind != RlpKind.bytes) return null;
    if (item.payload.len == 0) return 0;
    if (item.payload.len > 8) return null;
    if (item.payload[0] == 0x00) return null;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < item.payload.len) {
        acc = (acc << 8) | (item.payload[i] as u64);
        i += 1;
    }
    return acc;
}

/// 计算字节串的规范 RLP 编码总字节长
pub fn encodedBytesLen(payload: []const u8): usize {
    if (payload.len == 1 && payload[0] < 0x80) return 1;
    if (payload.len <= 55) return payload.len + 1;
    return uintByteLen(payload.len as u64) + payload.len + 1;
}

/// 计算无符号整数 `u64` 的规范 RLP 编码总字节长
pub fn encodedU64Len(value: u64): usize {
    if (value == 0 || value < 0x80) return 1;
    return uintByteLen(value) + 1;
}

/// 计算给定子项总长度的 RLP 列表头字节长
pub fn encodedListHeaderLen(payload_len: usize): usize {
    if (payload_len <= 55) return 1;
    return uintByteLen(payload_len as u64) + 1;
}
