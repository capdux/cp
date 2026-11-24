// Example 03: Built-in Zero-Dependency IANA TimeZone (cap:tz) & LEB128 (cap:leb128)
// Run with: cap examples/03_tz_and_leb128.cp

import { loadZone } from "cap:tz";
import { encodeU64, decodeU64 } from "cap:leb128";
import std from "std";

fn main() {
    let tz = loadZone(std.heap.page_allocator, "Asia/Shanghai") catch return;
    const utc_ts: i64 = 1735689600; // 2025-01-01 00:00:00 UTC
    const offset = tz.offsetSeconds(utc_ts);
    const local_ts = tz.utcToLocal(utc_ts);

    let buf: [16]u8 = undefined;
    const n = encodeU64(buf[0..], 9876543210) catch return;
    const decoded = decodeU64(buf[0..n]) catch return;

    std.debug.print("IANA TZ Asia/Shanghai: offset={d}s local={d} | LEB128: encoded={d}B decoded={d}\n", .{
        offset,
        local_ts,
        n,
        decoded.value,
    });
}
