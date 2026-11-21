// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Redis RDB Snapshot Parser)

/// Redis RDB 文件魔数与头部大小
pub const RDB_MAGIC: []const u8 = "REDIS";
pub const RDB_HEADER_SIZE: usize = 9;

/// Redis RDB 核心控制 Opcode
pub const OP_MODULE_AUX: u8 = 0xF7;
pub const OP_IDLE: u8 = 0xF8;
pub const OP_FREQ: u8 = 0xF9;
pub const OP_AUX: u8 = 0xFA;
pub const OP_RESIZEDB: u8 = 0xFB;
pub const OP_EXPIRETIME_MS: u8 = 0xFC;
pub const OP_EXPIRETIME: u8 = 0xFD;
pub const OP_SELECTDB: u8 = 0xFE;
pub const OP_EOF: u8 = 0xFF;

/// Redis RDB 值类型常量
pub const RDB_TYPE_STRING: u8 = 0;
pub const RDB_TYPE_LIST: u8 = 1;
pub const RDB_TYPE_SET: u8 = 2;
pub const RDB_TYPE_ZSET: u8 = 3;
pub const RDB_TYPE_HASH: u8 = 4;
pub const RDB_TYPE_ZSET_2: u8 = 5;
pub const RDB_TYPE_HASH_ZIPMAP: u8 = 9;
pub const RDB_TYPE_LIST_ZIPLIST: u8 = 10;
pub const RDB_TYPE_SET_INTSET: u8 = 11;
pub const RDB_TYPE_ZSET_ZIPLIST: u8 = 12;
pub const RDB_TYPE_HASH_ZIPLIST: u8 = 13;
pub const RDB_TYPE_LIST_QUICKLIST: u8 = 14;
pub const RDB_TYPE_STREAM_LISTPACKS: u8 = 15;
pub const RDB_TYPE_HASH_LISTPACK: u8 = 16;
pub const RDB_TYPE_ZSET_LISTPACK: u8 = 17;
pub const RDB_TYPE_LIST_QUICKLIST_2: u8 = 18;

/// RDB 长度编码类型
pub enum RdbLengthKind: u8 {
    plain_len = 0,
    int8 = 1,
    int16 = 2,
    int32 = 3,
    lzf = 4,
}

/// RDB 流式解析事件类型
pub enum RdbEventKind: u8 {
    aux = 0,
    select_db = 1,
    resize_db = 2,
    string_kv = 3,
    eof = 4,
}

/// RDB 文件头（`"REDIS"` + 4 位 ASCII 版本号）
pub struct RdbHeader {
    version: u32,
}

/// RDB 长度解码结果
pub struct RdbLength {
    kind: RdbLengthKind,
    value: u64,
    next_pos: usize,
}

/// RDB 字符串/紧凑整数/LZF 压缩串零拷贝视图
pub struct RdbStringVal {
    kind: RdbLengthKind,
    raw: []const u8,
    int_val: i64,
    uncompressed_len: u64,
    next_pos: usize,
}

/// RDB 流式解析事件
pub struct RdbEvent {
    kind: RdbEventKind,
    db_index: u64,
    db_size: u64,
    expires_size: u64,
    key: RdbStringVal,
    value: RdbStringVal,
    expire_ms: u64?,
    checksum: u64,
}

fn emptyStringVal(): RdbStringVal {
    return RdbStringVal {
        kind: RdbLengthKind.plain_len,
        raw: "",
        int_val: 0,
        uncompressed_len: 0,
        next_pos: 0,
    };
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU64LE(data: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(data, off) as u64;
    const hi: u64 = readU32LE(data, off + 4) as u64;
    return lo | (hi << 32);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU64BE(data: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(data, off) as u64;
    const lo: u64 = readU32BE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

/// 校验字节切片是否以 `"REDIS"` + 4 位 ASCII 数字开头
pub fn isRdb(data: []const u8): bool {
    return parseRdbHeader(data) != null;
}

/// 解析 9 字节 RDB 文件头（`"REDIS"` + 4 位 ASCII 数字版本号）
pub fn parseRdbHeader(data: []const u8): RdbHeader? {
    if (data.len < RDB_HEADER_SIZE) return null;
    if (data[0] != 'R' || data[1] != 'E' || data[2] != 'D' || data[3] != 'I' || data[4] != 'S') {
        return null;
    }

    let ver: u32 = 0;
    let i: usize = 5;
    while (i < 9) {
        const c = data[i];
        if (c < '0' || c > '9') return null;
        ver = ver * 10 + ((c - '0') as u32);
        i += 1;
    }
    return RdbHeader { version: ver };
}

/// 解码 RDB 变长整数 / 特殊编码前缀（支持 6-bit, 14-bit BE, 32-bit BE, 64-bit BE 及 int8/16/32/LZF）
pub fn readRdbLength(data: []const u8, pos: usize): RdbLength? {
    if (pos >= data.len) return null;
    const first = data[pos];
    const tag = (first >> 6) & 0x03;

    if (tag == 0) {
        return RdbLength {
            kind: RdbLengthKind.plain_len,
            value: (first & 0x3F) as u64,
            next_pos: pos + 1,
        };
    }

    if (tag == 1) {
        if (data.len - pos < 2) return null;
        const hi: u64 = (first & 0x3F) as u64;
        const lo: u64 = data[pos + 1] as u64;
        return RdbLength {
            kind: RdbLengthKind.plain_len,
            value: (hi << 8) | lo,
            next_pos: pos + 2,
        };
    }

    if (tag == 2) {
        if (first == 0x80) {
            if (data.len - pos < 5) return null;
            return RdbLength {
                kind: RdbLengthKind.plain_len,
                value: readU32BE(data, pos + 1) as u64,
                next_pos: pos + 5,
            };
        }
        if (first == 0x81) {
            if (data.len - pos < 9) return null;
            return RdbLength {
                kind: RdbLengthKind.plain_len,
                value: readU64BE(data, pos + 1),
                next_pos: pos + 9,
            };
        }
        return null;
    }

    // tag == 3: special encoding
    const enc = first & 0x3F;
    if (enc == 0) {
        return RdbLength { kind: RdbLengthKind.int8, value: 0, next_pos: pos + 1 };
    }
    if (enc == 1) {
        return RdbLength { kind: RdbLengthKind.int16, value: 0, next_pos: pos + 1 };
    }
    if (enc == 2) {
        return RdbLength { kind: RdbLengthKind.int32, value: 0, next_pos: pos + 1 };
    }
    if (enc == 3) {
        return RdbLength { kind: RdbLengthKind.lzf, value: 0, next_pos: pos + 1 };
    }
    return null;
}

/// 读取 RDB 字符串值（支持普通字节串、int8/int16/int32 紧凑整数编码与 LZF 压缩串边界）
pub fn readRdbString(data: []const u8, pos: usize): RdbStringVal? {
    let hdr = RdbLength { kind: RdbLengthKind.plain_len, value: 0, next_pos: 0 };
    if (l := readRdbLength(data, pos)) {
        hdr = l;
    } else {
        return null;
    }

    const p = hdr.next_pos;
    if (hdr.kind == RdbLengthKind.plain_len) {
        if (hdr.value > ((data.len - p) as u64)) return null;
        const n = hdr.value as usize;
        return RdbStringVal {
            kind: RdbLengthKind.plain_len,
            raw: data[p .. p + n],
            int_val: 0,
            uncompressed_len: hdr.value,
            next_pos: p + n,
        };
    }

    if (hdr.kind == RdbLengthKind.int8) {
        if (data.len - p < 1) return null;
        const u = data[p];
        const iv: i64 = if (u >= 128) ((u as i64) - 256) else (u as i64);
        return RdbStringVal {
            kind: RdbLengthKind.int8,
            raw: data[p .. p + 1],
            int_val: iv,
            uncompressed_len: 1,
            next_pos: p + 1,
        };
    }

    if (hdr.kind == RdbLengthKind.int16) {
        if (data.len - p < 2) return null;
        const u = readU16LE(data, p);
        const iv: i64 = if (u >= 32768) ((u as i64) - 65536) else (u as i64);
        return RdbStringVal {
            kind: RdbLengthKind.int16,
            raw: data[p .. p + 2],
            int_val: iv,
            uncompressed_len: 2,
            next_pos: p + 2,
        };
    }

    if (hdr.kind == RdbLengthKind.int32) {
        if (data.len - p < 4) return null;
        const u = readU32LE(data, p);
        const iv: i64 = if (u >= 2147483648) ((u as i64) - 4294967296) else (u as i64);
        return RdbStringVal {
            kind: RdbLengthKind.int32,
            raw: data[p .. p + 4],
            int_val: iv,
            uncompressed_len: 4,
            next_pos: p + 4,
        };
    }

    // LZF compressed string: clen (plain_len) + ulen (plain_len) + compressed bytes
    let clen = RdbLength { kind: RdbLengthKind.plain_len, value: 0, next_pos: 0 };
    if (c := readRdbLength(data, p)) {
        if (c.kind != RdbLengthKind.plain_len) return null;
        clen = c;
    } else {
        return null;
    }

    let ulen = RdbLength { kind: RdbLengthKind.plain_len, value: 0, next_pos: 0 };
    if (u := readRdbLength(data, clen.next_pos)) {
        if (u.kind != RdbLengthKind.plain_len) return null;
        ulen = u;
    } else {
        return null;
    }

    const data_start = ulen.next_pos;
    if (clen.value > ((data.len - data_start) as u64)) return null;
    const cn = clen.value as usize;

    return RdbStringVal {
        kind: RdbLengthKind.lzf,
        raw: data[data_start .. data_start + cn],
        int_val: 0,
        uncompressed_len: ulen.value,
        next_pos: data_start + cn,
    };
}

/// 零堆分配 Redis RDB 流式解析器
pub struct RdbReader {
    data: []const u8,
    pos: usize = RDB_HEADER_SIZE,
    version: u32 = 0,
    pending_expire_ms: u64? = null,
    done: bool = false,
    had_error: bool = false,

    static fn init(data: []const u8): RdbReader {
        if (hdr := parseRdbHeader(data)) {
            return RdbReader {
                data: data,
                pos: RDB_HEADER_SIZE,
                version: hdr.version,
                pending_expire_ms: null,
                done: false,
                had_error: false,
            };
        }
        return RdbReader {
            data: data,
            pos: 0,
            version: 0,
            pending_expire_ms: null,
            done: true,
            had_error: true,
        };
    }

    fn next(*_): RdbEvent? {
        if (_.done || _.had_error) return null;

        while (_.pos < _.data.len) {
            const op = _.data[_.pos];
            _.pos += 1;

            if (op == OP_EXPIRETIME) {
                if (_.data.len - _.pos < 4) {
                    _.had_error = true;
                    return null;
                }
                const secs = readU32LE(_.data, _.pos);
                _.pos += 4;
                _.pending_expire_ms = (secs as u64) * 1000;
                continue;
            }

            if (op == OP_EXPIRETIME_MS) {
                if (_.data.len - _.pos < 8) {
                    _.had_error = true;
                    return null;
                }
                const ms = readU64LE(_.data, _.pos);
                _.pos += 8;
                _.pending_expire_ms = ms;
                continue;
            }

            if (op == OP_IDLE) {
                if (idle := readRdbLength(_.data, _.pos)) {
                    if (idle.kind != RdbLengthKind.plain_len) {
                        _.had_error = true;
                        return null;
                    }
                    _.pos = idle.next_pos;
                    continue;
                }
                _.had_error = true;
                return null;
            }

            if (op == OP_FREQ) {
                if (_.pos >= _.data.len) {
                    _.had_error = true;
                    return null;
                }
                _.pos += 1;
                continue;
            }

            if (op == OP_AUX) {
                let k = emptyStringVal();
                if (ks := readRdbString(_.data, _.pos)) {
                    k = ks;
                } else {
                    _.had_error = true;
                    return null;
                }
                let v = emptyStringVal();
                if (vs := readRdbString(_.data, k.next_pos)) {
                    v = vs;
                } else {
                    _.had_error = true;
                    return null;
                }
                _.pos = v.next_pos;
                return RdbEvent {
                    kind: RdbEventKind.aux,
                    db_index: 0,
                    db_size: 0,
                    expires_size: 0,
                    key: k,
                    value: v,
                    expire_ms: null,
                    checksum: 0,
                };
            }

            if (op == OP_SELECTDB) {
                if (db := readRdbLength(_.data, _.pos)) {
                    if (db.kind != RdbLengthKind.plain_len) {
                        _.had_error = true;
                        return null;
                    }
                    _.pos = db.next_pos;
                    return RdbEvent {
                        kind: RdbEventKind.select_db,
                        db_index: db.value,
                        db_size: 0,
                        expires_size: 0,
                        key: emptyStringVal(),
                        value: emptyStringVal(),
                        expire_ms: null,
                        checksum: 0,
                    };
                }
                _.had_error = true;
                return null;
            }

            if (op == OP_RESIZEDB) {
                let db_sz: u64 = 0;
                let next_p = _.pos;
                if (l1 := readRdbLength(_.data, next_p)) {
                    if (l1.kind != RdbLengthKind.plain_len) {
                        _.had_error = true;
                        return null;
                    }
                    db_sz = l1.value;
                    next_p = l1.next_pos;
                } else {
                    _.had_error = true;
                    return null;
                }
                if (l2 := readRdbLength(_.data, next_p)) {
                    if (l2.kind != RdbLengthKind.plain_len) {
                        _.had_error = true;
                        return null;
                    }
                    _.pos = l2.next_pos;
                    return RdbEvent {
                        kind: RdbEventKind.resize_db,
                        db_index: 0,
                        db_size: db_sz,
                        expires_size: l2.value,
                        key: emptyStringVal(),
                        value: emptyStringVal(),
                        expire_ms: null,
                        checksum: 0,
                    };
                }
                _.had_error = true;
                return null;
            }

            if (op == RDB_TYPE_STRING) {
                let k = emptyStringVal();
                if (ks := readRdbString(_.data, _.pos)) {
                    k = ks;
                } else {
                    _.had_error = true;
                    return null;
                }
                let v = emptyStringVal();
                if (vs := readRdbString(_.data, k.next_pos)) {
                    v = vs;
                } else {
                    _.had_error = true;
                    return null;
                }
                _.pos = v.next_pos;
                const exp = _.pending_expire_ms;
                _.pending_expire_ms = null;
                return RdbEvent {
                    kind: RdbEventKind.string_kv,
                    db_index: 0,
                    db_size: 0,
                    expires_size: 0,
                    key: k,
                    value: v,
                    expire_ms: exp,
                    checksum: 0,
                };
            }

            if (op == OP_EOF) {
                let csum: u64 = 0;
                if (_.version >= 5) {
                    if (_.data.len - _.pos < 8) {
                        _.had_error = true;
                        return null;
                    }
                    csum = readU64LE(_.data, _.pos);
                    _.pos += 8;
                }
                _.done = true;
                return RdbEvent {
                    kind: RdbEventKind.eof,
                    db_index: 0,
                    db_size: 0,
                    expires_size: 0,
                    key: emptyStringVal(),
                    value: emptyStringVal(),
                    expire_ms: null,
                    checksum: csum,
                };
            }

            // Unsupported or invalid opcode
            _.had_error = true;
            return null;
        }

        _.had_error = true;
        return null;
    }
}
