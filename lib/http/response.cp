// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T14:39:13Z
/**
 * CP HTTP Response (cap:http)
 */
const std = @import("std");

pub class Response {
    pub status: i32;
    pub body: string;
    pub contentType: string;

    pub fn init(*_, body: string) {
        _.status = 200;
        _.body = body;
        _.contentType = "text/plain";
    }

    pub fn text(): string {
        return _.body;
    }

    pub fn json(): std.json.Value! {
        return std.json.parseFromSliceLeaky(std.json.Value, std.heap.page_allocator, _.body, .{});
    }

    pub fn ok(): bool {
        if (_.status >= 200) {
            if (_.status < 300) {
                return true;
            }
        }
        return false;
    }
}
