// cap/test/test_core_runtime_bundler.cp
// ============================================================================
// Offline Regression Suite for CP Core Runtime & Bundler Modules:
//   cap:bundler, cap:fs, cap:stream, cap:serde, cap:test, cap:beam, cap:reflect
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    BundleTarget,
    OutputFormat,
    ChunkKind,
    ChunkOrder,
    ChunkEntry,
    BundleConfig,
    BundleV2,
} from "cap:bundler";

import {
    tmpDir,
    tmpPath,
    writeFileSync,
    readFileSync,
    writeAtomic,
    copyFile,
    rename,
    exists,
    statSize,
    isDirectory,
    mkdir,
    remove,
    rmdir,
} from "cap:fs";

import {
    SeekOrigin,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    copyStream,
    copyN,
    MaxBuf,
    MaxBufKind,
    Pipe,
} from "cap:stream";

import {
    JSON,
    JsonTag,
} from "cap:serde";

import {
    SystemAction,
    PID,
} from "cap:beam";

import {
    self_pid,
} from "cap:actor";

struct AppManifest {
    name: []const u8,
    port: i64,
    enabled: bool,
}

// ============================================================================
// 1. cap:bundler — BundleConfig, ChunkOrder Topology Sort & BundleV2 State
// ============================================================================
fn testBundlerCore(): !void {
    const def_cfg = BundleConfig.defaultConfig();
    expect_eq(def_cfg.target, BundleTarget.CpNative);
    expect_eq(def_cfg.format, OutputFormat.Esm);
    expect_eq(def_cfg.minify, false);

    const prod_cfg = BundleConfig.productionBrowser("dist/app");
    expect_eq(prod_cfg.target, BundleTarget.Browser);
    expect_eq(prod_cfg.minify, true);
    expect_eq(prod_cfg.splitting, true);
    expect_eq(std.mem.eql(u8, prod_cfg.out_dir, "dist/app"), true);

    // Deterministic ChunkOrder sorting (distance first, then tie_breaker)
    let orders: [4]ChunkOrder = [
        ChunkOrder { source_index: 20, distance: 3, tie_breaker: 1 },
        ChunkOrder { source_index: 10, distance: 1, tie_breaker: 5 },
        ChunkOrder { source_index: 11, distance: 1, tie_breaker: 2 },
        ChunkOrder { source_index: 15, distance: 2, tie_breaker: 0 },
    ];
    ChunkOrder.sort(orders[0..4]);
    expect_eq(orders[0].source_index, 11);
    expect_eq(orders[1].source_index, 10);
    expect_eq(orders[2].source_index, 15);
    expect_eq(orders[3].source_index, 20);

    // BundleV2 chunk emission, lookup & compression ratio
    let bundler = BundleV2.init(prod_cfg);
    bundler.addSourceBytes(10000);
    bundler.markTopLevelAwait();
    expect_eq(bundler.has_top_level_await, true);

    expect_eq(bundler.emitChunk(ChunkEntry {
        name: "entry.js",
        kind: ChunkKind.EntryPoint,
        source_index: 11,
        size_bytes: 3000,
        is_executable: true,
    }), true);
    expect_eq(bundler.emitChunk(ChunkEntry {
        name: "shared.js",
        kind: ChunkKind.SharedChunk,
        source_index: 10,
        size_bytes: 1500,
        is_executable: false,
    }), true);

    expect_eq(bundler.chunkCount(), 2);
    expect_eq(bundler.entry_count, 1);
    expect_eq(bundler.total_output_bytes, 4500);
    expect_eq(bundler.compressionRatioPermille(), 450);

    const found = bundler.findChunk("shared.js");
    expect_eq(found != null, true);
    expect_eq(found.?.size_bytes, 1500);
    expect_eq(found.?.kind, ChunkKind.SharedChunk);
    expect_eq(bundler.findChunk("missing.js") == null, true);

    bundler.reset();
    expect_eq(bundler.chunkCount(), 0);
    expect_eq(bundler.total_output_bytes, 0);
}

// ============================================================================
// 2. cap:fs — tmpDir, tmpPath, writeAtomic, Sync IO, copyFile, rename & mkdir
// ============================================================================
fn testFsLifecycleAndAtomicWrite(): !void {
    const tdir = tmpDir();
    expect_eq(tdir.len > 0, true);
    expect_eq(isDirectory(tdir), true);

    let p1_buf: [256]u8 = undefined;
    const p1 = tmpPath("cap_core_fs_a", ".txt", &p1_buf);
    let p2_buf: [256]u8 = undefined;
    const p2 = tmpPath("cap_core_fs_b", ".txt", &p2_buf);
    let p3_buf: [256]u8 = undefined;
    const p3 = tmpPath("cap_core_fs_c", ".txt", &p3_buf);
    expect_eq(std.mem.eql(u8, p1, p2), false);

    // Atomic write & verify
    writeAtomic(p1, "CP Atomic Payload V1");
    expect_eq(exists(p1), true);
    expect_eq(statSize(p1), 20);
    const content1 = readFileSync(p1);
    expect_eq(std.mem.eql(u8, content1, "CP Atomic Payload V1"), true);

    // Overwrite via writeFileSync, copyFile, rename
    writeFileSync(p1, "Updated V2");
    expect_eq(statSize(p1), 10);
    copyFile(p1, p2);
    expect_eq(exists(p2), true);
    expect_eq(std.mem.eql(u8, readFileSync(p2), "Updated V2"), true);

    rename(p2, p3);
    expect_eq(exists(p2), false);
    expect_eq(exists(p3), true);

    // Directory lifecycle
    let dir_buf: [256]u8 = undefined;
    const dpath = tmpPath("cap_core_fs_dir", "", &dir_buf);
    mkdir(dpath);
    expect_eq(exists(dpath), true);
    expect_eq(isDirectory(dpath), true);

    // Cleanup
    remove(p1);
    remove(p3);
    rmdir(dpath);
    expect_eq(exists(p1), false);
    expect_eq(exists(p3), false);
    expect_eq(exists(dpath), false);
}

// ============================================================================
// 3. cap:stream — BufferReader/Writer, Buffered IO, copyN/copyStream, MaxBuf, Pipe
// ============================================================================
fn testStreamFacade(): !void {
    const raw = "header:payload-stream-data";
    let src = new BufferReader(raw[0..raw.len]);

    let w_hdr = new BufferWriter();
    expect_eq(copyN(src, w_hdr, 6) catch 0, 6);
    expect_eq(std.mem.eql(u8, w_hdr.bytes(), "header"), true);

    // Skip ':' and copy rest
    expect_eq(src.seek(1, SeekOrigin.Current) catch 999, 7);
    let w_body = new BufferWriter();
    expect_eq(copyStream(src, w_body) catch 0, 19);
    expect_eq(std.mem.eql(u8, w_body.bytes(), "payload-stream-data"), true);

    // BufferedReader & BufferedWriter
    let br_raw = new BufferReader("XYZ");
    let br = new BufferedReader<BufferReader>(br_raw);
    expect_eq(br.readByte() catch 0, 88); // 'X'
    expect_eq(br.readByte() catch 0, 89); // 'Y'
    expect_eq(br.readByte() catch 0, 90); // 'Z'

    let bw_sink = new BufferWriter();
    let bw = new BufferedWriter<BufferWriter>(bw_sink);
    expect_eq(bw.write("cap:") catch 0, 4);
    expect_eq(bw.write("stream") catch 0, 6);
    bw.flush() catch {};
    expect_eq(std.mem.eql(u8, bw_sink.bytes(), "cap:stream"), true);

    // MaxBuf & OS Pipe via cap:stream
    let guard = MaxBuf.withKind(12, MaxBufKind.Stderr);
    expect_eq(guard.onReadBytes(8) catch false, true);
    expect_eq(guard.onReadBytes(5) catch false, false);
    expect_eq(guard.isExceeded(), true);

    let pipe = Pipe.create() catch unreachable;
    expect_eq(pipe.writer.write("stream-ipc") catch 0, 10);
    let buf: [16]u8 = [0 as u8; 16];
    const got = pipe.reader.read(buf[0..16]) catch 0;
    expect_eq(got, 10);
    expect_eq(std.mem.eql(u8, buf[0..10], "stream-ipc"), true);
    pipe.writer.close();
    pipe.reader.close();
}

// ============================================================================
// 4. cap:serde — Dynamic DOM & Compile-time Typed JSON Deserialization
// ============================================================================
fn testSerdeJson(): !void {
    const json_src = "{\"name\":\"cp-runtime\",\"port\":9000,\"enabled\":true}";

    const val = try JSON.parse(std.heap.page_allocator, json_src);
    expect_eq(val.tag, JsonTag.Object);

    const parsed = try JSON.parseTyped(AppManifest, std.heap.page_allocator, json_src);
    expect_eq(std.mem.eql(u8, parsed.name, "cp-runtime"), true);
    expect_eq(parsed.port, 9000);
    expect_eq(parsed.enabled, true);
}

// ============================================================================
// 5. cap:beam & cap:actor — SystemAction Opcodes, PID & self_pid()
// ============================================================================
fn testBeamAndActorRuntime(): !void {
    expect_eq(@intFromEnum(SystemAction.WAKE), 1);
    expect_eq(@intFromEnum(SystemAction.SLEEP), 2);
    expect_eq(@intFromEnum(SystemAction.FS_READ), 20);
    expect_eq(@intFromEnum(SystemAction.HTTP_FETCH), 30);
    expect_eq(@intFromEnum(SystemAction.ERROR), 99);

    const pid = PID { node_id: 7, local_id: 4096 };
    expect_eq(pid.node_id, 7);
    expect_eq(pid.local_id, 4096);

    const cur = self_pid();
    expect_eq(cur.local_id >= 0, true);
}

fn main() {
    test("1. cap:bundler BundleConfig, ChunkOrder Sort & BundleV2 State", () => {
        try testBundlerCore();
    });
    test("2. cap:fs tmpDir, tmpPath, writeAtomic, Sync IO, copyFile & mkdir", () => {
        try testFsLifecycleAndAtomicWrite();
    });
    test("3. cap:stream Buffer Streams, Buffered IO, copyN/copyStream, MaxBuf & Pipe", () => {
        try testStreamFacade();
    });
    test("4. cap:serde Dynamic DOM & Typed JSON Deserialization", () => {
        try testSerdeJson();
    });
    test("5. cap:beam & cap:actor SystemAction, PID & self_pid Runtime", () => {
        try testBeamAndActorRuntime();
    });
}
