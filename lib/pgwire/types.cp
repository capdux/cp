// Source: cp/lib/pgwire/types.cp (PostgreSQL v3.0 Wire Protocol Types & Errors)

const std = @import("std");

pub const PG_PROTOCOL_VERSION_3_0: u32 = 0x00030000;
pub const PG_SSL_REQUEST_CODE: u32 = 80877103;
pub const DEFAULT_MAX_PACKET_BYTES: usize = 16 * 1024 * 1024;

pub const PgError = error{
    InvalidPacket,
    PacketTooLarge,
    Incomplete,
    UnexpectedEof,
    InvalidTag,
    OutOfMemory,
};

pub const PgTxStatus = enum(u8) {
    Idle = 'I',
    InTransaction = 'T',
    FailedTransaction = 'E',

    pub fn fromByte(b: u8): !PgTxStatus {
        return switch (b) {
            'I' => PgTxStatus.Idle,
            'T' => PgTxStatus.InTransaction,
            'E' => PgTxStatus.FailedTransaction,
            else => PgError.InvalidPacket,
        };
    }
};

pub struct PgKv {
    key: []const u8,
    value: []const u8,
}

pub struct PgColumnDesc {
    name: []const u8,
    table_oid: u32,
    column_attr: i16,
    type_oid: u32,
    type_size: i16,
    type_modifier: i32,
    format_code: i16,
}

pub struct PgFieldError {
    severity: []const u8,
    code: []const u8,
    message: []const u8,
    detail: ([]const u8)?,
    hint: ([]const u8)?,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.severity.len > 0) {
            alloc.free(_.severity);
        }
        if (_.code.len > 0) {
            alloc.free(_.code);
        }
        if (_.message.len > 0) {
            alloc.free(_.message);
        }
        if (d := _.detail) {
            alloc.free(d);
        }
        if (h := _.hint) {
            alloc.free(h);
        }
    }
}

pub union(enum) PgAuthKind {
    ok: void,
    kerberos_v5: void,
    cleartext_password: void,
    md5_password: [4]u8,
    scm_credential: void,
    gss: void,
    sspi: void,
    sasl: []const []const u8,
    sasl_continue: []const u8,
    sasl_final: []const u8,
    unknown: u32,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .sasl => |mechs| {
                for (m of mechs) {
                    alloc.free(m);
                }
                alloc.free(mechs);
            },
            .sasl_continue => |data| {
                alloc.free(data);
            },
            .sasl_final => |data| {
                alloc.free(data);
            },
            else => {},
        }
    }
}

pub struct PgParseMsg {
    statement: []const u8,
    query: []const u8,
    param_types: []const u32,
}

pub struct PgBindMsg {
    portal: []const u8,
    statement: []const u8,
    param_formats: []const i16,
    params: []const ([]const u8)?,
    result_formats: []const i16,
}

pub struct PgExecuteMsg {
    portal: []const u8,
    max_rows: i32,
}

pub struct PgParameterStatusMsg {
    name: []const u8,
    value: []const u8,
}

pub struct PgBackendKeyDataMsg {
    process_id: i32,
    secret_key: i32,
}

pub struct PgUnknownMsg {
    tag: u8,
    payload: []const u8,
}

pub union(enum) PgFrontendMsg {
    startup: []const PgKv,
    ssl_request: void,
    query: []const u8,
    parse: PgParseMsg,
    bind: PgBindMsg,
    execute: PgExecuteMsg,
    sync: void,
    terminate: void,
    password: []const u8,
    unknown: PgUnknownMsg,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .startup => |kvs| {
                for (kv of kvs) {
                    alloc.free(kv.key);
                    alloc.free(kv.value);
                }
                alloc.free(kvs);
            },
            .query => |q| {
                alloc.free(q);
            },
            .parse => |p| {
                alloc.free(p.statement);
                alloc.free(p.query);
                alloc.free(p.param_types);
            },
            .bind => |b| {
                alloc.free(b.portal);
                alloc.free(b.statement);
                alloc.free(b.param_formats);
                for (param_opt of b.params) {
                    if (param := param_opt) {
                        alloc.free(param);
                    }
                }
                alloc.free(b.params);
                alloc.free(b.result_formats);
            },
            .execute => |e| {
                alloc.free(e.portal);
            },
            .password => |pw| {
                alloc.free(pw);
            },
            .unknown => |u| {
                alloc.free(u.payload);
            },
            .ssl_request, .sync, .terminate => {},
        }
    }
}

pub union(enum) PgBackendMsg {
    authentication: PgAuthKind,
    parameter_status: PgParameterStatusMsg,
    backend_key_data: PgBackendKeyDataMsg,
    ready_for_query: PgTxStatus,
    row_description: []const PgColumnDesc,
    data_row: []const ([]const u8)?,
    command_complete: []const u8,
    empty_query_response: void,
    parse_complete: void,
    bind_complete: void,
    error_response: PgFieldError,
    notice_response: PgFieldError,
    unknown: PgUnknownMsg,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .authentication => |*auth| {
                auth.deinit(alloc);
            },
            .parameter_status => |ps| {
                alloc.free(ps.name);
                alloc.free(ps.value);
            },
            .row_description => |cols| {
                for (col of cols) {
                    alloc.free(col.name);
                }
                alloc.free(cols);
            },
            .data_row => |row| {
                for (cell_opt of row) {
                    if (cell := cell_opt) {
                        alloc.free(cell);
                    }
                }
                alloc.free(row);
            },
            .command_complete => |tag| {
                alloc.free(tag);
            },
            .error_response => |*err_msg| {
                err_msg.deinit(alloc);
            },
            .notice_response => |*notice_msg| {
                notice_msg.deinit(alloc);
            },
            .unknown => |u| {
                alloc.free(u.payload);
            },
            .backend_key_data, .ready_for_query, .empty_query_response, .parse_complete, .bind_complete => {},
        }
    }
}
