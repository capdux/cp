// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T14:39:13Z
/**
 * CP HTTP Request (cap:http)
 */
const std = @import("std");

pub class Request {
    pub method: string;
    pub url: string;
    pub body: string;

    pub fn init(*_, method: string, url: string, body: string) {
        _.method = method;
        _.url = url;
        _.body = body;
    }

    pub fn text(): string {
        return _.body;
    }

    pub fn json(): std.json.Value! {
        return std.json.parseFromSliceLeaky(std.json.Value, std.heap.page_allocator, _.body, .{});
    }
}
