// Bench 4 (CP): Native C package @cap/yyjson — 10,000 parses of medium JSON payload
import { JsonDoc } from "@cap/yyjson";
import { HwTimer } from "cap:bench";
import std from "std";

const ITERATIONS: usize = 10_000;
const PAYLOAD: []const u8 = @embedFile("payload.json");

fn parseOnce(payload: []const u8): i64 {
    const doc = JsonDoc.parse(payload) catch return 0;
    const root = doc.root();
    const meta = root.get("meta").?;
    const req_id = meta.get("request_id").?.asStr().?;
    const items = root.get("items").?;
    const n = items.arrLen();

    let sum: i64 = @as(i64, @intCast(req_id.len));
    let j: usize = 0;
    while (j < n) {
        const item = items.arrGet(j).?;
        sum += item.get("score").?.asInt().?;
        j += 1;
    }
    return sum;
}

fn main() {
    let checksum: i64 = 0;

    const t0 = HwTimer.nowNs();
    let i: usize = 0;
    while (i < ITERATIONS) {
        checksum += parseOnce(PAYLOAD);
        i += 1;
    }
    std.mem.doNotOptimizeAway(&checksum);
    const t1 = HwTimer.nowNs();

    const parse_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    std.debug.print("{{\"parse_ms\":{d:.3},\"iterations\":{d},\"payload_bytes\":{d},\"checksum\":{d}}}\n", .{
        parse_ms,
        ITERATIONS,
        PAYLOAD.len,
        checksum,
    });
}
