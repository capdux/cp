// Source: cp/lib/pgwire/codec.cp (PostgreSQL v3.0 Wire Protocol Binary Codec)

const std = @import("std");
const types = @import("./types.cp");

pub const PgError = types.PgError;
pub const PgTxStatus = types.PgTxStatus;
pub const PgKv = types.PgKv;
pub const PgColumnDesc = types.PgColumnDesc;
pub const PgFieldError = types.PgFieldError;
pub const PgAuthKind = types.PgAuthKind;
pub const PgFrontendMsg = types.PgFrontendMsg;
pub const PgBackendMsg = types.PgBackendMsg;

fn _byteSliceDummy(): []const u8 {
    return "";
}
const ByteSlice = @TypeOf(_byteSliceDummy());
fn _optBytesDummy(): ([]const u8)? {
    return null;
}
const OptBytes = @TypeOf(_optBytesDummy());

fn writeU16(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u16): !void {
    let buf: [2]u8 = undefined;
    std.mem.writeInt(u16, &buf, val, .big);
    try list.appendSlice(alloc, &buf);
}

fn writeI16(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: i16): !void {
    let buf: [2]u8 = undefined;
    std.mem.writeInt(i16, &buf, val, .big);
    try list.appendSlice(alloc, &buf);
}

fn writeU32(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): !void {
    let buf: [4]u8 = undefined;
    std.mem.writeInt(u32, &buf, val, .big);
    try list.appendSlice(alloc, &buf);
}

fn writeI32(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: i32): !void {
    let buf: [4]u8 = undefined;
    std.mem.writeInt(i32, &buf, val, .big);
    try list.appendSlice(alloc, &buf);
}

fn writeCString(list: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): !void {
    try list.appendSlice(alloc, s);
    try list.append(alloc, 0);
}

fn patchLengthAt(list: *std.ArrayList(u8), offset: usize, len_val: usize) {
    let buf: [4]u8 = undefined;
    std.mem.writeInt(u32, &buf, @intCast(len_val), .big);
    @memcpy(list.items[offset .. offset + 4], &buf);
}

pub fn encodeFrontendMsg(alloc: std.mem.Allocator, msg: PgFrontendMsg): ![]u8 {
    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);

    switch (msg) {
        .startup => |kvs| {
            try writeU32(&list, alloc, 0);
            try writeU32(&list, alloc, types.PG_PROTOCOL_VERSION_3_0);
            for (kv of kvs) {
                try writeCString(&list, alloc, kv.key);
                try writeCString(&list, alloc, kv.value);
            }
            try list.append(alloc, 0);
            patchLengthAt(&list, 0, list.items.len);
        },
        .ssl_request => {
            try writeU32(&list, alloc, 8);
            try writeU32(&list, alloc, types.PG_SSL_REQUEST_CODE);
        },
        .query => |sql| {
            try list.append(alloc, 'Q');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, sql);
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .parse => |p| {
            try list.append(alloc, 'P');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, p.statement);
            try writeCString(&list, alloc, p.query);
            try writeU16(&list, alloc, @intCast(p.param_types.len));
            for (oid of p.param_types) {
                try writeU32(&list, alloc, oid);
            }
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .bind => |b| {
            try list.append(alloc, 'B');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, b.portal);
            try writeCString(&list, alloc, b.statement);
            try writeU16(&list, alloc, @intCast(b.param_formats.len));
            for (fmt of b.param_formats) {
                try writeI16(&list, alloc, fmt);
            }
            try writeU16(&list, alloc, @intCast(b.params.len));
            for (param_opt of b.params) {
                if (param := param_opt) {
                    try writeI32(&list, alloc, @intCast(param.len));
                    try list.appendSlice(alloc, param);
                } else {
                    try writeI32(&list, alloc, -1);
                }
            }
            try writeU16(&list, alloc, @intCast(b.result_formats.len));
            for (fmt of b.result_formats) {
                try writeI16(&list, alloc, fmt);
            }
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .execute => |e| {
            try list.append(alloc, 'E');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, e.portal);
            try writeI32(&list, alloc, e.max_rows);
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .sync => {
            try list.append(alloc, 'S');
            try writeU32(&list, alloc, 4);
        },
        .terminate => {
            try list.append(alloc, 'X');
            try writeU32(&list, alloc, 4);
        },
        .password => |pw| {
            try list.append(alloc, 'p');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, pw);
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .unknown => |u| {
            try list.append(alloc, u.tag);
            try writeU32(&list, alloc, @intCast(4 + u.payload.len));
            try list.appendSlice(alloc, u.payload);
        },
    }

    return try list.toOwnedSlice(alloc);
}

fn writeFieldError(list: *std.ArrayList(u8), alloc: std.mem.Allocator, tag: u8, err_msg: PgFieldError): !void {
    try list.append(alloc, tag);
    try writeU32(list, alloc, 0);
    if (err_msg.severity.len > 0) {
        try list.append(alloc, 'S');
        try writeCString(list, alloc, err_msg.severity);
    }
    if (err_msg.code.len > 0) {
        try list.append(alloc, 'C');
        try writeCString(list, alloc, err_msg.code);
    }
    if (err_msg.message.len > 0) {
        try list.append(alloc, 'M');
        try writeCString(list, alloc, err_msg.message);
    }
    if (d := err_msg.detail) {
        try list.append(alloc, 'D');
        try writeCString(list, alloc, d);
    }
    if (h := err_msg.hint) {
        try list.append(alloc, 'H');
        try writeCString(list, alloc, h);
    }
    try list.append(alloc, 0);
    patchLengthAt(list, 1, list.items.len - 1);
}

pub fn encodeBackendMsg(alloc: std.mem.Allocator, msg: PgBackendMsg): ![]u8 {
    let list = std.ArrayList(u8).empty;
    errdefer list.deinit(alloc);

    switch (msg) {
        .authentication => |auth| {
            try list.append(alloc, 'R');
            try writeU32(&list, alloc, 0);
            switch (auth) {
                .ok => try writeU32(&list, alloc, 0),
                .kerberos_v5 => try writeU32(&list, alloc, 2),
                .cleartext_password => try writeU32(&list, alloc, 3),
                .md5_password => |salt| {
                    try writeU32(&list, alloc, 5);
                    try list.appendSlice(alloc, &salt);
                },
                .scm_credential => try writeU32(&list, alloc, 6),
                .gss => try writeU32(&list, alloc, 7),
                .sspi => try writeU32(&list, alloc, 9),
                .sasl => |mechs| {
                    try writeU32(&list, alloc, 10);
                    for (m of mechs) {
                        try writeCString(&list, alloc, m);
                    }
                    try list.append(alloc, 0);
                },
                .sasl_continue => |data| {
                    try writeU32(&list, alloc, 11);
                    try list.appendSlice(alloc, data);
                },
                .sasl_final => |data| {
                    try writeU32(&list, alloc, 12);
                    try list.appendSlice(alloc, data);
                },
                .unknown => |code| try writeU32(&list, alloc, code),
            }
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .parameter_status => |ps| {
            try list.append(alloc, 'S');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, ps.name);
            try writeCString(&list, alloc, ps.value);
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .backend_key_data => |bk| {
            try list.append(alloc, 'K');
            try writeU32(&list, alloc, 12);
            try writeI32(&list, alloc, bk.process_id);
            try writeI32(&list, alloc, bk.secret_key);
        },
        .ready_for_query => |status| {
            try list.append(alloc, 'Z');
            try writeU32(&list, alloc, 5);
            try list.append(alloc, @intFromEnum(status));
        },
        .row_description => |cols| {
            try list.append(alloc, 'T');
            try writeU32(&list, alloc, 0);
            try writeU16(&list, alloc, @intCast(cols.len));
            for (col of cols) {
                try writeCString(&list, alloc, col.name);
                try writeU32(&list, alloc, col.table_oid);
                try writeI16(&list, alloc, col.column_attr);
                try writeU32(&list, alloc, col.type_oid);
                try writeI16(&list, alloc, col.type_size);
                try writeI32(&list, alloc, col.type_modifier);
                try writeI16(&list, alloc, col.format_code);
            }
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .data_row => |row| {
            try list.append(alloc, 'D');
            try writeU32(&list, alloc, 0);
            try writeU16(&list, alloc, @intCast(row.len));
            for (cell_opt of row) {
                if (cell := cell_opt) {
                    try writeI32(&list, alloc, @intCast(cell.len));
                    try list.appendSlice(alloc, cell);
                } else {
                    try writeI32(&list, alloc, -1);
                }
            }
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .command_complete => |tag| {
            try list.append(alloc, 'C');
            try writeU32(&list, alloc, 0);
            try writeCString(&list, alloc, tag);
            patchLengthAt(&list, 1, list.items.len - 1);
        },
        .empty_query_response => {
            try list.append(alloc, 'I');
            try writeU32(&list, alloc, 4);
        },
        .parse_complete => {
            try list.append(alloc, '1');
            try writeU32(&list, alloc, 4);
        },
        .bind_complete => {
            try list.append(alloc, '2');
            try writeU32(&list, alloc, 4);
        },
        .error_response => |err_msg| {
            try writeFieldError(&list, alloc, 'E', err_msg);
        },
        .notice_response => |notice_msg| {
            try writeFieldError(&list, alloc, 'N', notice_msg);
        },
        .unknown => |u| {
            try list.append(alloc, u.tag);
            try writeU32(&list, alloc, @intCast(4 + u.payload.len));
            try list.appendSlice(alloc, u.payload);
        },
    }

    return try list.toOwnedSlice(alloc);
}

struct Cursor {
    data: []const u8,
    pos: usize,

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn readU8(*_): !u8 {
        if (_.pos >= _.data.len) {
            return PgError.UnexpectedEof;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn readU16(*_): !u16 {
        if (_.remaining() < 2) {
            return PgError.UnexpectedEof;
        }
        const v = std.mem.readInt(u16, _.data[_.pos..][0..2], .big);
        _.pos += 2;
        return v;
    }

    fn readI16(*_): !i16 {
        if (_.remaining() < 2) {
            return PgError.UnexpectedEof;
        }
        const v = std.mem.readInt(i16, _.data[_.pos..][0..2], .big);
        _.pos += 2;
        return v;
    }

    fn readU32(*_): !u32 {
        if (_.remaining() < 4) {
            return PgError.UnexpectedEof;
        }
        const v = std.mem.readInt(u32, _.data[_.pos..][0..4], .big);
        _.pos += 4;
        return v;
    }

    fn readI32(*_): !i32 {
        if (_.remaining() < 4) {
            return PgError.UnexpectedEof;
        }
        const v = std.mem.readInt(i32, _.data[_.pos..][0..4], .big);
        _.pos += 4;
        return v;
    }

    fn readBytes(*_, len: usize): ![]const u8 {
        if (_.remaining() < len) {
            return PgError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    fn readCString(*_): ![]const u8 {
        const start = _.pos;
        while (_.pos < _.data.len) {
            if (_.data[_.pos] == 0) {
                const slice = _.data[start.._.pos];
                _.pos += 1;
                return slice;
            }
            _.pos += 1;
        }
        return PgError.UnexpectedEof;
    }
}

pub fn decodeFrontendPayload(alloc: std.mem.Allocator, tag_opt: u8?, payload: []const u8): !PgFrontendMsg {
    let cur = Cursor{ .data = payload, .pos = 0 };

    if (tag_opt == null) {
        const code = try cur.readU32();
        if (code == types.PG_SSL_REQUEST_CODE) {
            if (cur.remaining() != 0) {
                return PgError.InvalidPacket;
            }
            return PgFrontendMsg{ .ssl_request = {} };
        }
        if (code != types.PG_PROTOCOL_VERSION_3_0) {
            return PgError.InvalidPacket;
        }
        let kvs = std.ArrayList(PgKv).empty;
        errdefer {
            for (kv of kvs.items) {
                alloc.free(kv.key);
                alloc.free(kv.value);
            }
            kvs.deinit(alloc);
        }
        while (cur.remaining() > 0) {
            const b = cur.data[cur.pos];
            if (b == 0) {
                cur.pos += 1;
                break;
            }
            const k_slice = try cur.readCString();
            const v_slice = try cur.readCString();
            const k_dup = try alloc.dupe(u8, k_slice);
            errdefer alloc.free(k_dup);
            const v_dup = try alloc.dupe(u8, v_slice);
            errdefer alloc.free(v_dup);
            try kvs.append(alloc, PgKv{ .key = k_dup, .value = v_dup });
        }
        return PgFrontendMsg{ .startup = try kvs.toOwnedSlice(alloc) };
    }

    const tag = tag_opt.?;
    switch (tag) {
        'Q' => {
            const sql = try cur.readCString();
            return PgFrontendMsg{ .query = try alloc.dupe(u8, sql) };
        },
        'P' => {
            const stmt_s = try cur.readCString();
            const query_s = try cur.readCString();
            const count = try cur.readU16();
            let oids = try alloc.alloc(u32, count);
            errdefer alloc.free(oids);
            let i: usize = 0;
            while (i < count) {
                oids[i] = try cur.readU32();
                i += 1;
            }
            const stmt_dup = try alloc.dupe(u8, stmt_s);
            errdefer alloc.free(stmt_dup);
            const query_dup = try alloc.dupe(u8, query_s);
            return PgFrontendMsg{
                .parse = .{
                    .statement = stmt_dup,
                    .query = query_dup,
                    .param_types = oids,
                },
            };
        },
        'B' => {
            const portal_s = try cur.readCString();
            const stmt_s = try cur.readCString();

            const pfmt_count = try cur.readU16();
            let pfmts = try alloc.alloc(i16, pfmt_count);
            errdefer alloc.free(pfmts);
            let i: usize = 0;
            while (i < pfmt_count) {
                pfmts[i] = try cur.readI16();
                i += 1;
            }

            const param_count = try cur.readU16();
            let params = try alloc.alloc(OptBytes, param_count);
            let filled_params: usize = 0;
            errdefer {
                let j: usize = 0;
                while (j < filled_params) {
                    if (p := params[j]) {
                        alloc.free(p);
                    }
                    j += 1;
                }
                alloc.free(params);
            }
            i = 0;
            while (i < param_count) {
                const len_i32 = try cur.readI32();
                if (len_i32 == -1) {
                    params[i] = null;
                } else if (len_i32 < -1) {
                    return PgError.InvalidPacket;
                } else {
                    const bytes = try cur.readBytes(@intCast(len_i32));
                    params[i] = try alloc.dupe(u8, bytes);
                }
                filled_params += 1;
                i += 1;
            }

            const rfmt_count = try cur.readU16();
            let rfmts = try alloc.alloc(i16, rfmt_count);
            errdefer alloc.free(rfmts);
            i = 0;
            while (i < rfmt_count) {
                rfmts[i] = try cur.readI16();
                i += 1;
            }

            const portal_dup = try alloc.dupe(u8, portal_s);
            errdefer alloc.free(portal_dup);
            const stmt_dup = try alloc.dupe(u8, stmt_s);

            return PgFrontendMsg{
                .bind = .{
                    .portal = portal_dup,
                    .statement = stmt_dup,
                    .param_formats = pfmts,
                    .params = params,
                    .result_formats = rfmts,
                },
            };
        },
        'E' => {
            const portal_s = try cur.readCString();
            const max_rows = try cur.readI32();
            return PgFrontendMsg{
                .execute = .{
                    .portal = try alloc.dupe(u8, portal_s),
                    .max_rows = max_rows,
                },
            };
        },
        'S' => {
            if (cur.remaining() != 0) {
                return PgError.InvalidPacket;
            }
            return PgFrontendMsg{ .sync = {} };
        },
        'X' => {
            if (cur.remaining() != 0) {
                return PgError.InvalidPacket;
            }
            return PgFrontendMsg{ .terminate = {} };
        },
        'p' => {
            const pw = try cur.readCString();
            return PgFrontendMsg{ .password = try alloc.dupe(u8, pw) };
        },
        else => {
            return PgFrontendMsg{
                .unknown = .{
                    .tag = tag,
                    .payload = try alloc.dupe(u8, payload),
                },
            };
        },
    }
}

fn decodeFieldError(alloc: std.mem.Allocator, cur: *Cursor): !PgFieldError {
    let severity: []const u8 = "";
    let code: []const u8 = "";
    let message: []const u8 = "";
    let detail: ([]const u8)? = null;
    let hint: ([]const u8)? = null;

    errdefer {
        if (severity.len > 0) alloc.free(severity);
        if (code.len > 0) alloc.free(code);
        if (message.len > 0) alloc.free(message);
        if (d := detail) alloc.free(d);
        if (h := hint) alloc.free(h);
    }

    while (cur.remaining() > 0) {
        const field_type = try cur.readU8();
        if (field_type == 0) {
            break;
        }
        const val = try cur.readCString();
        switch (field_type) {
            'S' => {
                if (severity.len > 0) alloc.free(severity);
                severity = try alloc.dupe(u8, val);
            },
            'C' => {
                if (code.len > 0) alloc.free(code);
                code = try alloc.dupe(u8, val);
            },
            'M' => {
                if (message.len > 0) alloc.free(message);
                message = try alloc.dupe(u8, val);
            },
            'D' => {
                if (d := detail) alloc.free(d);
                detail = try alloc.dupe(u8, val);
            },
            'H' => {
                if (h := hint) alloc.free(h);
                hint = try alloc.dupe(u8, val);
            },
            else => {},
        }
    }

    return PgFieldError{
        .severity = severity,
        .code = code,
        .message = message,
        .detail = detail,
        .hint = hint,
    };
}

pub fn decodeBackendPayload(alloc: std.mem.Allocator, tag: u8, payload: []const u8): !PgBackendMsg {
    let cur = Cursor{ .data = payload, .pos = 0 };

    switch (tag) {
        'R' => {
            const auth_code = try cur.readU32();
            switch (auth_code) {
                0 => return PgBackendMsg{ .authentication = PgAuthKind{ .ok = {} } },
                2 => return PgBackendMsg{ .authentication = PgAuthKind{ .kerberos_v5 = {} } },
                3 => return PgBackendMsg{ .authentication = PgAuthKind{ .cleartext_password = {} } },
                5 => {
                    const salt_bytes = try cur.readBytes(4);
                    let salt: [4]u8 = undefined;
                    @memcpy(&salt, salt_bytes);
                    return PgBackendMsg{ .authentication = PgAuthKind{ .md5_password = salt } };
                },
                6 => return PgBackendMsg{ .authentication = PgAuthKind{ .scm_credential = {} } },
                7 => return PgBackendMsg{ .authentication = PgAuthKind{ .gss = {} } },
                9 => return PgBackendMsg{ .authentication = PgAuthKind{ .sspi = {} } },
                10 => {
                    let mechs = std.ArrayList(ByteSlice).empty;
                    errdefer {
                        for (m of mechs.items) {
                            alloc.free(m);
                        }
                        mechs.deinit(alloc);
                    }
                    while (cur.remaining() > 0) {
                        if (cur.data[cur.pos] == 0) {
                            cur.pos += 1;
                            break;
                        }
                        const m = try cur.readCString();
                        try mechs.append(alloc, try alloc.dupe(u8, m));
                    }
                    return PgBackendMsg{ .authentication = PgAuthKind{ .sasl = try mechs.toOwnedSlice(alloc) } };
                },
                11 => {
                    const rest = cur.data[cur.pos..];
                    return PgBackendMsg{ .authentication = PgAuthKind{ .sasl_continue = try alloc.dupe(u8, rest) } };
                },
                12 => {
                    const rest = cur.data[cur.pos..];
                    return PgBackendMsg{ .authentication = PgAuthKind{ .sasl_final = try alloc.dupe(u8, rest) } };
                },
                else => return PgBackendMsg{ .authentication = PgAuthKind{ .unknown = auth_code } },
            }
        },
        'S' => {
            const name_s = try cur.readCString();
            const val_s = try cur.readCString();
            const name_dup = try alloc.dupe(u8, name_s);
            errdefer alloc.free(name_dup);
            const val_dup = try alloc.dupe(u8, val_s);
            return PgBackendMsg{
                .parameter_status = .{
                    .name = name_dup,
                    .value = val_dup,
                },
            };
        },
        'K' => {
            const pid = try cur.readI32();
            const secret = try cur.readI32();
            return PgBackendMsg{
                .backend_key_data = .{
                    .process_id = pid,
                    .secret_key = secret,
                },
            };
        },
        'Z' => {
            const b = try cur.readU8();
            const status = try PgTxStatus.fromByte(b);
            return PgBackendMsg{ .ready_for_query = status };
        },
        'T' => {
            const count = try cur.readU16();
            let cols = try alloc.alloc(PgColumnDesc, count);
            let filled: usize = 0;
            errdefer {
                let j: usize = 0;
                while (j < filled) {
                    alloc.free(cols[j].name);
                    j += 1;
                }
                alloc.free(cols);
            }
            let i: usize = 0;
            while (i < count) {
                const name_s = try cur.readCString();
                const table_oid = try cur.readU32();
                const column_attr = try cur.readI16();
                const type_oid = try cur.readU32();
                const type_size = try cur.readI16();
                const type_modifier = try cur.readI32();
                const format_code = try cur.readI16();
                cols[i] = PgColumnDesc{
                    .name = try alloc.dupe(u8, name_s),
                    .table_oid = table_oid,
                    .column_attr = column_attr,
                    .type_oid = type_oid,
                    .type_size = type_size,
                    .type_modifier = type_modifier,
                    .format_code = format_code,
                };
                filled += 1;
                i += 1;
            }
            return PgBackendMsg{ .row_description = cols };
        },
        'D' => {
            const count = try cur.readU16();
            let cells = try alloc.alloc(OptBytes, count);
            let filled: usize = 0;
            errdefer {
                let j: usize = 0;
                while (j < filled) {
                    if (c := cells[j]) {
                        alloc.free(c);
                    }
                    j += 1;
                }
                alloc.free(cells);
            }
            let i: usize = 0;
            while (i < count) {
                const len_i32 = try cur.readI32();
                if (len_i32 == -1) {
                    cells[i] = null;
                } else if (len_i32 < -1) {
                    return PgError.InvalidPacket;
                } else {
                    const bytes = try cur.readBytes(@intCast(len_i32));
                    cells[i] = try alloc.dupe(u8, bytes);
                }
                filled += 1;
                i += 1;
            }
            return PgBackendMsg{ .data_row = cells };
        },
        'C' => {
            const cmd = try cur.readCString();
            return PgBackendMsg{ .command_complete = try alloc.dupe(u8, cmd) };
        },
        'I' => {
            return PgBackendMsg{ .empty_query_response = {} };
        },
        '1' => {
            return PgBackendMsg{ .parse_complete = {} };
        },
        '2' => {
            return PgBackendMsg{ .bind_complete = {} };
        },
        'E' => {
            return PgBackendMsg{ .error_response = try decodeFieldError(alloc, &cur) };
        },
        'N' => {
            return PgBackendMsg{ .notice_response = try decodeFieldError(alloc, &cur) };
        },
        else => {
            return PgBackendMsg{
                .unknown = .{
                    .tag = tag,
                    .payload = try alloc.dupe(u8, payload),
                },
            };
        },
    }
}
