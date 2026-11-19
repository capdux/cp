// Source: /data/dev/bun/src/http_types/URLPath.cp
// lib/http/url_path.cp
// CP HTTP URL Path Parser (cap:http)

const std = @import("std");

pub struct URLPath {
    pub extname: []const u8 = "";
    pub path: []const u8 = "";
    pub pathname: []const u8 = "";
    pub first_segment: []const u8 = "";
    pub query_string: []const u8 = "";
    pub needs_redirect: bool = false;
    pub is_source_map: bool = false;

    pub fn isRoot(): bool {
        return _.path.len == 0 || (_.path.len == 1 && _.path[0] == '/') || std.mem.eql(u8, _.path, "index");
    }

    pub static fn parse(raw_pathname: []const u8): URLPath {
        if (raw_pathname.len == 0) {
            return URLPath{};
        }

        let question_idx: i32 = -1;
        let period_idx: i32 = -1;
        let first_slash: i32 = -1;
        let last_slash: i32 = -1;

        const len = raw_pathname.len;
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const c = raw_pathname[i];
            if (c == '?' && question_idx < 0) {
                question_idx = i as i32;
            } else if (c == '/' && question_idx < 0) {
                if (first_slash < 0 && i > 0) {
                    first_slash = i as i32;
                }
                last_slash = i as i32;
            } else if (c == '.' && question_idx < 0) {
                period_idx = i as i32;
            }
        }

        const pathname_end: usize = if (question_idx >= 0) (question_idx as usize) else len;
        const path_slice = raw_pathname[0..pathname_end];
        const query_slice = if (question_idx >= 0) raw_pathname[(question_idx as usize)..len] else "";

        const ext_slice = if (period_idx > last_slash && period_idx >= 0)
            raw_pathname[((period_idx + 1) as usize)..pathname_end]
        else
            "";

        const first_seg_end: usize = if (first_slash > 0) (first_slash as usize) else pathname_end;
        const first_seg = if (first_seg_end > 1) raw_pathname[1..first_seg_end] else "";

        const is_map = std.mem.eql(u8, ext_slice, "map");

        return URLPath{
            extname: ext_slice,
            path: path_slice,
            pathname: raw_pathname,
            first_segment: first_seg,
            query_string: query_slice,
            needs_redirect: false,
            is_source_map: is_map,
        };
    }
}
