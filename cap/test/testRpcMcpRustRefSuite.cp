// Wave 38 R6 Reference-Hardened Verification Suite
// Targets: lib/rpc/framing.cp, lib/rpc/jsonrpc.cp, lib/rpc/mcp.cp
// Reference: paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ERR_PARSE,
    ERR_INVALID_REQUEST,
    ERR_METHOD_NOT_FOUND,
    ERR_INVALID_PARAMS,
    JsonRpcId,
    JsonRpcError,
    JsonRpcRequest,
    JsonRpcCodec,
} from "../../lib/rpc/jsonrpc.cp";
import {
    DEFAULT_MAX_FRAME_BYTES,
    DEFAULT_MAX_HEADER_BYTES,
    simdRpcFindCrlf,
    scalarRpcFindCrlf,
    simdRpcAsciiLowerEql,
    scalarRpcAsciiLowerEql,
    ContentLengthFramer,
    LineRpcFramer,
} from "../../lib/rpc/framing.cp";
import {
    MCP_MAX_TOOLS,
    McpToolSpec,
    McpResourceSpec,
    McpServerInfo,
    McpServer,
} from "../../lib/rpc/mcp.cp";

struct XorShift64 {
    state: u64 = 0x57333852_36525043;

    static fn init(seed: u64): XorShift64 {
        const fallback: u64 = (@as(u64, 0x9E3779B9) << 32) | 0x7F4A7C15;
        return XorShift64{ state: if (seed == 0) fallback else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

err DemoToolError {
    SimulatedFailure,
}

fn echoTool(alloc: std.mem.Allocator, args_json: []const u8): anyerror![]const u8 {
    return alloc.dupe(u8, args_json);
}

fn failTool(alloc: std.mem.Allocator, args_json: []const u8): anyerror![]const u8 {
    * = alloc;
    * = args_json;
    return DemoToolError.SimulatedFailure;
}

fn checkTest1FramingBoundaries(): u32 {
    const alloc = std.heap.page_allocator;

    // 1a. 1-byte streaming Content-Length roundtrip across 2 pipelined frames + mixed-case header
    let f1 = ContentLengthFramer.encodeFrame(alloc, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}") catch return 1;
    defer alloc.free(f1);
    const f2 = "X-Trace: abc\r\nCoNtEnT-LeNgTh: 11\r\n\r\n{\"ok\":true}";

    let framer = ContentLengthFramer.init(alloc, DEFAULT_MAX_FRAME_BYTES);
    defer framer.deinit();

    let got_count: usize = 0;
    let i: usize = 0;
    while (i < f1.len) : (i += 1) {
        const frames = framer.feed(f1[i .. i + 1]) catch return 2;
        defer ContentLengthFramer.freeFrames(alloc, frames);
        if (frames.len > 0) {
            if (frames.len != 1) return 3;
            if (!std.mem.eql(u8, frames[0], "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}")) return 4;
            got_count += 1;
        }
    }
    i = 0;
    while (i < f2.len) : (i += 1) {
        const frames = framer.feed(f2[i .. i + 1]) catch return 5;
        defer ContentLengthFramer.freeFrames(alloc, frames);
        if (frames.len > 0) {
            if (frames.len != 1) return 6;
            if (!std.mem.eql(u8, frames[0], "{\"ok\":true}")) return 7;
            got_count += 1;
        }
    }
    if (got_count != 2 || framer.bufferedBytes() != 0) return 8;

    // 1b. Over 16 MiB rejected (FrameTooLarge)
    let over_rejected = false;
    * = framer.feed("Content-Length: 16777217\r\n\r\n") catch |e| {
        if (e == @as(anyerror, error.FrameTooLarge)) over_rejected = true;
    };
    if (!over_rejected) return 9;

    // 1c. Negative & integer overflow Content-Length rejected (InvalidContentLength)
    let neg_rejected = false;
    * = framer.feed("Content-Length: -5\r\n\r\n") catch |e| {
        if (e == @as(anyerror, error.InvalidContentLength)) neg_rejected = true;
    };
    if (!neg_rejected) return 10;

    let ovf_rejected = false;
    * = framer.feed("Content-Length: 99999999999999999999999999\r\n\r\n") catch |e| {
        if (e == @as(anyerror, error.InvalidContentLength)) ovf_rejected = true;
    };
    if (!ovf_rejected) return 11;

    // 1d. Duplicate conflicting Content-Length rejected (InvalidHeader)
    let dup_rejected = false;
    * = framer.feed("Content-Length: 5\r\nContent-Length: 9\r\n\r\nhello") catch |e| {
        if (e == @as(anyerror, error.InvalidHeader)) dup_rejected = true;
    };
    if (!dup_rejected) return 12;

    // 1e. Missing \r\n\r\n header > 16 KiB rejected (HeaderTooLarge)
    let big_hdr = alloc.alloc(u8, DEFAULT_MAX_HEADER_BYTES + 32) catch return 13;
    defer alloc.free(big_hdr);
    @memset(big_hdr, 'A');
    let hdr_too_large = false;
    * = framer.feed(big_hdr) catch |e| {
        if (e == @as(anyerror, error.HeaderTooLarge)) hdr_too_large = true;
    };
    if (!hdr_too_large) return 14;

    // 1f. LineRpcFramer: 1-byte streaming, \r\n & \n, empty lines skipped, over-limit rejected, encodeLine rejects \n
    let lframer = LineRpcFramer.init(alloc, 32);
    defer lframer.deinit();
    const nd_stream = "\n  \r\n{\"a\":1}\r\n{\"b\":2}\n";
    let line_count: usize = 0;
    i = 0;
    while (i < nd_stream.len) : (i += 1) {
        const lines = lframer.feed(nd_stream[i .. i + 1]) catch return 15;
        defer LineRpcFramer.freeLines(alloc, lines);
        for (ln of lines) {
            if (line_count == 0 && !std.mem.eql(u8, ln, "{\"a\":1}")) return 16;
            if (line_count == 1 && !std.mem.eql(u8, ln, "{\"b\":2}")) return 17;
            line_count += 1;
        }
    }
    if (line_count != 2) return 18;

    let line_too_large = false;
    * = lframer.feed("012345678901234567890123456789012345\n") catch |e| {
        if (e == @as(anyerror, error.FrameTooLarge)) line_too_large = true;
    };
    if (!line_too_large) return 19;

    let bad_enc_line = false;
    * = LineRpcFramer.encodeLine(alloc, "a\nb") catch |e| {
        if (e == @as(anyerror, error.InvalidLine)) bad_enc_line = true;
    };
    if (!bad_enc_line) return 20;

    return 0;
}

fn checkTest2JsonRpcAndMcp(): u32 {
    const alloc = std.heap.page_allocator;

    // 2a. Valid IDs (int, string, null) & invalid IDs (bool, float)
    let r_int = JsonRpcCodec.parseRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"ping\"}") catch return 1;
    defer r_int.deinit(alloc);
    if ((r_int.id.asInt() ?? 0) != 42 || r_int.isNotification()) return 2;

    let r_str = JsonRpcCodec.parseRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":\"abc\",\"method\":\"ping\"}") catch return 3;
    defer r_str.deinit(alloc);
    if (!std.mem.eql(u8, r_str.id.asStr() ?? "", "abc")) return 4;

    let r_null = JsonRpcCodec.parseRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":null,\"method\":\"ping\"}") catch return 5;
    defer r_null.deinit(alloc);
    if (!r_null.id.isNull() || r_null.isNotification()) return 6;

    let bad_id = false;
    * = JsonRpcCodec.parseRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":true,\"method\":\"ping\"}") catch |e| {
        if (e == @as(anyerror, error.InvalidId)) bad_id = true;
    };
    if (!bad_id) return 7;

    let bad_float_id = false;
    * = JsonRpcCodec.parseRequest(alloc, "{\"jsonrpc\":\"2.0\",\"id\":1.5,\"method\":\"ping\"}") catch |e| {
        if (e == @as(anyerror, error.InvalidId)) bad_float_id = true;
    };
    if (!bad_float_id) return 8;

    // 2b. McpServer dispatch & standard JSON-RPC error codes
    let srv = McpServer.init(alloc, McpServerInfo{ name: "w38-mcp", version: "1.0.0" });
    defer srv.deinit();

    srv.registerTool(McpToolSpec{ name: "echo", description: "Echo args" }, echoTool) catch return 9;
    srv.registerTool(McpToolSpec{ name: "boom", description: "Always fails" }, failTool) catch return 10;
    srv.registerResource(McpResourceSpec{ uri: "file:///a.txt", name: "a.txt" }) catch return 11;

    // Parse error (-32700)
    const resp_parse = (srv.handleRequestJson(alloc, "{bad json") catch return 12) ?? return 13;
    defer alloc.free(resp_parse);
    let p_parse = JsonRpcCodec.parseResponse(alloc, resp_parse) catch return 14;
    defer p_parse.deinit(alloc);
    if (!p_parse.isError() || p_parse.rpc_error.?.code != ERR_PARSE) return 15;

    // Invalid Request (-32600): wrong jsonrpc version & missing method
    const resp_ver = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"1.0\",\"id\":1,\"method\":\"ping\"}") catch return 16) ?? return 17;
    defer alloc.free(resp_ver);
    let p_ver = JsonRpcCodec.parseResponse(alloc, resp_ver) catch return 18;
    defer p_ver.deinit(alloc);
    if (!p_ver.isError() || p_ver.rpc_error.?.code != ERR_INVALID_REQUEST) return 19;

    const resp_nomethod = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":1}") catch return 20) ?? return 21;
    defer alloc.free(resp_nomethod);
    let p_nom = JsonRpcCodec.parseResponse(alloc, resp_nomethod) catch return 22;
    defer p_nom.deinit(alloc);
    if (!p_nom.isError() || p_nom.rpc_error.?.code != ERR_INVALID_REQUEST) return 23;

    // Method not found (-32601)
    const resp_nf = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"no/such\"}") catch return 24) ?? return 25;
    defer alloc.free(resp_nf);
    let p_nf = JsonRpcCodec.parseResponse(alloc, resp_nf) catch return 26;
    defer p_nf.deinit(alloc);
    if (!p_nf.isError() || p_nf.rpc_error.?.code != ERR_METHOD_NOT_FOUND) return 27;

    // Invalid params (-32602): unknown tool
    const resp_ut = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"missing\"}}") catch return 28) ?? return 29;
    defer alloc.free(resp_ut);
    let p_ut = JsonRpcCodec.parseResponse(alloc, resp_ut) catch return 30;
    defer p_ut.deinit(alloc);
    if (!p_ut.isError() || p_ut.rpc_error.?.code != ERR_INVALID_PARAMS) return 31;

    // Valid tool call + failing tool call (isError: true) + notification
    const resp_ok = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"x\":1}}}") catch return 32) ?? return 33;
    defer alloc.free(resp_ok);
    if (std.mem.indexOf(u8, resp_ok, "\"isError\":false") == null) return 34;

    const resp_fail = (srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":{\"name\":\"boom\",\"arguments\":{}}}") catch return 35) ?? return 36;
    defer alloc.free(resp_fail);
    if (std.mem.indexOf(u8, resp_fail, "\"isError\":true") == null) return 37;

    const notif_resp = srv.handleRequestJson(alloc, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}") catch return 38;
    if (notif_resp != null || !srv.isInitialized()) return 39;

    // 2c. Tool registration cap (MCP_MAX_TOOLS = 256)
    let srv_cap = McpServer.init(alloc, McpServerInfo{});
    defer srv_cap.deinit();
    let idx: usize = 0;
    let name_buf: [16]u8 = [0; 16];
    while (idx < MCP_MAX_TOOLS) : (idx += 1) {
        name_buf[0] = 't';
        name_buf[1] = @as(u8, '0') + @as(u8, @intCast((idx / 100) % 10));
        name_buf[2] = @as(u8, '0') + @as(u8, @intCast((idx / 10) % 10));
        name_buf[3] = @as(u8, '0') + @as(u8, @intCast(idx % 10));
        srv_cap.registerTool(McpToolSpec{ name: name_buf[0..4] }, echoTool) catch return 40;
    }
    let cap_hit = false;
    srv_cap.registerTool(McpToolSpec{ name: "overflow_tool" }, echoTool) catch |e| {
        if (e == @as(anyerror, error.TooManyTools)) cap_hit = true;
    };
    if (!cap_hit) return 41;

    return 0;
}

fn checkTest3SimdDifferential(): u32 {
    let rng = XorShift64.init(0x57333852_3653494D);

    // 3a. simdRpcFindCrlf vs scalarRpcFindCrlf across lengths 0..=96
    let buf: [96]u8 = [0; 96];
    let len: usize = 0;
    while (len <= 96) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            buf[i] = (32 as u8) + @as(u8, @truncate(rng.next() % 90));
        }
        if (simdRpcFindCrlf(buf[0..len], 0) != scalarRpcFindCrlf(buf[0..len], 0)) return 1;
        if (len > 0) {
            const pos = @as(usize, @truncate(rng.next())) % len;
            buf[pos] = if ((rng.next() & 1) == 0) '\r' else '\n';
            const r_simd = simdRpcFindCrlf(buf[0..len], 0);
            const r_scalar = scalarRpcFindCrlf(buf[0..len], 0);
            if (r_simd != r_scalar || (r_simd ?? 999) != pos) return 2;
        }
    }

    // 3b. simdRpcAsciiLowerEql vs scalarRpcAsciiLowerEql across lengths 0..=64
    let a: [64]u8 = [0; 64];
    let b: [64]u8 = [0; 64];
    let slen: usize = 0;
    while (slen <= 64) : (slen += 1) {
        let i: usize = 0;
        while (i < slen) : (i += 1) {
            const base: u8 = (64 as u8) + @as(u8, @truncate(rng.next() % 60)); // covers '@', 'A'..'Z', '[', '`', 'a'..'z', '{'
            a[i] = base;
            if (base >= 'A' && base <= 'Z' && (rng.next() & 1) == 0) {
                b[i] = base + 32;
            } else if (base >= 'a' && base <= 'z' && (rng.next() & 1) == 0) {
                b[i] = base - 32;
            } else {
                b[i] = base;
            }
        }
        if (simdRpcAsciiLowerEql(a[0..slen], b[0..slen]) != scalarRpcAsciiLowerEql(a[0..slen], b[0..slen])) {
            return 3;
        }
        if (!simdRpcAsciiLowerEql(a[0..slen], b[0..slen])) return 4;
        if (slen > 0) {
            const mpos = @as(usize, @truncate(rng.next())) % slen;
            b[mpos] ^= 0x01;
            if (simdRpcAsciiLowerEql(a[0..slen], b[0..slen]) != scalarRpcAsciiLowerEql(a[0..slen], b[0..slen])) {
                return 5;
            }
        }
    }

    return 0;
}

fn checkTest4PrngFuzz1600(): u32 {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64.init(0x57333852_3646555A);

    // 4a. 800 framing fuzz cases (400 ContentLengthFramer + 400 LineRpcFramer)
    let pbuf: [64]u8 = [0; 64];
    let iter: usize = 0;
    while (iter < 400) : (iter += 1) {
        let framer = ContentLengthFramer.init(alloc, 1024);
        defer framer.deinit();
        const plen: usize = @as(usize, @truncate(rng.next())) % 48;
        let k: usize = 0;
        while (k < plen) : (k += 1) {
            pbuf[k] = (33 as u8) + @as(u8, @truncate(rng.next() % 90));
        }
        const enc = ContentLengthFramer.encodeFrame(alloc, pbuf[0..plen]) catch return 1;
        defer alloc.free(enc);

        let off: usize = 0;
        let matched = false;
        while (off < enc.len) {
            const step: usize = (1 as usize) + (@as(usize, @truncate(rng.next())) % 7);
            const end = if (off + step > enc.len) enc.len else off + step;
            const frames = framer.feed(enc[off..end]) catch return 2;
            defer ContentLengthFramer.freeFrames(alloc, frames);
            if (frames.len == 1) {
                if (!std.mem.eql(u8, frames[0], pbuf[0..plen])) return 3;
                matched = true;
            }
            off = end;
        }
        if (!matched) return 4;
    }

    iter = 0;
    while (iter < 400) : (iter += 1) {
        let lframer = LineRpcFramer.init(alloc, 128);
        defer lframer.deinit();
        const plen: usize = (1 as usize) + (@as(usize, @truncate(rng.next())) % 40);
        let k: usize = 0;
        while (k < plen) : (k += 1) {
            pbuf[k] = (33 as u8) + @as(u8, @truncate(rng.next() % 90)); // non-whitespace, no \r/\n
        }
        const enc = LineRpcFramer.encodeLine(alloc, pbuf[0..plen]) catch return 5;
        defer alloc.free(enc);
        const lines = lframer.feed(enc) catch return 6;
        defer LineRpcFramer.freeLines(alloc, lines);
        if (lines.len != 1 || !std.mem.eql(u8, lines[0], pbuf[0..plen])) return 7;
    }

    // 4b. 800 JSON-RPC 2.0 & MCP fuzz cases (malformed + structured, every response valid JSON-RPC 2.0)
    let srv = McpServer.init(alloc, McpServerInfo{ name: "fuzz-mcp", version: "1.0.0" });
    defer srv.deinit();
    srv.registerTool(McpToolSpec{ name: "echo" }, echoTool) catch return 8;
    srv.registerTool(McpToolSpec{ name: "boom" }, failTool) catch return 9;

    const templates: [8][]const u8 = [
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}",
        "{\"jsonrpc\":\"2.0\",\"id\":\"s1\",\"method\":\"initialize\"}",
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}",
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"k\":1}}}",
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":{\"name\":\"boom\"}}",
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"unknown/op\"}",
        "{\"jsonrpc\":\"1.0\",\"id\":6,\"method\":\"ping\"}",
        "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"tools/call\",\"params\":123}",
    ];
    const fuzz_alpha = "{}\"[]:,0123456789abcdefghijklmnopqrstuvwxyz \t\n\\";
    let fbuf: [80]u8 = [0; 80];

    iter = 0;
    while (iter < 800) : (iter += 1) {
        let input: []const u8 = "";
        if ((iter & 1) == 0) {
            input = templates[@as(usize, @truncate(rng.next())) % 8];
        } else {
            const flen: usize = @as(usize, @truncate(rng.next())) % 64;
            let k: usize = 0;
            while (k < flen) : (k += 1) {
                fbuf[k] = fuzz_alpha[@as(usize, @truncate(rng.next())) % fuzz_alpha.len];
            }
            input = fbuf[0..flen];
        }

        const resp_opt = srv.handleRequestJson(alloc, input) catch return 10;
        if (resp := resp_opt) {
            defer alloc.free(resp);
            let parsed = JsonRpcCodec.parseResponse(alloc, resp) catch return 11;
            defer parsed.deinit(alloc);
            if (!parsed.isSuccess() && !parsed.isError()) return 12;
        }
    }

    return 0;
}

fn main() {
    test("1. Content-Length & NDJSON 1-Byte Streaming, 16MiB Cap, Conflict & Overflow Guards", () => {
        expect_eq(checkTest1FramingBoundaries(), 0 as u32);
    });

    test("2. JSON-RPC 2.0 & MCP Standard Error Codes (-32700..-32602), Dispatch & Tool Cap", () => {
        expect_eq(checkTest2JsonRpcAndMcp(), 0 as u32);
    });

    test("3. 16-Lane SIMD vs Scalar 0-Diff Differential Parity", () => {
        expect_eq(checkTest3SimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG Fuzzing (800 Framing + 800 JSON-RPC/MCP = 1,600 Cases)", () => {
        expect_eq(checkTest4PrngFuzz1600(), 0 as u32);
    });
}
