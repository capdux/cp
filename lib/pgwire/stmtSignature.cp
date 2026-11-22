// Source: Absorbed from /data/dev/bun/src/sql_jsc/postgres/Signature.cp | Synced: 2026-11-22T21:33:59Z
// Pure-CP zero-allocation PostgreSQL prepared statement signature & cache key generator.
// Computes deterministic parameter-typed signature keys, PostgreSQL wire parameter OID lists,
// bounded prepared statement identifiers ("P<prefix>$<id>"), and 64-bit lookup hashes.

/// 单条预处理语句签名支持的最大绑定参数个数（零堆分配内联上限）
pub const PG_STMT_SIG_MAX_PARAMS: usize = 32;

/// 签名键缓冲区最大字节数（`query + ".int4.string..."`）
pub const PG_STMT_SIG_MAX_NAME_LEN: usize = 512;

/// 预处理语句名称最大字节数（`"P" + 最多 40B 前缀 + "$" + 最多 20 位 u64 十进制`）
pub const PG_STMT_SIG_MAX_STMT_NAME_LEN: usize = 64;

/// 预处理语句名称中截取签名前缀的最大字节数（对齐 Bun `@min(40, name.len)`）
pub const PG_STMT_SIG_PREFIX_MAX: usize = 40;

/// `PgStmtSignature` 构造错误集
pub err PgStmtSignatureError {
    TooManyParams,
    SignatureTooLong,
}

/// PostgreSQL 参数绑定类型标签（对应 PostgreSQL 内建 OID）
pub enum PgParamTypeTag: i32 {
    Null = 0,
    Bool = 16,
    Bytea = 17,
    Int8 = 20,
    Int2 = 21,
    Int4 = 23,
    Text = 25,
    Json = 114,
    Float4 = 700,
    Float8 = 701,
    Timestamp = 1114,
    Timestamptz = 1184,
    Numeric = 1700,
    Jsonb = 3802,

    /// 返回追加到签名键尾部的规范类型后缀（完全对齐 Bun `Signature.generate`）
    fn typeSuffix(): []const u8 {
        if (_ == PgParamTypeTag.Null) {
            return ".null";
        }
        if (_ == PgParamTypeTag.Int8) {
            return ".int8";
        }
        if (_ == PgParamTypeTag.Int4) {
            return ".int4";
        }
        if (_ == PgParamTypeTag.Int2) {
            return ".int2";
        }
        if (_ == PgParamTypeTag.Float8) {
            return ".float8";
        }
        if (_ == PgParamTypeTag.Float4) {
            return ".float4";
        }
        if (_ == PgParamTypeTag.Numeric) {
            return ".numeric";
        }
        if (_ == PgParamTypeTag.Json || _ == PgParamTypeTag.Jsonb) {
            return ".json";
        }
        if (_ == PgParamTypeTag.Bool) {
            return ".bool";
        }
        if (_ == PgParamTypeTag.Timestamp) {
            return ".timestamp";
        }
        if (_ == PgParamTypeTag.Timestamptz) {
            return ".timestamptz";
        }
        if (_ == PgParamTypeTag.Bytea) {
            return ".bytea";
        }
        return ".string";
    }

    /// 返回 `Parse` 帧中发送给 PostgreSQL 的参数 OID：
    /// 仅基础标量/二进制类型显式传递 OID，其余（含 `Null`/`Text`/`Json`/`Timestamp*`）传 `0` 交由服务端推导。
    fn wireParamOid(): i32 {
        if (_ == PgParamTypeTag.Bool ||
            _ == PgParamTypeTag.Int4 ||
            _ == PgParamTypeTag.Int8 ||
            _ == PgParamTypeTag.Float8 ||
            _ == PgParamTypeTag.Int2 ||
            _ == PgParamTypeTag.Numeric ||
            _ == PgParamTypeTag.Float4 ||
            _ == PgParamTypeTag.Bytea) {
            return @as(i32, @intFromEnum(_));
        }
        return 0;
    }
}

fn appendSlice(dst: []u8, pos: usize, src: []const u8): PgStmtSignatureError!usize {
    if (pos + src.len > dst.len) {
        return PgStmtSignatureError.SignatureTooLong;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendU64Decimal(dst: []u8, pos: usize, value: u64): PgStmtSignatureError!usize {
    let digits: [20]u8 = [0; 20];
    let count: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (v > 0) {
            digits[count] = @as(u8, @intCast(v % 10)) + '0';
            v /= 10;
            count += 1;
        }
    }
    if (pos + count > dst.len) {
        return PgStmtSignatureError.SignatureTooLong;
    }
    let i: usize = 0;
    while (i < count) {
        dst[pos + i] = digits[count - 1 - i];
        i += 1;
    }
    return pos + count;
}

fn fnv1aUpdate(seed: u64, bytes: []const u8): u64 {
    let h = seed;
    let i: usize = 0;
    while (i < bytes.len) {
        h = (h ^ @as(u64, bytes[i])) *% 0x100000001B3;
        i += 1;
    }
    return h;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// PostgreSQL 预处理语句签名（零动态堆分配，对应 Bun `Signature`）。
pub struct PgStmtSignature {
    param_oids_buf: [PG_STMT_SIG_MAX_PARAMS]i32 = [0; PG_STMT_SIG_MAX_PARAMS],
    param_count: usize = 0,
    name_buf: [PG_STMT_SIG_MAX_NAME_LEN]u8 = [0; PG_STMT_SIG_MAX_NAME_LEN],
    name_len: usize = 0,
    prepared_name_buf: [PG_STMT_SIG_MAX_STMT_NAME_LEN]u8 = [0; PG_STMT_SIG_MAX_STMT_NAME_LEN],
    prepared_name_len: usize = 0,
    query_len: usize = 0,

    /// 返回空签名实例
    static fn empty(): PgStmtSignature {
        return PgStmtSignature{};
    }

    /// 根据 SQL 查询文本、绑定参数类型列表、预处理语句序号与 `unnamed` 标志构建签名。
    static fn build(
        query: []const u8,
        param_tags: []const PgParamTypeTag,
        prepared_statement_id: u64,
        unnamed: bool,
    ): PgStmtSignatureError!PgStmtSignature {
        if (param_tags.len > PG_STMT_SIG_MAX_PARAMS) {
            return PgStmtSignatureError.TooManyParams;
        }

        let sig = PgStmtSignature{};
        let pos = appendSlice(sig.name_buf[0..], 0, query)?;
        sig.query_len = query.len;

        let i: usize = 0;
        while (i < param_tags.len) {
            const tag = param_tags[i];
            pos = appendSlice(sig.name_buf[0..], pos, tag.typeSuffix())?;
            sig.param_oids_buf[i] = tag.wireParamOid();
            i += 1;
        }
        sig.name_len = pos;
        sig.param_count = param_tags.len;

        if (!unnamed) {
            const prefix_len: usize = if (sig.name_len < PG_STMT_SIG_PREFIX_MAX) {
                sig.name_len
            } else {
                PG_STMT_SIG_PREFIX_MAX
            };
            let ppos = appendSlice(sig.prepared_name_buf[0..], 0, "P")?;
            ppos = appendSlice(sig.prepared_name_buf[0..], ppos, sig.name_buf[0..prefix_len])?;
            ppos = appendSlice(sig.prepared_name_buf[0..], ppos, "$")?;
            ppos = appendU64Decimal(sig.prepared_name_buf[0..], ppos, prepared_statement_id)?;
            sig.prepared_name_len = ppos;
        }

        return sig;
    }

    /// 完整签名键切片（`query + 类型后缀序列`）
    fn nameSlice(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    /// 原始 SQL 查询文本切片（从 `name_buf` 零拷贝切出前缀）
    fn querySlice(): []const u8 {
        return _.name_buf[0.._.query_len];
    }

    /// 预处理语句名称切片（`unnamed` 时为空切片 `""`，否则为 `"P<prefix>$<id>"`）
    fn preparedNameSlice(): []const u8 {
        return _.prepared_name_buf[0.._.prepared_name_len];
    }

    /// `Parse` 帧参数 OID 切片
    fn paramOids(): []const i32 {
        return _.param_oids_buf[0.._.param_count];
    }

    /// 参数个数
    fn paramCount(): usize {
        return _.param_count;
    }

    /// 是否为未命名预处理语句
    fn isUnnamed(): bool {
        return _.prepared_name_len == 0;
    }

    /// 计算用于预处理语句缓存表查找的确定性 64 位哈希（同时覆盖 `nameSlice()` 与 `paramOids()`）
    fn hash(): u64 {
        let h: u64 = 0xCBF29CE484222325;
        h = fnv1aUpdate(h, _.nameSlice());
        let i: usize = 0;
        while (i < _.param_count) {
            const bits = @as(u32, @bitCast(_.param_oids_buf[i]));
            const le: [4]u8 = [
                @as(u8, @truncate(bits)),
                @as(u8, @truncate(bits >> 8)),
                @as(u8, @truncate(bits >> 16)),
                @as(u8, @truncate(bits >> 24)),
            ];
            h = fnv1aUpdate(h, le[0..]);
            i += 1;
        }
        return h;
    }

    /// 比较两条签名是否表示同一预处理语句缓存键（忽略 `prepared_statement_id` 差异）
    fn eql(other: PgStmtSignature): bool {
        if (!bytesEql(_.nameSlice(), other.nameSlice())) {
            return false;
        }
        if (_.param_count != other.param_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.param_count) {
            if (_.param_oids_buf[i] != other.param_oids_buf[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}
