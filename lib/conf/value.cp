// Source: Absorbed from /data/dev/bun/src/interchange/interchange.cp | Synced: 2026-11-20T09:48:03Z
// lib/conf/value.cp
// Native CP Configuration AST (ConfValue, ConfField, ConfDateTime) with RFC 3339 Date/Time Validation

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "../json/value";

pub enum ConfTag: u8 {
    Null = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    String = 4,
    DateTime = 5,
    Array = 6,
    Object = 7,
}

pub enum ConfDateTimeKind: u8 {
    OffsetDateTime = 0,
    LocalDateTime = 1,
    LocalDate = 2,
    LocalTime = 3,
}

pub struct ConfDateTime {
    pub kind: ConfDateTimeKind = ConfDateTimeKind.OffsetDateTime;
    pub year: u16 = 0;
    pub month: u8 = 0;
    pub day: u8 = 0;
    pub hour: u8 = 0;
    pub minute: u8 = 0;
    pub second: u8 = 0;
    pub nanosecond: u32 = 0;
    pub offset_minutes: i16 = 0;
    pub has_offset: bool = false;
    pub is_utc: bool = false;
    pub raw: []const u8 = "";

    static fn isLeapYear(y: u16): bool {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    static fn daysInMonth(y: u16, m: u8): u8 {
        return switch (m) {
            1, 3, 5, 7, 8, 10, 12 => 31,
            4, 6, 9, 11 => 30,
            2 => if (ConfDateTime.isLeapYear(y)) (29 as u8) else (28 as u8),
            else => 0,
        };
    }

    static fn parseDigits(s: []const u8): u32? {
        if (s.len == 0) return null;
        let acc: u32 = 0;
        for (c of s) {
            if (c < '0' || c > '9') return null;
            acc = acc * 10 + ((c - '0') as u32);
        }
        return acc;
    }

    static fn parseTimePart(
        s: []const u8,
        out_h: *u8,
        out_m: *u8,
        out_s: *u8,
        out_ns: *u32,
        out_consumed: *usize,
    ): bool {
        if (s.len < 8) return false;
        if (s[2] != ':' || s[5] != ':') return false;
        const hh = ConfDateTime.parseDigits(s[0..2]) orelse return false;
        const mm = ConfDateTime.parseDigits(s[3..5]) orelse return false;
        const ss = ConfDateTime.parseDigits(s[6..8]) orelse return false;
        if (hh > 23 || mm > 59 || ss > 60) return false;

        out_h.* = hh as u8;
        out_m.* = mm as u8;
        out_s.* = ss as u8;
        out_ns.* = 0;
        let pos: usize = 8;

        if (pos < s.len && s[pos] == '.') {
            pos += 1;
            const frac_start = pos;
            let ns: u32 = 0;
            let mult: u32 = 100000000;
            while (pos < s.len && s[pos] >= '0' && s[pos] <= '9') {
                if (mult > 0) {
                    ns += ((s[pos] - '0') as u32) * mult;
                    mult /= 10;
                }
                pos += 1;
            }
            if (pos == frac_start) return false;
            out_ns.* = ns;
        }
        out_consumed.* = pos;
        return true;
    }

    /// Parses and validates an RFC 3339 / TOML 1.0 Date/Time literal:
    /// - Offset Date-Time: `1979-05-27T07:32:00Z`, `1979-05-27 07:32:00-07:00`
    /// - Local Date-Time:  `1979-05-27T07:32:00.999999`
    /// - Local Date:       `1979-05-27`
    /// - Local Time:       `07:32:00`, `07:32:00.5`
    pub static fn parse(raw: []const u8): ConfDateTime? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len < 8) return null;

        // Case 1: Local Time (`HH:MM:SS[.frac]`)
        if (s[2] == ':') {
            let dt = ConfDateTime{
                kind: ConfDateTimeKind.LocalTime,
                raw: s,
            };
            let used: usize = 0;
            if (!ConfDateTime.parseTimePart(s, &dt.hour, &dt.minute, &dt.second, &dt.nanosecond, &used)) {
                return null;
            }
            if (used != s.len) return null;
            return dt;
        }

        // Must start with `YYYY-MM-DD`
        if (s.len < 10 || s[4] != '-' || s[7] != '-') return null;
        const yr = ConfDateTime.parseDigits(s[0..4]) orelse return null;
        const mo = ConfDateTime.parseDigits(s[5..7]) orelse return null;
        const dy = ConfDateTime.parseDigits(s[8..10]) orelse return null;
        if (mo < 1 || mo > 12) return null;
        const max_day = ConfDateTime.daysInMonth(yr as u16, mo as u8);
        if (dy < 1 || dy > (max_day as u32)) return null;

        let dt = ConfDateTime{
            kind: ConfDateTimeKind.LocalDate,
            year: yr as u16,
            month: mo as u8,
            day: dy as u8,
            raw: s,
        };

        if (s.len == 10) {
            return dt;
        }

        // Date-Time separator: 'T', 't', or ' '
        const sep = s[10];
        if (sep != 'T' && sep != 't' && sep != ' ') return null;
        if (s.len < 19) return null;

        let used: usize = 0;
        if (!ConfDateTime.parseTimePart(s[11..], &dt.hour, &dt.minute, &dt.second, &dt.nanosecond, &used)) {
            return null;
        }
        const off_pos = 11 + used;
        if (off_pos == s.len) {
            dt.kind = ConfDateTimeKind.LocalDateTime;
            return dt;
        }

        const tz = s[off_pos..];
        if (tz.len == 1 && (tz[0] == 'Z' || tz[0] == 'z')) {
            dt.kind = ConfDateTimeKind.OffsetDateTime;
            dt.has_offset = true;
            dt.is_utc = true;
            dt.offset_minutes = 0;
            return dt;
        }

        if (tz.len == 6 && (tz[0] == '+' || tz[0] == '-') && tz[3] == ':') {
            const oh = ConfDateTime.parseDigits(tz[1..3]) orelse return null;
            const om = ConfDateTime.parseDigits(tz[4..6]) orelse return null;
            if (oh > 23 || om > 59) return null;
            const total_mins: i16 = ((oh * 60 + om) as i16);
            dt.kind = ConfDateTimeKind.OffsetDateTime;
            dt.has_offset = true;
            dt.is_utc = (total_mins == 0);
            dt.offset_minutes = if (tz[0] == '-') -total_mins else total_mins;
            return dt;
        }

        return null;
    }

    pub fn format(buf: []u8): ([]const u8)! {
        if (_.raw.len > 0) {
            if (_.raw.len > buf.len) return error.BufferTooSmall;
            @memcpy(buf[0.._.raw.len], _.raw);
            return buf[0.._.raw.len];
        }
        return switch (_.kind) {
            .LocalDate => std.fmt.bufPrint(buf, "{d:0>4}-{d:0>2}-{d:0>2}", .{ _.year, _.month, _.day }) catch return error.BufferTooSmall,
            .LocalTime => std.fmt.bufPrint(buf, "{d:0>2}:{d:0>2}:{d:0>2}", .{ _.hour, _.minute, _.second }) catch return error.BufferTooSmall,
            .LocalDateTime => std.fmt.bufPrint(buf, "{d:0>4}-{d:0>2}-{d:0>2}T{d:0>2}:{d:0>2}:{d:0>2}", .{ _.year, _.month, _.day, _.hour, _.minute, _.second }) catch return error.BufferTooSmall,
            .OffsetDateTime => {
                if (_.is_utc) {
                    return std.fmt.bufPrint(buf, "{d:0>4}-{d:0>2}-{d:0>2}T{d:0>2}:{d:0>2}:{d:0>2}Z", .{ _.year, _.month, _.day, _.hour, _.minute, _.second }) catch return error.BufferTooSmall;
                }
                const sign: u8 = if (_.offset_minutes < 0) '-' else '+';
                const abs_m: u16 = if (_.offset_minutes < 0) ((-_.offset_minutes) as u16) else (_.offset_minutes as u16);
                const oh = abs_m / 60;
                const om = abs_m % 60;
                return std.fmt.bufPrint(buf, "{d:0>4}-{d:0>2}-{d:0>2}T{d:0>2}:{d:0>2}:{d:0>2}{c}{d:0>2}:{d:0>2}", .{ _.year, _.month, _.day, _.hour, _.minute, _.second, sign, oh, om }) catch return error.BufferTooSmall;
            },
        };
    }
}

pub struct ConfField {
    pub key: []const u8 = "";
    pub value: ConfValue = ConfValue{},
}

pub struct ConfValue {
    pub tag: ConfTag = ConfTag.Null;
    pub bool_val: bool = false;
    pub int_val: i64 = 0;
    pub float_val: f64 = 0.0;
    pub str_val: []const u8 = "";
    pub dt_val: ConfDateTime = ConfDateTime{},
    pub arr_val: []ConfValue = [],
    pub obj_val: []ConfField = [],

    pub static fn makeNull(): ConfValue {
        return ConfValue{ tag: ConfTag.Null };
    }

    pub static fn makeBool(v: bool): ConfValue {
        return ConfValue{ tag: ConfTag.Bool, bool_val: v };
    }

    pub static fn makeInt(v: i64): ConfValue {
        return ConfValue{ tag: ConfTag.Int, int_val: v, float_val: v as f64 };
    }

    pub static fn makeFloat(v: f64): ConfValue {
        return ConfValue{ tag: ConfTag.Float, float_val: v };
    }

    pub static fn makeString(s: []const u8): ConfValue {
        return ConfValue{ tag: ConfTag.String, str_val: s };
    }

    pub static fn makeDateTime(dt: ConfDateTime): ConfValue {
        return ConfValue{ tag: ConfTag.DateTime, dt_val: dt, str_val: dt.raw };
    }

    pub static fn makeArray(items: []ConfValue): ConfValue {
        return ConfValue{ tag: ConfTag.Array, arr_val: items };
    }

    pub static fn makeEmptyObject(): ConfValue {
        return ConfValue{ tag: ConfTag.Object };
    }

    pub static fn makeObject(fields: []ConfField): ConfValue {
        return ConfValue{ tag: ConfTag.Object, obj_val: fields };
    }

    pub fn isNull(): bool { return _.tag == ConfTag.Null; }
    pub fn isBool(): bool { return _.tag == ConfTag.Bool; }
    pub fn isInt(): bool { return _.tag == ConfTag.Int; }
    pub fn isFloat(): bool { return _.tag == ConfTag.Float; }
    pub fn isNumber(): bool { return _.tag == ConfTag.Int || _.tag == ConfTag.Float; }
    pub fn isString(): bool { return _.tag == ConfTag.String; }
    pub fn isDateTime(): bool { return _.tag == ConfTag.DateTime; }
    pub fn isArray(): bool { return _.tag == ConfTag.Array; }
    pub fn isObject(): bool { return _.tag == ConfTag.Object; }

    pub fn asBool(): bool? {
        if (_.tag == ConfTag.Bool) return _.bool_val;
        return null;
    }

    pub fn asInt(): i64? {
        if (_.tag == ConfTag.Int) return _.int_val;
        return null;
    }

    pub fn asFloat(): f64? {
        if (_.tag == ConfTag.Float) return _.float_val;
        if (_.tag == ConfTag.Int) return _.int_val as f64;
        return null;
    }

    pub fn asNumber(): f64? {
        return _.asFloat();
    }

    pub fn asString(): ([]const u8)? {
        if (_.tag == ConfTag.String) return _.str_val;
        if (_.tag == ConfTag.DateTime) return _.dt_val.raw;
        return null;
    }

    pub fn asDateTime(): ConfDateTime? {
        if (_.tag == ConfTag.DateTime) return _.dt_val;
        return null;
    }

    pub fn asArray(): ([]ConfValue)? {
        if (_.tag == ConfTag.Array) return _.arr_val;
        return null;
    }

    pub fn asObject(): ([]ConfField)? {
        if (_.tag == ConfTag.Object) return _.obj_val;
        return null;
    }

    pub fn len(): usize {
        return switch (_.tag) {
            .Array => _.arr_val.len,
            .Object => _.obj_val.len,
            .String => _.str_val.len,
            else => 0,
        };
    }

    pub fn get(key: []const u8): ConfValue? {
        if (_.tag != ConfTag.Object) return null;
        let i: usize = _.obj_val.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.obj_val[i].key, key)) {
                return _.obj_val[i].value;
            }
        }
        return null;
    }

    pub fn getIndex(idx: usize): ConfValue? {
        if (_.tag != ConfTag.Array || idx >= _.arr_val.len) return null;
        return _.arr_val[idx];
    }

    /// Recursive dot/index path query (e.g. `"server.tls.port"` or `"products.0.name"`).
    pub fn getPath(path: []const u8): ConfValue? {
        if (path.len == 0) return _;
        let cur = _;
        let iter = std.mem.splitScalar(u8, path, '.');
        while (seg := iter.next()) {
            if (seg.len == 0) continue;
            if (cur.tag == ConfTag.Object) {
                if (next_v := cur.get(seg)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else if (cur.tag == ConfTag.Array) {
                const idx = std.fmt.parseInt(usize, seg, 10) catch return null;
                if (next_v := cur.getIndex(idx)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        }
        return cur;
    }

    pub fn clone(allocator: std.mem.Allocator): ConfValue! {
        return switch (_.tag) {
            .Null, .Bool, .Int, .Float => _,
            .String => ConfValue.makeString(try allocator.dupe(u8, _.str_val)),
            .DateTime => {
                let dt = _.dt_val;
                dt.raw = try allocator.dupe(u8, _.dt_val.raw);
                return ConfValue.makeDateTime(dt);
            },
            .Array => {
                let items = try allocator.alloc(ConfValue, _.arr_val.len);
                let i: usize = 0;
                while (i < _.arr_val.len) : (i += 1) {
                    items[i] = try _.arr_val[i].clone(allocator);
                }
                return ConfValue.makeArray(items);
            },
            .Object => {
                let fields = try allocator.alloc(ConfField, _.obj_val.len);
                let i: usize = 0;
                while (i < _.obj_val.len) : (i += 1) {
                    fields[i] = ConfField{
                        key: try allocator.dupe(u8, _.obj_val[i].key),
                        value: try _.obj_val[i].value.clone(allocator),
                    };
                }
                return ConfValue.makeObject(fields);
            },
        };
    }

    pub fn toJsonValue(allocator: std.mem.Allocator): JsonValue! {
        return switch (_.tag) {
            .Null => JsonValue{ tag: JsonTag.Null },
            .Bool => JsonValue{ tag: JsonTag.Bool, bool_val: _.bool_val },
            .Int => JsonValue{ tag: JsonTag.Number, num_val: _.int_val as f64 },
            .Float => JsonValue{ tag: JsonTag.Number, num_val: _.float_val },
            .String => JsonValue{ tag: JsonTag.String, str_val: _.str_val },
            .DateTime => JsonValue{ tag: JsonTag.String, str_val: _.dt_val.raw },
            .Array => {
                let items = try allocator.alloc(JsonValue, _.arr_val.len);
                let i: usize = 0;
                while (i < _.arr_val.len) : (i += 1) {
                    items[i] = try _.arr_val[i].toJsonValue(allocator);
                }
                return JsonValue{ tag: JsonTag.Array, arr_val: items };
            },
            .Object => {
                let fields = try allocator.alloc(JsonField, _.obj_val.len);
                let i: usize = 0;
                while (i < _.obj_val.len) : (i += 1) {
                    fields[i] = JsonField{
                        key: _.obj_val[i].key,
                        value: try _.obj_val[i].value.toJsonValue(allocator),
                    };
                }
                return JsonValue{ tag: JsonTag.Object, obj_val: fields };
            },
        };
    }
}
