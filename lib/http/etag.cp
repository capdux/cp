// Source: Absorbed from /data/dev/bun/src/http_types/ETag.cp | Synced: 2026-11-19T20:58:51Z
// lib/http/etag.cp
// HTTP ETag and If-None-Match evaluation in CP

const std = @import("std");

pub struct ParsedETag {
    pub tag: []const u8 = "";
    pub is_weak: bool = false;
}

pub struct ETag {
    pub static fn parse(tag_str: []const u8): ParsedETag {
        let str = std.mem.trim(u8, tag_str, " \t");
        let is_weak = false;
        if (std.mem.startsWith(u8, str, "W/")) {
            is_weak = true;
            str = str[2..];
            str = std.mem.trimStart(u8, str, " \t");
        }
        if (str.len >= 2 && str[0] == '"' && str[str.len - 1] == '"') {
            str = str[1 .. str.len - 1];
        }
        return ParsedETag{
            tag: str,
            is_weak: is_weak,
        };
    }

    pub static fn ifNoneMatch(etag: []const u8, if_none_match: []const u8): bool {
        const our_parsed = ETag.parse(etag);
        if (std.mem.eql(u8, std.mem.trim(u8, if_none_match, " \t"), "*")) {
            return true;
        }

        let iter = std.mem.splitScalar(u8, if_none_match, ',');
        while (tag_str := iter.next()) {
            const parsed = ETag.parse(tag_str);
            if (std.mem.eql(u8, our_parsed.tag, parsed.tag)) {
                return true;
            }
        }
        return false;
    }

    pub static fn generate(bytes: []const u8, out_buf: *[32]u8): []const u8 {
        const hash = std.hash.Wyhash.hash(0, bytes);
        return std.fmt.bufPrint(out_buf, "\"{x}\"", .{hash}) catch "\"0\"";
    }
}
