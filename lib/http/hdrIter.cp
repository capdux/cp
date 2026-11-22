// Source: Absorbed from /data/dev/bun/src/http/HeaderValueIterator.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/header_iterator.cp
// CP HTTP Header Value Iterator (cap:http)

const std = @import("std");

pub struct HeaderValueIterator {
    pub raw: []const u8 = "";
    pub pos: usize = 0;

    pub static fn init(input: []const u8): HeaderValueIterator {
        return HeaderValueIterator{
            raw: std.mem.trim(u8, input, " \t"),
            pos: 0,
        };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.raw.len) {
            const start = _.pos;
            let end = start;
            while (end < _.raw.len && _.raw[end] != ',') {
                end += 1;
            }

            const slice = std.mem.trim(u8, _.raw[start..end], " \t");
            _.pos = if (end < _.raw.len) end + 1 else _.raw.len;

            if (slice.len > 0) {
                return slice;
            }
        }
        return null;
    }
}
