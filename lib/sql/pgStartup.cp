// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/StartupMessage.cp | Synced: 2026-11-22T05:30:00Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 StartupMessage / SSL / Cancel Codec
// Absorbed from Bun: src/sql/postgres/protocol/StartupMessage.cp
// ============================================================================

import { PgWireWriter } from "./pgWriter.cp";
import { PgWireReader } from "./pgReader.cp";

/// PostgreSQL 前端协议 3.0 版本号（`(3 << 16) | 0 = 196608`）
pub const PG_PROTOCOL_V3_0: u32 = 196608;

/// PostgreSQL SSLRequest 魔数（`(1234 << 16) | 5679 = 80877103`）
pub const PG_SSL_REQUEST_CODE: u32 = 80877103;

/// PostgreSQL CancelRequest 魔数（`(1234 << 16) | 5678 = 80877102`）
pub const PG_CANCEL_REQUEST_CODE: u32 = 80877102;

/// 单次 StartupMessage 最大参数对上限（防异常大包资源耗尽）
pub const PG_MAX_STARTUP_PARAMS: usize = 32;

/// PostgreSQL Startup / SSL / Cancel 编解码错误集
pub err PgStartupCodecError {
    BufferTooSmall,
    MissingUser,
    InvalidMessage,
    InvalidLength,
    UnsupportedProtocol,
    TruncatedPayload,
    UnterminatedParam,
    TooManyParams,
}

/// PostgreSQL 启动参数键值对（零拷贝切片）
pub struct PgStartupParam {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// PostgreSQL v3 `StartupMessage` 编码配置
pub struct PgStartupConfig {
    user: []const u8,
    database: []const u8 = "",
    client_encoding: []const u8 = "UTF8",
    application_name: []const u8 = "",
    extra_params: []const PgStartupParam = [],
}

/// 零拷贝 `StartupMessage` 参数对迭代器
pub struct PgStartupParamIter {
    reader: PgWireReader,
    done: bool = false,

    static fn init(params_payload: []const u8): PgStartupParamIter {
        return PgStartupParamIter{
            reader: PgWireReader.init(params_payload),
            done: false,
        };
    }

    fn next(*_): PgStartupParam?! {
        if (_.done) {
            return null;
        }
        if (_.reader.remaining() == 0) {
            _.done = true;
            return err.UnterminatedParam;
        }
        // 末尾单字节 NUL 标志参数表结束
        const peek_slice = _.reader.peek();
        if (peek_slice[0] == 0) {
            * = _.reader.skip(1);
            _.done = true;
            return null;
        }
        const k = _.reader.readZ() catch {
            _.done = true;
            return err.UnterminatedParam;
        };
        const v = _.reader.readZ() catch {
            _.done = true;
            return err.UnterminatedParam;
        };
        return PgStartupParam{
            key: k,
            value: v,
        };
    }
}

/// 零拷贝解码后的 PostgreSQL `StartupMessage` 视图
pub struct PgDecodedStartup {
    protocol_version: u32 = 0,
    total_len: usize = 0,
    user: []const u8 = "",
    database: []const u8 = "",
    client_encoding: []const u8 = "",
    application_name: []const u8 = "",
    param_count: usize = 0,
    params_payload: []const u8 = "",

    /// 返回覆盖全部启动参数对的零拷贝迭代器
    fn iterParams(): PgStartupParamIter {
        return PgStartupParamIter.init(_.params_payload);
    }
}

/// 编码 PostgreSQL v3 `StartupMessage`（4B BE 总长 + `196608` + 参数对 + 尾随 `0x00`），零堆分配
///
/// 对齐 Bun `StartupMessage.writeInternal` 约定：
/// - `cfg.database` 为空时自动回退为 `cfg.user`；
/// - `cfg.client_encoding` 默认写入 `"UTF8"`；
/// - 可选写入 `application_name` 与 `extra_params`。
pub fn encodePgStartup(buf: []u8, cfg: PgStartupConfig): []const u8! {
    if (cfg.user.len == 0) {
        return err.MissingUser;
    }
    let total_params: usize = 2; // user + database
    if (cfg.client_encoding.len > 0) {
        total_params += 1;
    }
    if (cfg.application_name.len > 0) {
        total_params += 1;
    }
    total_params += cfg.extra_params.len;
    if (total_params > PG_MAX_STARTUP_PARAMS) {
        return err.TooManyParams;
    }

    let w = PgWireWriter.init(buf);
    const patch = w.beginLength() catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(PG_PROTOCOL_V3_0) catch {
        return err.BufferTooSmall;
    };

    // user
    * = w.writeCString("user") catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(cfg.user) catch {
        return err.BufferTooSmall;
    };

    // database (空则回退为 user)
    const eff_db = if (cfg.database.len == 0) cfg.user else cfg.database;
    * = w.writeCString("database") catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(eff_db) catch {
        return err.BufferTooSmall;
    };

    // client_encoding
    if (cfg.client_encoding.len > 0) {
        * = w.writeCString("client_encoding") catch {
            return err.BufferTooSmall;
        };
        * = w.writeCString(cfg.client_encoding) catch {
            return err.BufferTooSmall;
        };
    }

    // application_name
    if (cfg.application_name.len > 0) {
        * = w.writeCString("application_name") catch {
            return err.BufferTooSmall;
        };
        * = w.writeCString(cfg.application_name) catch {
            return err.BufferTooSmall;
        };
    }

    // extra_params
    let i: usize = 0;
    while (i < cfg.extra_params.len) {
        const p = cfg.extra_params[i];
        i += 1;
        if (p.key.len == 0) {
            continue;
        }
        * = w.writeCString(p.key) catch {
            return err.BufferTooSmall;
        };
        * = w.writeCString(p.value) catch {
            return err.BufferTooSmall;
        };
    }

    // 终止符 NUL
    * = w.writeByte(0) catch {
        return err.BufferTooSmall;
    };

    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 零拷贝解码并校验 PostgreSQL v3 `StartupMessage`
pub fn decodePgStartup(packet: []const u8): PgDecodedStartup! {
    if (packet.len < 9) {
        return err.InvalidMessage;
    }
    let r = PgWireReader.init(packet);
    const total_len_u32 = r.readU32() catch {
        return err.InvalidMessage;
    };
    const total_len: usize = @intCast(total_len_u32);
    if (total_len < 9) {
        return err.InvalidLength;
    }
    if (total_len > packet.len) {
        return err.TruncatedPayload;
    }

    const version = r.readU32() catch {
        return err.TruncatedPayload;
    };
    if (version != PG_PROTOCOL_V3_0) {
        return err.UnsupportedProtocol;
    }

    const params_slice = packet[8..total_len];
    let it = PgStartupParamIter.init(params_slice);

    let res = PgDecodedStartup{
        protocol_version: version,
        total_len: total_len,
        params_payload: params_slice,
    };

    while (maybe_p := it.next()?) {
        res.param_count += 1;
        if (res.param_count > PG_MAX_STARTUP_PARAMS) {
            return err.TooManyParams;
        }
        if (std.mem.eql(u8, maybe_p.key, "user")) {
            res.user = maybe_p.value;
        } else if (std.mem.eql(u8, maybe_p.key, "database")) {
            res.database = maybe_p.value;
        } else if (std.mem.eql(u8, maybe_p.key, "client_encoding")) {
            res.client_encoding = maybe_p.value;
        } else if (std.mem.eql(u8, maybe_p.key, "application_name")) {
            res.application_name = maybe_p.value;
        }
    }

    return res;
}

/// 编码 8 字节 PostgreSQL `SSLRequest` 帧（零堆分配）
pub fn encodePgSslRequestFrame(buf: []u8): []const u8! {
    let w = PgWireWriter.init(buf);
    * = w.writeU32(8) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(PG_SSL_REQUEST_CODE) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 判断给定字节切片是否为合法的 8 字节 PostgreSQL `SSLRequest` 帧
pub fn isPgSslRequestFrame(packet: []const u8): bool {
    if (packet.len < 8) {
        return false;
    }
    let r = PgWireReader.init(packet);
    const len = r.readU32() catch return false;
    const code = r.readU32() catch return false;
    return len == 8 && code == PG_SSL_REQUEST_CODE;
}

/// 编码 16 字节 PostgreSQL `CancelRequest` 帧（零堆分配）
pub fn encodePgCancelFrame(buf: []u8, process_id: u32, secret_key: u32): []const u8! {
    let w = PgWireWriter.init(buf);
    * = w.writeU32(16) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(PG_CANCEL_REQUEST_CODE) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(process_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(secret_key) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}
