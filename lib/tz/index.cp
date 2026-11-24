// ============================================================================
// cap:tz — IANA TZif Time Zone Parser & Lookup (wraps Zig 0.17 `std.Tz`)
// ============================================================================

const std = @import("std");

pub err TzError {
    InvalidZoneName,
    EmptyZoneData,
    ZoneFileNotFound,
    ZoneReadFailed,
    ZoneFileTooLarge,
}

fn readZoneFile(allocator: std.mem.Allocator, path: []const u8): ([]u8)! {
    let path_buf: [512]u8 = undefined;
    if (path.len == 0 || path.len >= path_buf.len) return error.InvalidZoneName;
    @memcpy(path_buf[0 .. path.len], path);
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0 .. path.len :0].ptr;

    const open_rc = std.os.linux.openat(std.os.linux.AT.FDCWD, path_z, .{ .ACCMODE = .RDONLY, .CLOEXEC = true }, 0);
    if (@as(isize, @bitCast(open_rc)) < 0) return error.ZoneFileNotFound;
    const fd: i32 = @as(i32, @intCast(open_rc));
    defer {
        * = std.os.linux.close(fd);
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);
    let chunk: [4096]u8 = undefined;
    while (true) {
        const n_rc = std.os.linux.read(fd, &chunk, chunk.len);
        const n_signed = @as(isize, @bitCast(n_rc));
        if (n_signed < 0) return error.ZoneReadFailed;
        if (n_signed == 0) break;
        const n: usize = @as(usize, @intCast(n_signed));
        try out.appendSlice(allocator, chunk[0 .. n]);
        if (out.items.len > 1024 * 1024) return error.ZoneFileTooLarge;
    }
    return out.toOwnedSlice(allocator);
}

/// Resolved UTC offset, DST flag, and short time zone abbreviation for a timestamp.
pub struct OffsetInfo {
    offset_seconds: i32,
    is_dst: bool,
    abbr_buf: [6]u8,
    abbr_len: u8,

    pub fn abbr(): []const u8 {
        return _.abbr_buf[0 .. @as(usize, _.abbr_len)];
    }

    pub fn offsetMinutes(): i32 {
        return @divTrunc(_.offset_seconds, 60);
    }
}

fn makeOffsetInfo(offset: i32, is_dst: bool, nm: []const u8): OffsetInfo {
    let buf: [6]u8 = [0, 0, 0, 0, 0, 0];
    const n: usize = if (nm.len > 6) 6 else nm.len;
    if (n > 0) {
        @memcpy(buf[0 .. n], nm[0 .. n]);
    }
    return OffsetInfo{
        offset_seconds: offset,
        is_dst: is_dst,
        abbr_buf: buf,
        abbr_len: @as(u8, @intCast(n)),
    };
}

/// Owned IANA TZif time zone handle with idempotent `~init()` RAII cleanup.
pub struct TimeZone {
    inner: std.Tz,
    active: bool,

    /// Parses raw RFC 8536 TZif binary bytes into a `TimeZone`.
    pub static fn parse(allocator: std.mem.Allocator, data: []const u8): TimeZone! {
        if (data.len == 0) return error.EmptyZoneData;
        let reader: std.Io.Reader = std.Io.Reader.fixed(data);
        const tz = try std.Tz.parse(allocator, &reader);
        return TimeZone{
            inner: tz,
            active: true,
        };
    }

    /// Loads and parses a TZif file from an explicit filesystem path.
    pub static fn loadFile(allocator: std.mem.Allocator, path: []const u8): TimeZone! {
        const raw = try readZoneFile(allocator, path);
        defer allocator.free(raw);
        return TimeZone.parse(allocator, raw);
    }

    /// Loads an IANA time zone from `/usr/share/zoneinfo/<zone_name>`.
    pub static fn loadZone(allocator: std.mem.Allocator, zone_name: []const u8): TimeZone! {
        if (zone_name.len == 0 || zone_name.len > 255) return error.InvalidZoneName;
        if (zone_name[0] == '/') return error.InvalidZoneName;
        let i: usize = 0;
        while (i < zone_name.len) : (i += 1) {
            const c = zone_name[i];
            if (c == 0) return error.InvalidZoneName;
            if (c == '.' && i + 1 < zone_name.len && zone_name[i + 1] == '.') {
                return error.InvalidZoneName;
            }
        }
        const prefix: []const u8 = "/usr/share/zoneinfo/";
        let path_buf: [512]u8 = undefined;
        const total_len = prefix.len + zone_name.len;
        if (total_len >= path_buf.len) return error.InvalidZoneName;
        @memcpy(path_buf[0 .. prefix.len], prefix);
        @memcpy(path_buf[prefix.len .. total_len], zone_name);
        return TimeZone.loadFile(allocator, path_buf[0 .. total_len]);
    }

    ~init() {
        if (_.active) {
            _.inner.deinit();
            _.active = false;
        }
    }

    pub fn deinit(*_) {
        _.~init();
    }

    pub fn transitionCount(): usize {
        if (!_.active) return 0;
        return _.inner.transitions.len;
    }

    pub fn timetypeCount(): usize {
        if (!_.active) return 0;
        return _.inner.timetypes.len;
    }

    pub fn footer(): ([]const u8)? {
        if (!_.active) return null;
        return _.inner.footer;
    }

    /// Looks up the active `OffsetInfo` for a given UTC Unix timestamp (seconds).
    pub fn lookup(utc_timestamp: i64): OffsetInfo {
        if (!_.active || _.inner.timetypes.len == 0) {
            return OffsetInfo{
                offset_seconds: 0,
                is_dst: false,
                abbr_buf: ['U', 'T', 'C', 0, 0, 0],
                abbr_len: 3,
            };
        }
        const transitions = _.inner.transitions;
        if (transitions.len == 0) {
            const tt = _.inner.timetypes[0];
            return makeOffsetInfo(tt.offset, tt.isDst(), tt.name());
        }
        if (utc_timestamp < transitions[0].ts) {
            let k: usize = 0;
            while (k < _.inner.timetypes.len) : (k += 1) {
                const tt = _.inner.timetypes[k];
                if (!tt.isDst()) {
                    return makeOffsetInfo(tt.offset, tt.isDst(), tt.name());
                }
            }
            const tt0 = _.inner.timetypes[0];
            return makeOffsetInfo(tt0.offset, tt0.isDst(), tt0.name());
        }
        let lo: usize = 0;
        let hi: usize = transitions.len;
        while (lo + 1 < hi) {
            const mid = lo + (hi - lo) / 2;
            if (transitions[mid].ts <= utc_timestamp) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        const matched = transitions[lo].timetype.*;
        return makeOffsetInfo(matched.offset, matched.isDst(), matched.name());
    }

    /// Returns the UTC offset in seconds at `utc_timestamp`.
    pub fn offsetSeconds(utc_timestamp: i64): i32 {
        return _.lookup(utc_timestamp).offset_seconds;
    }

    /// Returns true if daylight saving time is active at `utc_timestamp`.
    pub fn isDst(utc_timestamp: i64): bool {
        return _.lookup(utc_timestamp).is_dst;
    }

    /// Converts a UTC Unix timestamp (seconds) to local wall-clock seconds.
    pub fn utcToLocal(utc_timestamp: i64): i64 {
        return utc_timestamp + @as(i64, _.lookup(utc_timestamp).offset_seconds);
    }
}

pub fn parseTz(allocator: std.mem.Allocator, data: []const u8): TimeZone! {
    return TimeZone.parse(allocator, data);
}

pub fn loadZoneFile(allocator: std.mem.Allocator, path: []const u8): TimeZone! {
    return TimeZone.loadFile(allocator, path);
}

pub fn loadZone(allocator: std.mem.Allocator, zone_name: []const u8): TimeZone! {
    return TimeZone.loadZone(allocator, zone_name);
}
