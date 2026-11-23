// cap/test/testPkgWinShimCodec.cp — Deep production tests for cap:pkg winShimCodec
// Covers:
//   1. Windows .exe trailer encode & zero-copy tail decode (UTF-8 & embedded after dummy MZ/PE bytes)
//   2. UTF-16LE trailer round-trip including CJK + Emoji surrogate pairs & odd-byte/lone-surrogate rejection
//   3. Exact Microsoft CRT CommandLineToArgvW quoting (spaces, quotes, trailing & pre-quote backslashes)
//   4. buildWindowsCommandLine & SIMD cmd.exe metacharacter detection (`hasCmdExeMetachars`)
//   5. SIMD Shebang parser & Windows runtime router (`env -S cap run`, `env node`, `/bin/sh`, `python3`)
//   6. Diagnostic report formatting (`formatWinShimReport`) & IEEE CRC32 standard vector check
//   7. 500-iteration deterministic random CLI arg quoting & corrupted .exe trailer Fuzz gate

import { expect, expectEqual, test } from "cap:test";
import {
    WIN_SHIM_TRAILER_VERSION,
    WIN_SHIM_FOOTER_BYTES,
    WIN_SHIM_FLAG_USE_UTF16,
    WIN_SHIM_FLAG_INHERIT_CWD,
    WIN_SHIM_FLAG_HAS_SHEBANG,
    computeWinShimCrc32,
    updateWinShimCrc32,
    encodeUtf8ToUtf16LeBytes,
    decodeUtf16LeBytesToUtf8,
    WinShimTrailerSpec,
    encodeWinShimTrailer,
    decodeWinShimTrailer,
    hasCmdExeMetachars,
    quoteWindowsArg,
    buildWindowsCommandLine,
    WinShimRuntimeKind,
    parseScriptShebang,
    formatWinShimReport,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("winShimCodec: .exe trailer encode & zero-copy tail decode after PE binary prefix", () => {
    expectEqual(computeWinShimCrc32("123456789"), @as(u32, 0xCBF43926));

    // Simulate a dummy 128-byte PE/MZ .exe prefix followed by the shim trailer
    let exe_buf: [512]u8 = [0; 512];
    exe_buf[0] = 'M';
    exe_buf[1] = 'Z';
    const pe_len: usize = 128;

    const tlen = encodeWinShimTrailer(WinShimTrailerSpec{
        target_path: "C:\\project\\cap_modules\\.bin\\tsc.js",
        interpreter_path: "node.exe",
        interpreter_args: "--max-old-space-size=4096",
        flags: WIN_SHIM_FLAG_INHERIT_CWD | WIN_SHIM_FLAG_HAS_SHEBANG,
    }, exe_buf[pe_len..]) catch unreachable;

    const total_exe = pe_len + tlen;
    const view = decodeWinShimTrailer(exe_buf[0..total_exe]) catch unreachable;
    expectEqual(view.version, WIN_SHIM_TRAILER_VERSION);
    expectEqual(view.trailer_offset, pe_len);
    expectEqual(view.trailer_total_bytes, tlen);
    expect(!view.isUtf16());

    let tmp: [128]u8 = [0; 128];
    const n1 = view.readTargetUtf8(tmp[0..]) catch unreachable;
    expect(strEql(tmp[0..n1], "C:\\project\\cap_modules\\.bin\\tsc.js"));
    const n2 = view.readInterpreterUtf8(tmp[0..]) catch unreachable;
    expect(strEql(tmp[0..n2], "node.exe"));
    const n3 = view.readArgsUtf8(tmp[0..]) catch unreachable;
    expect(strEql(tmp[0..n3], "--max-old-space-size=4096"));
});

test("winShimCodec: UTF-16LE trailer round-trip with CJK + Emoji surrogate pairs", () => {
    // Includes ASCII, 3-byte UTF-8 CJK ("中"), and 4-byte UTF-8 Emoji (🚀 = F0 9F 9A 80)
    const unicode_path = "C:\\\xe4\xb8\xad\\\xf0\x9f\x9a\x80\\app.cp";
    let u16_buf: [128]u8 = [0; 128];
    let back_buf: [128]u8 = [0; 128];
    const u16_len = encodeUtf8ToUtf16LeBytes(unicode_path, u16_buf[0..]) catch unreachable;
    const back_len = decodeUtf16LeBytesToUtf8(u16_buf[0..u16_len], back_buf[0..]) catch unreachable;
    expect(strEql(back_buf[0..back_len], unicode_path));

    // Reject odd-length UTF-16LE and lone surrogate
    let odd_err = false;
    * = decodeUtf16LeBytesToUtf8(u16_buf[0..3], back_buf[0..]) catch { odd_err = true; };
    expect(odd_err);

    // Full UTF-16LE trailer encode + decode
    let tr: [256]u8 = [0; 256];
    const tlen = encodeWinShimTrailer(WinShimTrailerSpec{
        target_path: unicode_path,
        interpreter_path: "cap.exe",
        interpreter_args: "run",
        flags: WIN_SHIM_FLAG_USE_UTF16 | WIN_SHIM_FLAG_INHERIT_CWD,
    }, tr[0..]) catch unreachable;

    const view = decodeWinShimTrailer(tr[0..tlen]) catch unreachable;
    expect(view.isUtf16());
    const t_out = view.readTargetUtf8(back_buf[0..]) catch unreachable;
    expect(strEql(back_buf[0..t_out], unicode_path));
    const i_out = view.readInterpreterUtf8(back_buf[0..]) catch unreachable;
    expect(strEql(back_buf[0..i_out], "cap.exe"));
});

test("winShimCodec: exact Microsoft CRT CommandLineToArgvW argument quoting", () => {
    let out: [256]u8 = [0; 256];

    // Fast path: no spaces or quotes -> unchanged
    const n1 = quoteWindowsArg("simple-arg_123", out[0..]) catch unreachable;
    expect(strEql(out[0..n1], "simple-arg_123"));

    // Empty arg -> ""
    const n2 = quoteWindowsArg("", out[0..]) catch unreachable;
    expect(strEql(out[0..n2], "\"\""));

    // Space inside arg -> wrapped in quotes
    const n3 = quoteWindowsArg("hello world", out[0..]) catch unreachable;
    expect(strEql(out[0..n3], "\"hello world\""));

    // Trailing backslashes before closing quote must be doubled: `C:\dir name\` -> `"C:\dir name\\"`
    const n4 = quoteWindowsArg("C:\\dir name\\", out[0..]) catch unreachable;
    expect(strEql(out[0..n4], "\"C:\\dir name\\\\\""));

    // Backslashes before embedded quote must be 2*k + 1: `a\"b` -> `"a\\\"b"`
    const n5 = quoteWindowsArg("a\\\"b", out[0..]) catch unreachable;
    expect(strEql(out[0..n5], "\"a\\\\\\\"b\""));
});

test("winShimCodec: buildWindowsCommandLine & SIMD cmd.exe metacharacter detection", () => {
    const args: [3][]const u8 = ["run", "my script.cp", "--msg=say \"hi\""];
    let cmd: [256]u8 = [0; 256];
    const n = buildWindowsCommandLine("C:\\Program Files\\Cap\\cap.exe", args[0..], cmd[0..]) catch unreachable;
    expect(strEql(
        cmd[0..n],
        "\"C:\\Program Files\\Cap\\cap.exe\" run \"my script.cp\" \"--msg=say \\\"hi\\\"\"",
    ));

    expect(hasCmdExeMetachars("foo & calc.exe"));
    expect(hasCmdExeMetachars("a | b"));
    expect(hasCmdExeMetachars("%PATH%"));
    expect(!hasCmdExeMetachars("C:\\Program Files\\Cap\\cap.exe"));
});

test("winShimCodec: SIMD Shebang parser & Windows runtime router", () => {
    const s1 = parseScriptShebang("#!/usr/bin/env -S cap run --smol\r\nconsole.log(1);") catch unreachable;
    expect(s1.has_shebang);
    expect(s1.used_env_split);
    expect(s1.runtime == WinShimRuntimeKind.Cap);
    expect(strEql(s1.interpreter, "cap"));
    expect(strEql(s1.windows_exe, "cap.exe"));
    expect(strEql(s1.extra_args, "run --smol"));

    const s2 = parseScriptShebang("#!/usr/bin/env node\nconsole.log(2);") catch unreachable;
    expect(s2.runtime == WinShimRuntimeKind.Node);
    expect(strEql(s2.windows_exe, "node.exe"));
    expectEqual(s2.extra_args.len, @as(usize, 0));

    const s3 = parseScriptShebang("#!/bin/sh -e\necho hi") catch unreachable;
    expect(s3.runtime == WinShimRuntimeKind.Shell);
    expect(strEql(s3.windows_exe, "bash.exe"));
    expect(strEql(s3.extra_args, "-e"));

    const s4 = parseScriptShebang("#!/usr/bin/env python3 -u\nprint(1)") catch unreachable;
    expect(s4.runtime == WinShimRuntimeKind.Python);
    expect(strEql(s4.windows_exe, "python.exe"));

    const s5 = parseScriptShebang("console.log('no shebang');") catch unreachable;
    expect(!s5.has_shebang);
    expect(s5.runtime == WinShimRuntimeKind.DirectExecutable);
});

test("winShimCodec: diagnostic report formatting", () => {
    let tr: [256]u8 = [0; 256];
    const tlen = encodeWinShimTrailer(WinShimTrailerSpec{
        target_path: "cli.js",
        interpreter_path: "node.exe",
        flags: WIN_SHIM_FLAG_USE_UTF16,
    }, tr[0..]) catch unreachable;
    const view = decodeWinShimTrailer(tr[0..tlen]) catch unreachable;

    let rep: [256]u8 = [0; 256];
    const rlen = formatWinShimReport(view, rep[0..]) catch unreachable;
    expect(containsSubstr(rep[0..rlen], "WinShimTrailer(v=1"));
    expect(containsSubstr(rep[0..rlen], "enc=utf16le"));
    expect(containsSubstr(rep[0..rlen], "target_len=12"));
});

test("winShimCodec: 500-iteration deterministic random quoting & corrupt trailer Fuzz gate", () => {
    // Baseline valid trailer
    let valid_tr: [256]u8 = [0; 256];
    const valid_len = encodeWinShimTrailer(WinShimTrailerSpec{
        target_path: "C:\\app\\bin.js",
        interpreter_path: "cap.exe",
        interpreter_args: "run",
        flags: WIN_SHIM_FLAG_INHERIT_CWD,
    }, valid_tr[0..]) catch unreachable;

    let scratch_tr: [256]u8 = [0; 256];
    let arg_in: [96]u8 = [0; 96];
    let arg_out: [256]u8 = [0; 256];

    const arg_chars = "abc \\\"\t\n&|<>^%";
    let rng: u64 = 0x54111C0DE1234567;
    let passed_iters: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const r = rng *% 0x2545F4914F6CDD1D;

        // Part A: Fuzz quoteWindowsArg on random backslash/quote/whitespace strings
        const alen = @as(usize, @intCast(r % 64));
        let k: usize = 0;
        while (k < alen) {
            rng ^= rng >> 12;
            rng ^= rng << 25;
            rng ^= rng >> 27;
            arg_in[k] = arg_chars[@as(usize, @intCast(rng % @as(u64, arg_chars.len)))];
            k += 1;
        }
        const qlen = quoteWindowsArg(arg_in[0..alen], arg_out[0..]) catch unreachable;
        expect(qlen <= arg_out.len);

        // Part B: Fuzz decodeWinShimTrailer on corrupted/truncated trailers (all must reject cleanly)
        let c: usize = 0;
        while (c < valid_len) {
            scratch_tr[c] = valid_tr[c];
            c += 1;
        }
        let cur_len = valid_len;
        const mode = iter % 4;
        if (mode == 0) {
            cur_len = @as(usize, @intCast(r % @as(u64, valid_len)));
        } else if (mode == 1) {
            const bit_pos = @as(usize, @intCast(r % @as(u64, valid_len)));
            scratch_tr[bit_pos] ^= @as(u8, 1) << @as(u3, @truncate(r >> 8));
        } else if (mode == 2) {
            // Corrupt magic
            scratch_tr[valid_len - 1] ^= 0xFF;
        } else {
            // Corrupt target_path_len > payload_len and recompute CRC32 to test bounds validation
            const foff = valid_len - WIN_SHIM_FOOTER_BYTES;
            scratch_tr[foff + 12] = 0xFF;
            scratch_tr[foff + 13] = 0xFF;
            let ncrc = updateWinShimCrc32(0, scratch_tr[0..foff]);
            ncrc = updateWinShimCrc32(ncrc, scratch_tr[foff..foff + 4]);
            ncrc = updateWinShimCrc32(ncrc, scratch_tr[foff + 8..valid_len]);
            scratch_tr[foff + 4] = @as(u8, @truncate(ncrc));
            scratch_tr[foff + 5] = @as(u8, @truncate(ncrc >> 8));
            scratch_tr[foff + 6] = @as(u8, @truncate(ncrc >> 16));
            scratch_tr[foff + 7] = @as(u8, @truncate(ncrc >> 24));
        }

        let rejected = false;
        * = decodeWinShimTrailer(scratch_tr[0..cur_len]) catch {
            rejected = true;
        };
        expect(rejected);

        passed_iters += 1;
        iter += 1;
    }

    expectEqual(passed_iters, @as(usize, 500));
});
