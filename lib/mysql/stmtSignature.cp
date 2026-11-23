// Source: Absorbed from /data/dev/bun/src/sql_jsc/mysql/protocol/Signature.cp | Synced: 2026-11-23T01:59:25Z
// Pure-CP zero-allocation MySQL prepared statement signature & cache key generator.
// Encodes signed/unsigned parameter type descriptors and computes deterministic 64-bit hashes
// for prepared statement caching without any dynamic heap allocation.

/// 单条 MySQL 预处理语句签名支持的最大参数槽位数（零堆分配内联上限）
pub const MYSQL_STMT_SIG_MAX_PARAMS: usize = 32;

/// 签名键缓冲区最大字节数（`query + 类型签名后缀序列`）
pub const MYSQL_STMT_SIG_MAX_NAME_LEN: usize = 512;

/// `MySqlStmtSignature` 构造错误集
pub err MySqlStmtSignatureError {
    TooManyParams,
    SignatureTooLong,
}

/// MySQL 预处理语句参数绑定字段类型（对应 MySQL 线协议 `FieldType` 枚举值）
pub enum MySqlSigFieldType: u8 {
    Decimal = 0x00,
    Tiny = 0x01,
    Short = 0x02,
    Long = 0x03,
    Float = 0x04,
    Double = 0x05,
    Null = 0x06,
    Timestamp = 0x07,
    LongLong = 0x08,
    Date = 0x0A,
    Time = 0x0B,
    DateTime = 0x0C,
    Json = 0xF5,
    NewDecimal = 0xF6,
    Blob = 0xFC,
    VarString = 0xFD,
    String = 0xFE,

    /// 返回标准 `MYSQL_TYPE_*` 名称（对齐 Bun `@tagName(tag)`）
    fn tagName(): []const u8 {
        if (_ == MySqlSigFieldType.Decimal) {
            return "MYSQL_TYPE_DECIMAL";
        }
        if (_ == MySqlSigFieldType.Tiny) {
            return "MYSQL_TYPE_TINY";
        }
        if (_ == MySqlSigFieldType.Short) {
            return "MYSQL_TYPE_SHORT";
        }
        if (_ == MySqlSigFieldType.Long) {
            return "MYSQL_TYPE_LONG";
        }
        if (_ == MySqlSigFieldType.Float) {
            return "MYSQL_TYPE_FLOAT";
        }
        if (_ == MySqlSigFieldType.Double) {
            return "MYSQL_TYPE_DOUBLE";
        }
        if (_ == MySqlSigFieldType.Null) {
            return "MYSQL_TYPE_NULL";
        }
        if (_ == MySqlSigFieldType.Timestamp) {
            return "MYSQL_TYPE_TIMESTAMP";
        }
        if (_ == MySqlSigFieldType.LongLong) {
            return "MYSQL_TYPE_LONGLONG";
        }
        if (_ == MySqlSigFieldType.Date) {
            return "MYSQL_TYPE_DATE";
        }
        if (_ == MySqlSigFieldType.Time) {
            return "MYSQL_TYPE_TIME";
        }
        if (_ == MySqlSigFieldType.DateTime) {
            return "MYSQL_TYPE_DATETIME";
        }
        if (_ == MySqlSigFieldType.Json) {
            return "MYSQL_TYPE_JSON";
        }
        if (_ == MySqlSigFieldType.NewDecimal) {
            return "MYSQL_TYPE_NEWDECIMAL";
        }
        if (_ == MySqlSigFieldType.Blob) {
            return "MYSQL_TYPE_BLOB";
        }
        if (_ == MySqlSigFieldType.VarString) {
            return "MYSQL_TYPE_VAR_STRING";
        }
        return "MYSQL_TYPE_STRING";
    }
}

/// MySQL 预处理语句参数类型与符号位描述符（对应 Bun `MySQLStatement.Param`）
pub struct MySqlSigParam {
    field_type: MySqlSigFieldType = MySqlSigFieldType.Null,
    is_unsigned: bool = false,

    /// 构造 `MYSQL_TYPE_NULL` 参数描述符
    static fn nullParam(): MySqlSigParam {
        return MySqlSigParam{
            field_type: MySqlSigFieldType.Null,
            is_unsigned: false,
        };
    }

    /// 构造有符号参数描述符
    static fn signed(ft: MySqlSigFieldType): MySqlSigParam {
        return MySqlSigParam{
            field_type: ft,
            is_unsigned: false,
        };
    }

    /// 构造无符号参数描述符（`MYSQL_TYPE_NULL` 强制忽略无符号位）
    static fn unsigned(ft: MySqlSigFieldType): MySqlSigParam {
        if (ft == MySqlSigFieldType.Null) {
            return MySqlSigParam.nullParam();
        }
        return MySqlSigParam{
            field_type: ft,
            is_unsigned: true,
        };
    }

    /// 返回 `COM_STMT_EXECUTE` 类型段高字节标志位（无符号 `0x80`，有符号 `0x00`）
    fn wireFlagByte(): u8 {
        return if (_.is_unsigned) 0x80 else 0x00;
    }

    /// 比较两个参数描述符是否相等
    fn eql(other: MySqlSigParam): bool {
        return _.field_type == other.field_type && _.is_unsigned == other.is_unsigned;
    }
}

fn appendSlice(dst: []u8, pos: usize, src: []const u8): MySqlStmtSignatureError!usize {
    if (pos + src.len > dst.len) {
        return MySqlStmtSignatureError.SignatureTooLong;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
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

/// MySQL 预处理语句缓存签名（零动态堆分配，对应 Bun `Signature`）。
pub struct MySqlStmtSignature {
    fields_buf: [MYSQL_STMT_SIG_MAX_PARAMS]MySqlSigParam = [MySqlSigParam{}; MYSQL_STMT_SIG_MAX_PARAMS],
    field_count: usize = 0,
    name_buf: [MYSQL_STMT_SIG_MAX_NAME_LEN]u8 = [0; MYSQL_STMT_SIG_MAX_NAME_LEN],
    name_len: usize = 0,
    query_len: usize = 0,

    /// 返回空签名实例
    static fn empty(): MySqlStmtSignature {
        return MySqlStmtSignature{};
    }

    /// 根据 SQL 查询文本与参数描述符列表构建签名（完全对齐 Bun `Signature.generate` 后缀规则）。
    static fn build(query: []const u8, params: []const MySqlSigParam): MySqlStmtSignatureError!MySqlStmtSignature {
        if (params.len > MYSQL_STMT_SIG_MAX_PARAMS) {
            return MySqlStmtSignatureError.TooManyParams;
        }

        let sig = MySqlStmtSignature{};
        let pos = appendSlice(sig.name_buf[0..], 0, query)?;
        sig.query_len = query.len;

        let i: usize = 0;
        while (i < params.len) {
            const p = params[i];
            if (p.field_type == MySqlSigFieldType.Null) {
                pos = appendSlice(sig.name_buf[0..], pos, ".null")?;
                sig.fields_buf[i] = MySqlSigParam.nullParam();
            } else {
                if (p.is_unsigned) {
                    pos = appendSlice(sig.name_buf[0..], pos, "U")?;
                }
                pos = appendSlice(sig.name_buf[0..], pos, p.field_type.tagName())?;
                sig.fields_buf[i] = p;
            }
            i += 1;
        }

        sig.name_len = pos;
        sig.field_count = params.len;
        return sig;
    }

    /// 返回完整签名键切片（`query + 类型标记序列`）
    fn nameSlice(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    /// 返回原始 SQL 查询文本切片（从 `name_buf` 零拷贝切出前缀）
    fn querySlice(): []const u8 {
        return _.name_buf[0.._.query_len];
    }

    /// 返回参数描述符切片
    fn fields(): []const MySqlSigParam {
        return _.fields_buf[0.._.field_count];
    }

    /// 返回参数个数
    fn fieldCount(): usize {
        return _.field_count;
    }

    /// 签名是否为空
    fn isEmpty(): bool {
        return _.name_len == 0 && _.field_count == 0;
    }

    /// 计算用于预处理语句缓存表查找的确定性 64 位哈希（同时覆盖 `nameSlice()` 与 `fields()`）
    fn hash(): u64 {
        let h: u64 = 0xCBF29CE484222325;
        h = fnv1aUpdate(h, _.nameSlice());
        let i: usize = 0;
        while (i < _.field_count) {
            const pair: [2]u8 = [
                @as(u8, @intFromEnum(_.fields_buf[i].field_type)),
                _.fields_buf[i].wireFlagByte(),
            ];
            h = fnv1aUpdate(h, pair[0..]);
            i += 1;
        }
        return h;
    }

    /// 比较两条签名是否等价
    fn eql(other: MySqlStmtSignature): bool {
        if (!bytesEql(_.nameSlice(), other.nameSlice())) {
            return false;
        }
        if (_.field_count != other.field_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.field_count) {
            if (!_.fields_buf[i].eql(other.fields_buf[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}
