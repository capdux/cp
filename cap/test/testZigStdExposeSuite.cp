// Verification suite for Zig 0.17 std-backed CP stdlib modules:
// cap:tz, cap:leb128, cap:progress, cap:zon (4-Gate 0/0/0/0)

import std from "std";
import { test, expect_eq } from "cap:test";
import { TimeZone, loadZone, loadZoneFile } from "cap:tz";
import {
    encodeU64,
    decodeU64,
    encodeI64,
    decodeI64,
    encodeUsize,
    decodeUsize,
    encodedLenU64,
    encodedLenI64,
    writeUnsignedExtended,
} from "cap:leb128";
import { ProgressBar, ProgressNode, ProgressOptions } from "cap:progress";
import { ZonDocument, ZonTag, parseZon, validateZon, formatZon, formatZonCompact } from "cap:zon";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn runCase1Tz(): void! {
    const alloc = std.heap.page_allocator;

    // 1. UTC
    let utc = try loadZone(alloc, "UTC");
    defer utc.deinit();
    const utc_info = utc.lookup(1705320000); // 2024-01-15T12:00:00Z
    expect_eq(utc_info.offset_seconds, 0);
    expect_eq(utc_info.is_dst, false);
    expect_eq(bytesEq(utc_info.abbr(), "UTC"), true);
    expect_eq(utc.utcToLocal(1705320000), 1705320000);

    // 2. America/New_York winter (EST, -05:00) vs summer (EDT, -04:00)
    let ny = try loadZone(alloc, "America/New_York");
    defer ny.deinit();
    expect_eq(ny.transitionCount() > 0, true);

    const winter_ts: i64 = 1705320000; // 2024-01-15T12:00:00Z
    const winter_info = ny.lookup(winter_ts);
    expect_eq(winter_info.offset_seconds, -18000);
    expect_eq(winter_info.offsetMinutes(), -300);
    expect_eq(winter_info.is_dst, false);
    expect_eq(bytesEq(winter_info.abbr(), "EST"), true);
    expect_eq(ny.utcToLocal(winter_ts), winter_ts - 18000);

    const summer_ts: i64 = 1721044800; // 2024-07-15T12:00:00Z
    const summer_info = ny.lookup(summer_ts);
    expect_eq(summer_info.offset_seconds, -14400);
    expect_eq(summer_info.offsetMinutes(), -240);
    expect_eq(summer_info.is_dst, true);
    expect_eq(bytesEq(summer_info.abbr(), "EDT"), true);
    expect_eq(ny.utcToLocal(summer_ts), summer_ts - 14400);

    // 3. Asia/Shanghai (+08:00, CST, non-DST)
    let sh = try loadZoneFile(alloc, "/usr/share/zoneinfo/Asia/Shanghai");
    defer sh.deinit();
    const sh_info = sh.lookup(winter_ts);
    expect_eq(sh_info.offset_seconds, 28800);
    expect_eq(sh_info.is_dst, false);
    expect_eq(bytesEq(sh_info.abbr(), "CST"), true);

    // 4. Reject path traversal & empty zone name
    let bad_traversal = false;
    * = loadZone(alloc, "../etc/passwd") catch {
        bad_traversal = true;
    };
    expect_eq(bad_traversal, true);

    let bad_empty = false;
    * = loadZone(alloc, "") catch {
        bad_empty = true;
    };
    expect_eq(bad_empty, true);

    // 5. Idempotent double deinit
    utc.deinit();
}

fn runCase2Leb128(): void! {
    let buf: [16]u8 = undefined;

    // Unsigned ULEB128 values
    const u_cases: [8]u64 = [
        0,
        1,
        127,
        128,
        16383,
        16384,
        624485,
        std.math.maxInt(u64),
    ];
    let i: usize = 0;
    while (i < u_cases.len) : (i += 1) {
        const v = u_cases[i];
        const written = try encodeU64(&buf, v);
        expect_eq(written, encodedLenU64(v));
        const decoded = try decodeU64(buf[0 .. written]);
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, written);
    }

    // Standard DWARF test vector: 624485 -> 0xE5 0x8E 0x26
    const n_dwarf = try encodeU64(&buf, 624485);
    expect_eq(n_dwarf, 3);
    expect_eq(buf[0], 0xE5);
    expect_eq(buf[1], 0x8E);
    expect_eq(buf[2], 0x26);

    // Signed SLEB128 values
    const i_cases: [13]i64 = [
        0,
        1,
        63,
        64,
        127,
        128,
        -1,
        -64,
        -65,
        -128,
        -123456,
        std.math.minInt(i64),
        std.math.maxInt(i64),
    ];
    i = 0;
    while (i < i_cases.len) : (i += 1) {
        const v = i_cases[i];
        const written = try encodeI64(&buf, v);
        expect_eq(written, encodedLenI64(v));
        const decoded = try decodeI64(buf[0 .. written]);
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, written);
    }

    // usize round-trip + fixed-width padded unsigned
    const n_usize = try encodeUsize(&buf, 1234567);
    const dec_usize = try decodeUsize(buf[0 .. n_usize]);
    expect_eq(dec_usize.value, 1234567);

    let fixed_buf: [4]u8 = undefined;
    try writeUnsignedExtended(&fixed_buf, 1000);
    const dec_fixed = try decodeU64(&fixed_buf);
    expect_eq(dec_fixed.value, 1000);
    expect_eq(dec_fixed.bytes_read, 4);

    // Truncated input -> UnexpectedEof
    let eof_empty = false;
    * = decodeU64(&.{}) catch {
        eof_empty = true;
    };
    expect_eq(eof_empty, true);

    const trunc_bytes: [2]u8 = [0x80, 0x80];
    let eof_u = false;
    * = decodeU64(&trunc_bytes) catch {
        eof_u = true;
    };
    expect_eq(eof_u, true);

    let eof_i = false;
    * = decodeI64(&trunc_bytes) catch {
        eof_i = true;
    };
    expect_eq(eof_i, true);

    // Overflow input -> Overflow
    const overflow_u: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
    let ovf_u = false;
    * = decodeU64(&overflow_u) catch {
        ovf_u = true;
    };
    expect_eq(ovf_u, true);

    const overflow_i: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01];
    let ovf_i = false;
    * = decodeI64(&overflow_i) catch {
        ovf_i = true;
    };
    expect_eq(ovf_i, true);

    // BufferTooSmall guard
    let tiny: [1]u8 = undefined;
    let small_err = false;
    * = encodeU64(&tiny, 128) catch {
        small_err = true;
    };
    expect_eq(small_err, true);
}

fn runCase3Progress(): void! {
    const alloc = std.heap.page_allocator;

    {
        let bar = try ProgressBar.start(alloc, ProgressOptions{
            root_name: "build",
            estimated_total: 2,
            disable_printing: true,
        });
        defer bar.end();

        expect_eq(bar.activeCount(), 1);
        expect_eq(bar.completed(), 0);
        expect_eq(bar.estimatedTotal(), 2);

        let compile_node: ProgressNode = bar.startChild("compile", 4);
        expect_eq(bar.activeCount(), 2);
        compile_node.setCompleted(1);
        compile_node.completeOne();
        expect_eq(compile_node.completed(), 2);
        expect_eq(compile_node.percent(), 50);

        let codegen_node: ProgressNode = compile_node.start("codegen", 1);
        expect_eq(bar.activeCount(), 3);
        codegen_node.completeOne();
        codegen_node.end();
        expect_eq(bar.activeCount(), 2);
        // Ending child increments parent's completed count
        expect_eq(compile_node.completed(), 3);

        let snap: [512]u8 = undefined;
        const snap_len = bar.render(&snap);
        expect_eq(snap_len > 16, true);
        // Starts with synchronized output ESC sequence
        expect_eq(bytesEq(snap[0 .. 8], "\x1b[?2026h"), true);

        compile_node.end();
        expect_eq(bar.activeCount(), 1);
        expect_eq(bar.completed(), 1);
        expect_eq(bar.percent(), 50);

        bar.completeOne();
        expect_eq(bar.completed(), 2);
        expect_eq(bar.percent(), 100);
        // Idempotent double end
        bar.end();
        expect_eq(bar.activeCount(), 0);
    }

    // Second instance in same process works cleanly without global panic
    let bar2 = try ProgressBar.start(alloc, ProgressOptions{
        root_name: "second-run",
        estimated_total: 1,
        disable_printing: true,
    });
    defer bar2.end();
    bar2.completeOne();
    expect_eq(bar2.percent(), 100);
}

fn runCase4Zon(): void! {
    const alloc = std.heap.page_allocator;
    const sample =
        "// Package manifest in ZON format\n" ++
        ".{\n" ++
        "    .name = .my_app,\n" ++
        "    .version = \"1.2.3\",\n" ++
        "    .enabled = true,\n" ++
        "    .retries = 3,\n" ++
        "    .threshold = 0.75,\n" ++
        "    .optional = null,\n" ++
        "    .@\"max-hex\" = 0xFF,\n" ++
        "    .tags = .{ \"fast\", \"safe\", \"zero-dep\" },\n" ++
        "    .nested = .{\n" ++
        "        // nested comment\n" ++
        "        .host = \"127.0.0.1\",\n" ++
        "        .port = 8080,\n" ++
        "    },\n" ++
        "}\n";

    expect_eq(validateZon(sample), true);

    let doc = try parseZon(alloc, sample);
    defer doc.deinit();

    expect_eq(doc.root.tag == .Struct, true);
    expect_eq(bytesEq(doc.get("name").?.asEnum().?, "my_app"), true);
    expect_eq(bytesEq(doc.get("version").?.asString().?, "1.2.3"), true);
    expect_eq(doc.get("enabled").?.asBool().?, true);
    expect_eq(doc.get("retries").?.asInt().?, 3);
    expect_eq(doc.get("threshold").?.asFloat().? > 0.74, true);
    expect_eq(doc.get("optional").?.isNull(), true);
    expect_eq(doc.get("max-hex").?.asInt().?, 255);

    const tags = doc.get("tags").?;
    expect_eq(tags.tag == .Array, true);
    expect_eq(tags.len(), 3);
    expect_eq(bytesEq(tags.at(0).?.asString().?, "fast"), true);
    expect_eq(bytesEq(tags.at(2).?.asString().?, "zero-dep"), true);

    const nested = doc.get("nested").?;
    expect_eq(bytesEq(nested.get("host").?.asString().?, "127.0.0.1"), true);
    expect_eq(nested.get("port").?.asInt().?, 8080);

    // Format pretty & compact and verify round-trip parse
    const pretty = try formatZon(alloc, doc.root, 2);
    defer alloc.free(pretty);
    expect_eq(validateZon(pretty), true);

    const compact = try formatZonCompact(alloc, doc.root);
    defer alloc.free(compact);
    let doc2 = try parseZon(alloc, compact);
    defer doc2.deinit();
    expect_eq(doc2.get("nested").?.get("port").?.asInt().?, 8080);

    // Invalid ZON inputs must fail validation
    expect_eq(validateZon(".{ .a = 1, 2 }"), false);
    expect_eq(validateZon(".{ .a = \"unterminated }"), false);
    expect_eq(validateZon(".{ .a = 1 } extra"), false);
}

fn main() {
    test("1. cap:tz loads UTC, America/New_York DST/STD, and Asia/Shanghai", () => {
        runCase1Tz()?;
    });
    test("2. cap:leb128 ULEB128/SLEB128 round-trip and boundary guards", () => {
        runCase2Leb128()?;
    });
    test("3. cap:progress hierarchical nodes, updates, render, and RAII end", () => {
        runCase3Progress()?;
    });
    test("4. cap:zon parses commented ZON config, validates, and formats round-trip", () => {
        runCase4Zon()?;
    });
}
