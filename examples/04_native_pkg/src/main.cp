// Example 04: Using Official Native C Ecosystem Packages (@cap/lz4 + @cap/yyjson)
// Run with: cd examples/04_native_pkg && cap install && cap run src/main.cp

import { compressInto, decompressInto } from "@cap/lz4";
import { JsonDoc } from "@cap/yyjson";
import std from "std";

fn main() {
    const raw = "{\"lang\":\"CP\",\"version\":\"1.0.0\",\"fast\":true,\"packages\":43}";
    let comp_buf: [256]u8 = undefined;
    let out_buf: [256]u8 = undefined;

    const comp = compressInto(raw, comp_buf[0..]) catch return;
    const restored = decompressInto(comp, out_buf[0..]) catch return;

    const doc = JsonDoc.parse(restored) catch return;
    const root = doc.root();
    const lang: string = root.get("lang").?.asStr().?;
    const pkgs: i64 = root.get("packages").?.asInt().?;
    std.debug.print("LZ4 ({d}B -> {d}B) + yyjson OK: lang={s}, packages={d}\n", .{ raw.len, comp.len, lang, pkgs });
}
