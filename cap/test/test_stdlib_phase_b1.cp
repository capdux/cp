// cap/test/test_stdlib_phase_b1.cp
// Comprehensive E2E Regression Suite for Phase A & Phase B1 Standard Library Modules:
// cap:col, cap:conf, cap:pack, cap:redis, cap:cli, cap:watch, cap:patch, cap:bench

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { BitSet256, BoundedArray } from "cap:col";
import { DotEnv, Ini, TOML, YAML, JSON5 } from "cap:conf";
import { TarHeader, TarReader, TAR_BLOCK_SIZE, Zlib, Zstd } from "cap:pack";
import { RESPType, RESPParser, RESPEncoder } from "cap:redis";
import { CliArgs, AnsiColor, ProgressBar } from "cap:cli";
import { WatchMask, WatchEvent, WatchEventKind, FileWatcher } from "cap:watch";
import { UnifiedHunkHeader, UnifiedDiff } from "cap:patch";
import { HwTimer, Stopwatch, BenchStats } from "cap:bench";

fn main() {
    test("1. cap:col — BitSet256 multi-word bitset & BoundedArray", () => {
        let bs = BitSet256.initEmpty();
        expect_eq(bs.capacity(), 256 as usize);
        expect_eq(bs.count(), 0 as usize);

        bs.set(5);
        bs.set(70);
        bs.set(200);
        expect_eq(bs.isSet(5), true);
        expect_eq(bs.isSet(70), true);
        expect_eq(bs.isSet(200), true);
        expect_eq(bs.isSet(71), false);
        expect_eq(bs.count(), 3 as usize);
        expect_eq(bs.findFirstSet().?, 5 as usize);

        bs.unset(5);
        expect_eq(bs.findFirstSet().?, 70 as usize);

        let arr = BoundedArray<i32, 4>.init();
        expect_eq(arr.append(42), true);
        expect_eq(arr.get(0), 42);
    });

    test("2. cap:conf — Unified DotEnv, INI, TOML, YAML & JSON5 parsing", () => {
        const toml_doc = TOML.parse("title = \"CP Engine\"\n[server]\nhost = \"0.0.0.0\"\nport = 9000 # listen port\n");
        expect_eq(std.mem.eql(u8, toml_doc.getRoot("title").?, "CP Engine"), true);
        expect_eq(toml_doc.hasTable("server"), true);
        expect_eq(std.mem.eql(u8, toml_doc.get("server", "host").?, "0.0.0.0"), true);
        expect_eq(std.mem.eql(u8, toml_doc.get("server", "port").?, "9000"), true);

        const yaml_doc = YAML.parse("---\napp: antigravity\ndatabase:\n  driver: sqlite\n  pool: 16 # max conns\n");
        expect_eq(std.mem.eql(u8, yaml_doc.getRoot("app").?, "antigravity"), true);
        expect_eq(std.mem.eql(u8, yaml_doc.get("database", "driver").?, "sqlite"), true);
        expect_eq(std.mem.eql(u8, yaml_doc.get("database", "pool").?, "16"), true);

        let json5_out: [128]u8 = [0; 128];
        const cleaned = JSON5.stripCommentsAndTrailingCommas("{\n  // comment\n  \"a\": 1,\n}", json5_out[0..128]);
        expect_eq(std.mem.indexOf(u8, cleaned, "//") == null, true);
        expect_eq(std.mem.indexOf(u8, cleaned, "\"a\": 1}") != null || std.mem.indexOf(u8, cleaned, "\"a\": 1\n}") != null, true);
    });

    test("3. cap:pack — POSIX USTAR TarHeader encode & TarReader zero-copy iteration", () => {
        expect_eq(Zlib.bound(512) > 512, true);
        expect_eq(Zstd.bound(512) > 512, true);

        let archive_buf: [1536]u8 = [0; 1536];

        const payload = "Hello Tar Archive!";
        const ok = TarHeader.encodeHeader(
            archive_buf[0..512],
            "hello.txt",
            payload.len as u64,
            0o644,
            1700000000,
            '0' as u8
        );
        expect_eq(ok, true);

        let k: usize = 0;
        while (k < payload.len) {
            archive_buf[512 + k] = payload[k];
            k += 1;
        }

        let reader = TarReader.init(archive_buf[0..1536]);
        const entry_opt = reader.next();
        expect_eq(entry_opt != null, true);
        const entry = entry_opt.?;
        expect_eq(std.mem.eql(u8, entry.name, "hello.txt"), true);
        expect_eq(entry.size, payload.len as u64);
        expect_eq(entry.isRegularFile(), true);
        expect_eq(std.mem.eql(u8, entry.data, payload), true);
        expect_eq(reader.next() == null, true);
    });

    test("4. cap:redis — RESP2/RESP3 Frame Parser & Command Encoder", () => {
        let out_buf: [128]u8 = [0; 128];
        const n = RESPEncoder.encodeCommand3(out_buf[0..128], "SET", "key", "val");
        expect_eq(std.mem.eql(u8, out_buf[0..n], "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$3\r\nval\r\n"), true);

        const f_ok = RESPParser.parseFrame("+PONG\r\n").?;
        expect_eq(f_ok.kind == RESPType.SimpleString, true);
        expect_eq(std.mem.eql(u8, f_ok.str_val, "PONG"), true);
        expect_eq(f_ok.bytes_consumed, 7 as usize);

        const f_int = RESPParser.parseFrame(":-42\r\n").?;
        expect_eq(f_int.kind == RESPType.Integer, true);
        expect_eq(f_int.int_val, -42 as i64);

        const f_bulk = RESPParser.parseFrame("$5\r\nhello\r\n").?;
        expect_eq(f_bulk.kind == RESPType.BulkString, true);
        expect_eq(std.mem.eql(u8, f_bulk.str_val, "hello"), true);
        expect_eq(f_bulk.bytes_consumed, 11 as usize);

        const f_bool = RESPParser.parseFrame("#t\r\n").?;
        expect_eq(f_bool.kind == RESPType.Boolean, true);
        expect_eq(f_bool.bool_val, true);
    });

    test("5. cap:cli — CliArgs flag/option parser, AnsiColor stripper & ProgressBar", () => {
        const raw: [5][]const u8 = ["build", "--target=linux-x64", "-v", "--release", "src/main.cp"];
        const cli = CliArgs.parse(raw[0..5]);
        expect_eq(cli.hasFlag("v"), true);
        expect_eq(cli.hasFlag("release"), true);
        expect_eq(cli.hasFlag("debug"), false);
        expect_eq(std.mem.eql(u8, cli.getOption("target").?, "linux-x64"), true);
        expect_eq(cli.positional_count, 2 as usize);
        expect_eq(std.mem.eql(u8, cli.positional(0).?, "build"), true);
        expect_eq(std.mem.eql(u8, cli.positional(1).?, "src/main.cp"), true);

        let strip_buf: [64]u8 = [0; 64];
        const plain = AnsiColor.stripAnsi("\x1b[32mPASS\x1b[0m", strip_buf[0..64]);
        expect_eq(std.mem.eql(u8, plain, "PASS"), true);

        let bar = ProgressBar.init(100, 10);
        bar.tick(50);
        expect_eq(bar.percent(), 50 as u8);
        let bar_buf: [16]u8 = [0; 16];
        const rendered = bar.renderBar(bar_buf[0..16]);
        expect_eq(std.mem.eql(u8, rendered, "[#####-----]"), true);
    });

    test("6. cap:watch — WatchEvent inotify mask classification & FileWatcher table", () => {
        const ev = WatchEvent {
            wd: 1,
            mask: WatchMask.IN_CREATE | WatchMask.IN_ISDIR,
            cookie: 0,
            name: "subdir",
        };
        expect_eq(ev.kind() == WatchEventKind.Created, true);
        expect_eq(ev.isDirectory(), true);

        let watcher = FileWatcher.init();
        const wd1 = watcher.addWatch("/data/dev/cp/lib", true);
        const wd2 = watcher.addWatch("/data/dev/cp/cap.json", false);
        expect_eq(watcher.activeCount(), 2 as usize);
        expect_eq(std.mem.eql(u8, watcher.findPath(wd1).?, "/data/dev/cp/lib"), true);
        expect_eq(watcher.removeWatch(wd2), true);
        expect_eq(watcher.activeCount(), 1 as usize);
    });

    test("7. cap:patch — UnifiedHunkHeader parser & UnifiedDiff analyzer", () => {
        const hdr = UnifiedHunkHeader.parse("@@ -12,4 +15,6 @@ fn example()").?;
        expect_eq(hdr.old_range.start_line, 12 as usize);
        expect_eq(hdr.old_range.line_count, 4 as usize);
        expect_eq(hdr.new_range.start_line, 15 as usize);
        expect_eq(hdr.new_range.line_count, 6 as usize);

        const diff_text = "--- a/foo.cp\n+++ b/foo.cp\n@@ -1,3 +1,3 @@\n line1\n-old_line\n+new_line\n line3\n";
        const stats = UnifiedDiff.analyzePatch(diff_text);
        expect_eq(stats.hunk_count, 1 as usize);
        expect_eq(stats.equal_lines, 2 as usize);
        expect_eq(stats.deleted_lines, 1 as usize);
        expect_eq(stats.inserted_lines, 1 as usize);

        let out_buf: [128]u8 = [0; 128];
        const patched = UnifiedDiff.renderHunkNewSide(diff_text, out_buf[0..128]);
        expect_eq(std.mem.eql(u8, patched, "line1\nnew_line\nline3\n"), true);
    });

    test("8. cap:bench — HwTimer, Stopwatch & BenchStats percentile statistics", () => {
        const sw = Stopwatch.start();
        const t0 = HwTimer.nowNs();
        expect_eq(t0 > 0, true);
        expect_eq(sw.readNs() >= 0, true);

        let stats = BenchStats.init();
        stats.record(300);
        stats.record(100);
        stats.record(500);
        stats.record(200);
        stats.record(400);

        expect_eq(stats.count, 5 as usize);
        expect_eq(stats.min_ns, 100 as u64);
        expect_eq(stats.max_ns, 500 as u64);
        expect_eq(stats.meanNs(), 300 as u64);
        expect_eq(stats.p50Ns(), 300 as u64);
        expect_eq(stats.opsPerSec() > 0, true);
    });
}
