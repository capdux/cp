// Source: Absorbed from /data/dev/bun/src/install/windows-shim/bun_shim_impl.cp | Synced: 2026-11-22T11:02:40Z
// ============================================================================
// lib/pkg/winShimCodec.cp — Windows .exe Shim Trailer, Shebang & Argv Quoting Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven cross-platform binary shim engine:
//   1. Windows `.exe` trailer metadata codec (`encodeWinShimTrailer` /
//      `decodeWinShimTrailer`) supporting both UTF-8 and UTF-16LE payloads,
//      8-byte magic `"CPSHIM01"`, IEEE 802.3 CRC32 verification, and O(1)
//      zero-copy tail lookup from arbitrary `.exe` byte buffers;
//   2. Exact Microsoft CRT `CommandLineToArgvW` argument quoter and command-line
//      builder (`quoteWindowsArg` / `buildWindowsCommandLine`) with `cap:simd`
//      fast-path scanning, backslash doubling rules, 32,767-char limit guard,
//      and `cmd.exe` metacharacter detection (`hasCmdExeMetachars`);
//   3. `cap:simd`-accelerated Shebang parser & Windows runtime router
//      (`parseScriptShebang`) handling `#!/usr/bin/env -S cap run`,
//      `#!/usr/bin/env node`, `#!/bin/sh`, and custom interpreters.

import { Simd } from "cap:simd";

/// Eight-byte magic suffix identifying a Cap Windows shim trailer ("CPSHIM01").
pub const WIN_SHIM_TRAILER_MAGIC: [8]u8 = ['C', 'P', 'S', 'H', 'I', 'M', '0', '1'];

/// Current binary format version for Windows shim trailers.
pub const WIN_SHIM_TRAILER_VERSION: u16 = 1;

/// Fixed byte length of the footer placed at the very end of the `.exe` shim.
pub const WIN_SHIM_FOOTER_BYTES: usize = 32;

/// Maximum command-line length permitted by Windows `CreateProcessW`.
pub const WIN_SHIM_MAX_CMDLINE_CHARS: usize = 32767;

/// Trailer flag: string fields are encoded as UTF-16LE instead of UTF-8.
pub const WIN_SHIM_FLAG_USE_UTF16: u16 = 0x0001;

/// Trailer flag: child process inherits caller's working directory.
pub const WIN_SHIM_FLAG_INHERIT_CWD: u16 = 0x0002;

/// Trailer flag: target script contained a Shebang header.
pub const WIN_SHIM_FLAG_HAS_SHEBANG: u16 = 0x0004;

/// Trailer flag: target binary uses Windows GUI subsystem.
pub const WIN_SHIM_FLAG_GUI_SUBSYSTEM: u16 = 0x0008;

/// Error set for Windows shim trailer encoding, decoding, quoting, and Shebang parsing.
pub err WinShimCodecError {
    BufferTooSmall,
    TruncatedTrailer,
    InvalidMagic,
    UnsupportedVersion,
    ChecksumMismatch,
    CorruptBounds,
    InvalidUtf8,
    InvalidUtf16,
    CommandLineTooLong,
    InvalidShebang,
}

// ============================================================================
// Internal Zero-Alloc Endian, CRC32, and Formatting Helpers
// ============================================================================

const WS_CRC32_TABLE: [16]u32 = [
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
];

fn wsWriteU16Le(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @truncate(v));
    out[off + 1] = @as(u8, @truncate(v >> 8));
}

fn wsWriteU32Le(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @truncate(v));
    out[off + 1] = @as(u8, @truncate(v >> 8));
    out[off + 2] = @as(u8, @truncate(v >> 16));
    out[off + 3] = @as(u8, @truncate(v >> 24));
}

fn wsReadU16Le(data: []const u8, off: usize): u16 {
    return @as(u16, data[off]) | (@as(u16, data[off + 1]) << 8);
}

fn wsReadU32Le(data: []const u8, off: usize): u32 {
    return @as(u32, data[off]) |
        (@as(u32, data[off + 1]) << 8) |
        (@as(u32, data[off + 2]) << 16) |
        (@as(u32, data[off + 3]) << 24);
}

fn wsBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn wsAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn wsAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn wsAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        wsAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        wsAppendByte(out, pos, tmp[n])?;
    }
}

// ============================================================================
// Section 1: IEEE 802.3 CRC32 & Zero-Heap UTF-8 <-> UTF-16LE Conversion
// ============================================================================

/// Incrementally updates an IEEE 802.3 CRC32 checksum.
pub fn updateWinShimCrc32(initial_crc: u32, data: []const u8): u32 {
    let crc: u32 = initial_crc ^ 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        const b: u32 = @as(u32, data[i]);
        crc = (crc >> 4) ^ WS_CRC32_TABLE[@as(usize, (crc ^ b) & 0xF)];
        crc = (crc >> 4) ^ WS_CRC32_TABLE[@as(usize, (crc ^ (b >> 4)) & 0xF)];
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes the IEEE 802.3 CRC32 checksum of `data`.
pub fn computeWinShimCrc32(data: []const u8): u32 {
    return updateWinShimCrc32(0, data);
}

/// Encodes valid UTF-8 bytes into UTF-16LE bytes (supporting full Unicode surrogate pairs).
pub fn encodeUtf8ToUtf16LeBytes(utf8: []const u8, out: []u8): WinShimCodecError!usize {
    let i: usize = 0;
    let pos: usize = 0;
    while (i < utf8.len) {
        const b0 = utf8[i];
        let cp: u32 = 0;
        if (b0 < 0x80) {
            cp = @as(u32, b0);
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0) {
            if (i + 1 >= utf8.len) return err.InvalidUtf8;
            const b1 = utf8[i + 1];
            if ((b1 & 0xC0) != 0x80) return err.InvalidUtf8;
            cp = (@as(u32, b0 & 0x1F) << 6) | @as(u32, b1 & 0x3F);
            if (cp < 0x80) return err.InvalidUtf8;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0) {
            if (i + 2 >= utf8.len) return err.InvalidUtf8;
            const b1 = utf8[i + 1];
            const b2 = utf8[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return err.InvalidUtf8;
            cp = (@as(u32, b0 & 0x0F) << 12) | (@as(u32, b1 & 0x3F) << 6) | @as(u32, b2 & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) return err.InvalidUtf8;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0) {
            if (i + 3 >= utf8.len) return err.InvalidUtf8;
            const b1 = utf8[i + 1];
            const b2 = utf8[i + 2];
            const b3 = utf8[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return err.InvalidUtf8;
            cp = (@as(u32, b0 & 0x07) << 18) | (@as(u32, b1 & 0x3F) << 12) |
                (@as(u32, b2 & 0x3F) << 6) | @as(u32, b3 & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) return err.InvalidUtf8;
            i += 4;
        } else {
            return err.InvalidUtf8;
        }

        if (cp <= 0xFFFF) {
            if (2 > out.len - pos) return err.BufferTooSmall;
            wsWriteU16Le(out, pos, @as(u16, @intCast(cp)));
            pos += 2;
        } else {
            if (4 > out.len - pos) return err.BufferTooSmall;
            const adj = cp - 0x10000;
            const hi: u16 = @as(u16, 0xD800) + @as(u16, @intCast(adj >> 10));
            const lo: u16 = @as(u16, 0xDC00) + @as(u16, @intCast(adj & 0x3FF));
            wsWriteU16Le(out, pos, hi);
            wsWriteU16Le(out, pos + 2, lo);
            pos += 4;
        }
    }
    return pos;
}

/// Decodes UTF-16LE bytes into UTF-8 bytes, rejecting odd lengths and lone surrogates.
pub fn decodeUtf16LeBytesToUtf8(utf16_le: []const u8, out: []u8): WinShimCodecError!usize {
    if ((utf16_le.len & 1) != 0) return err.InvalidUtf16;
    let i: usize = 0;
    let pos: usize = 0;
    while (i < utf16_le.len) {
        const cu0 = wsReadU16Le(utf16_le, i);
        i += 2;
        let cp: u32 = @as(u32, cu0);
        if (cu0 >= 0xD800 && cu0 <= 0xDBFF) {
            if (i + 2 > utf16_le.len) return err.InvalidUtf16;
            const cu1 = wsReadU16Le(utf16_le, i);
            if (cu1 < 0xDC00 || cu1 > 0xDFFF) return err.InvalidUtf16;
            i += 2;
            cp = @as(u32, 0x10000) + ((@as(u32, cu0 - 0xD800) << 10) | @as(u32, cu1 - 0xDC00));
        } else if (cu0 >= 0xDC00 && cu0 <= 0xDFFF) {
            return err.InvalidUtf16;
        }

        if (cp < 0x80) {
            wsAppendByte(out, &pos, @as(u8, @intCast(cp)))?;
        } else if (cp < 0x800) {
            wsAppendByte(out, &pos, @as(u8, @intCast(0xC0 | (cp >> 6))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | (cp & 0x3F))))?;
        } else if (cp < 0x10000) {
            wsAppendByte(out, &pos, @as(u8, @intCast(0xE0 | (cp >> 12))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | (cp & 0x3F))))?;
        } else {
            wsAppendByte(out, &pos, @as(u8, @intCast(0xF0 | (cp >> 18))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | ((cp >> 12) & 0x3F))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F))))?;
            wsAppendByte(out, &pos, @as(u8, @intCast(0x80 | (cp & 0x3F))))?;
        }
    }
    return pos;
}

// ============================================================================
// Section 2: Windows `.exe` Shim Trailer Metadata Codec
// ============================================================================

/// Metadata payload serialized into the tail of a Windows `.exe` launcher shim.
pub struct WinShimTrailerSpec {
    target_path: []const u8 = "",
    interpreter_path: []const u8 = "",
    interpreter_args: []const u8 = "",
    flags: u16 = WIN_SHIM_FLAG_INHERIT_CWD,
}

/// Zero-copy view of a validated Windows `.exe` shim trailer.
pub struct WinShimTrailerView {
    version: u16 = 0,
    flags: u16 = 0,
    crc32: u32 = 0,
    trailer_offset: usize = 0,
    trailer_total_bytes: usize = 0,
    target_path_raw: []const u8 = "",
    interpreter_path_raw: []const u8 = "",
    interpreter_args_raw: []const u8 = "",

    fn isUtf16(): bool {
        return (_.flags & WIN_SHIM_FLAG_USE_UTF16) != 0;
    }

    fn readFieldUtf8(raw: []const u8, out: []u8): WinShimCodecError!usize {
        if (_.isUtf16()) {
            return decodeUtf16LeBytesToUtf8(raw, out);
        }
        let pos: usize = 0;
        wsAppendBytes(out, &pos, raw)?;
        return pos;
    }

    fn readTargetUtf8(out: []u8): WinShimCodecError!usize {
        return _.readFieldUtf8(_.target_path_raw, out);
    }

    fn readInterpreterUtf8(out: []u8): WinShimCodecError!usize {
        return _.readFieldUtf8(_.interpreter_path_raw, out);
    }

    fn readArgsUtf8(out: []u8): WinShimCodecError!usize {
        return _.readFieldUtf8(_.interpreter_args_raw, out);
    }
}

/// Encodes `spec` into a self-describing, CRC32-verified `.exe` trailer block.
/// When `WIN_SHIM_FLAG_USE_UTF16` is set in `spec.flags`, string fields are
/// encoded as UTF-16LE for direct Windows `CreateProcessW` consumption.
pub fn encodeWinShimTrailer(spec: WinShimTrailerSpec, out: []u8): WinShimCodecError!usize {
    const use_utf16 = (spec.flags & WIN_SHIM_FLAG_USE_UTF16) != 0;
    let pos: usize = 0;

    const target_start = pos;
    if (use_utf16) {
        pos += encodeUtf8ToUtf16LeBytes(spec.target_path, out[pos..])?;
    } else {
        wsAppendBytes(out, &pos, spec.target_path)?;
    }
    const target_len = pos - target_start;

    const interp_start = pos;
    if (use_utf16) {
        pos += encodeUtf8ToUtf16LeBytes(spec.interpreter_path, out[pos..])?;
    } else {
        wsAppendBytes(out, &pos, spec.interpreter_path)?;
    }
    const interp_len = pos - interp_start;

    const args_start = pos;
    if (use_utf16) {
        pos += encodeUtf8ToUtf16LeBytes(spec.interpreter_args, out[pos..])?;
    } else {
        wsAppendBytes(out, &pos, spec.interpreter_args)?;
    }
    const args_len = pos - args_start;

    const payload_len = pos;
    if (WIN_SHIM_FOOTER_BYTES > out.len - pos) return err.BufferTooSmall;
    if (payload_len > 0xFFFFFFFF) return err.CorruptBounds;

    const foff = pos;
    wsWriteU32Le(out, foff + 0, @as(u32, @intCast(payload_len)));
    wsWriteU32Le(out, foff + 4, 0); // crc32 placeholder
    wsWriteU16Le(out, foff + 8, WIN_SHIM_TRAILER_VERSION);
    wsWriteU16Le(out, foff + 10, spec.flags);
    wsWriteU32Le(out, foff + 12, @as(u32, @intCast(target_len)));
    wsWriteU32Le(out, foff + 16, @as(u32, @intCast(interp_len)));
    wsWriteU32Le(out, foff + 20, @as(u32, @intCast(args_len)));
    let m: usize = 0;
    while (m < 8) {
        out[foff + 24 + m] = WIN_SHIM_TRAILER_MAGIC[m];
        m += 1;
    }

    // CRC32 over payload[0..payload_len] + footer[0..4] + footer[8..32]
    let crc = updateWinShimCrc32(0, out[0..payload_len]);
    crc = updateWinShimCrc32(crc, out[foff..foff + 4]);
    crc = updateWinShimCrc32(crc, out[foff + 8..foff + WIN_SHIM_FOOTER_BYTES]);
    wsWriteU32Le(out, foff + 4, crc);

    return payload_len + WIN_SHIM_FOOTER_BYTES;
}

/// Locates, verifies, and zero-copy decodes the shim trailer from the end of an
/// arbitrary `.exe` byte buffer (or standalone trailer buffer).
pub fn decodeWinShimTrailer(exe_bytes: []const u8): WinShimCodecError!WinShimTrailerView {
    if (exe_bytes.len < WIN_SHIM_FOOTER_BYTES) {
        return err.TruncatedTrailer;
    }
    const foff = exe_bytes.len - WIN_SHIM_FOOTER_BYTES;
    if (!wsBytesEql(exe_bytes[foff + 24..foff + 32], WIN_SHIM_TRAILER_MAGIC[0..])) {
        return err.InvalidMagic;
    }

    const version = wsReadU16Le(exe_bytes, foff + 8);
    if (version != WIN_SHIM_TRAILER_VERSION) {
        return err.UnsupportedVersion;
    }

    const payload_len = @as(usize, wsReadU32Le(exe_bytes, foff + 0));
    const stored_crc = wsReadU32Le(exe_bytes, foff + 4);
    const flags = wsReadU16Le(exe_bytes, foff + 10);
    const target_len = @as(usize, wsReadU32Le(exe_bytes, foff + 12));
    const interp_len = @as(usize, wsReadU32Le(exe_bytes, foff + 16));
    const args_len = @as(usize, wsReadU32Le(exe_bytes, foff + 20));

    if (payload_len > foff) {
        return err.TruncatedTrailer;
    }
    if (target_len > payload_len || interp_len > payload_len - target_len || args_len != payload_len - target_len - interp_len) {
        return err.CorruptBounds;
    }

    const pstart = foff - payload_len;
    let computed_crc = updateWinShimCrc32(0, exe_bytes[pstart..foff]);
    computed_crc = updateWinShimCrc32(computed_crc, exe_bytes[foff..foff + 4]);
    computed_crc = updateWinShimCrc32(computed_crc, exe_bytes[foff + 8..foff + WIN_SHIM_FOOTER_BYTES]);
    if (computed_crc != stored_crc) {
        return err.ChecksumMismatch;
    }

    if ((flags & WIN_SHIM_FLAG_USE_UTF16) != 0) {
        if ((target_len & 1) != 0 || (interp_len & 1) != 0 || (args_len & 1) != 0) {
            return err.InvalidUtf16;
        }
    }

    const t_end = pstart + target_len;
    const i_end = t_end + interp_len;
    const a_end = i_end + args_len;

    return WinShimTrailerView{
        version: version,
        flags: flags,
        crc32: computed_crc,
        trailer_offset: pstart,
        trailer_total_bytes: payload_len + WIN_SHIM_FOOTER_BYTES,
        target_path_raw: exe_bytes[pstart..t_end],
        interpreter_path_raw: exe_bytes[t_end..i_end],
        interpreter_args_raw: exe_bytes[i_end..a_end],
    };
}

// ============================================================================
// Section 3: Windows `CommandLineToArgvW` Quoting & Command-Line Builder
// ============================================================================

/// Detects shell metacharacters that could trigger `cmd.exe` command injection
/// (`&`, `|`, `<`, `>`, `^`, `%`, `\n`, `\r`) via `cap:simd` (`Simd.indexOfAny4`).
pub fn hasCmdExeMetachars(s: []const u8): bool {
    if (Simd.indexOfAny4(s, '&', '|', '<', '>') != null) return true;
    if (Simd.indexOfAny4(s, '^', '%', '\n', '\r') != null) return true;
    return false;
}

fn wsNeedsWindowsQuotes(arg: []const u8): bool {
    if (arg.len == 0) return true;
    if (Simd.indexOfAny4(arg, ' ', '\t', '"', '\n') != null) return true;
    if (Simd.indexOfAny2(arg, '\r', 0x0B) != null) return true;
    return false;
}

/// Quotes a single argument according to Microsoft CRT `CommandLineToArgvW` rules:
///   - Unchanged when non-empty and free of whitespace / double-quotes (SIMD fast-path);
///   - Otherwise wrapped in `"..."`, doubling backslashes immediately preceding `"`
///     or the closing quote.
pub fn quoteWindowsArg(arg: []const u8, out: []u8): WinShimCodecError!usize {
    let pos: usize = 0;
    if (!wsNeedsWindowsQuotes(arg)) {
        wsAppendBytes(out, &pos, arg)?;
        return pos;
    }

    wsAppendByte(out, &pos, '"')?;
    let i: usize = 0;
    while (i < arg.len) {
        let backslashes: usize = 0;
        while (i < arg.len && arg[i] == '\\') {
            backslashes += 1;
            i += 1;
        }

        if (i == arg.len) {
            // Trailing backslashes before closing quote must be doubled
            let b: usize = 0;
            while (b < backslashes * 2) {
                wsAppendByte(out, &pos, '\\')?;
                b += 1;
            }
            break;
        } else if (arg[i] == '"') {
            // Backslashes before embedded quote must be doubled + 1 escape backslash
            let b: usize = 0;
            while (b < backslashes * 2 + 1) {
                wsAppendByte(out, &pos, '\\')?;
                b += 1;
            }
            wsAppendByte(out, &pos, '"')?;
            i += 1;
        } else {
            // Normal backslashes are emitted verbatim
            let b: usize = 0;
            while (b < backslashes) {
                wsAppendByte(out, &pos, '\\')?;
                b += 1;
            }
            wsAppendByte(out, &pos, arg[i])?;
            i += 1;
        }
    }
    wsAppendByte(out, &pos, '"')?;
    return pos;
}

/// Builds a complete Windows `CreateProcessW` command line from `executable` and `args`,
/// enforcing the 32,767-character Windows limit.
pub fn buildWindowsCommandLine(
    executable: []const u8,
    args: []const []const u8,
    out: []u8,
): WinShimCodecError!usize {
    let pos: usize = 0;
    pos += quoteWindowsArg(executable, out[pos..])?;

    let i: usize = 0;
    while (i < args.len) {
        wsAppendByte(out, &pos, ' ')?;
        pos += quoteWindowsArg(args[i], out[pos..])?;
        if (pos > WIN_SHIM_MAX_CMDLINE_CHARS) return err.CommandLineTooLong;
        i += 1;
    }

    if (pos > WIN_SHIM_MAX_CMDLINE_CHARS) return err.CommandLineTooLong;
    return pos;
}

// ============================================================================
// Section 4: SIMD Shebang Parser & Windows Runtime Router
// ============================================================================

/// Known script runtime families routed by `parseScriptShebang`.
pub enum WinShimRuntimeKind {
    DirectExecutable,
    Cap,
    Bun,
    Node,
    Shell,
    Python,
    Custom,
}

/// Parsed Shebang header mapped to a Windows launcher plan.
pub struct ParsedScriptShebang {
    has_shebang: bool = false,
    used_env_split: bool = false,
    runtime: WinShimRuntimeKind = WinShimRuntimeKind.DirectExecutable,
    interpreter: []const u8 = "",
    windows_exe: []const u8 = "",
    extra_args: []const u8 = "",
}

fn wsBasename(path: []const u8): []const u8 {
    let i = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') return path[i + 1..];
    }
    return path;
}

fn wsNextToken(s: []const u8, pos: *usize): []const u8 {
    const ws = Simd.skipWhitespace(s[pos.*..]);
    pos.* += ws;
    const start = pos.*;
    while (pos.* < s.len && s[pos.*] != ' ' && s[pos.*] != '\t') {
        pos.* += 1;
    }
    return s[start..pos.*];
}

fn wsClassifyRuntime(interp_name: []const u8): WinShimRuntimeKind {
    if (wsBytesEql(interp_name, "cap") || wsBytesEql(interp_name, "cap.exe")) return WinShimRuntimeKind.Cap;
    if (wsBytesEql(interp_name, "bun") || wsBytesEql(interp_name, "bun.exe")) return WinShimRuntimeKind.Bun;
    if (wsBytesEql(interp_name, "node") || wsBytesEql(interp_name, "nodejs") || wsBytesEql(interp_name, "node.exe")) return WinShimRuntimeKind.Node;
    if (wsBytesEql(interp_name, "sh") || wsBytesEql(interp_name, "bash") || wsBytesEql(interp_name, "ash")) return WinShimRuntimeKind.Shell;
    if (wsBytesEql(interp_name, "python") || wsBytesEql(interp_name, "python3") || wsBytesEql(interp_name, "python.exe")) return WinShimRuntimeKind.Python;
    return WinShimRuntimeKind.Custom;
}

fn wsWindowsExeForRuntime(kind: WinShimRuntimeKind, raw_interp: []const u8): []const u8 {
    return match (kind) {
        WinShimRuntimeKind.Cap => "cap.exe",
        WinShimRuntimeKind.Bun => "bun.exe",
        WinShimRuntimeKind.Node => "node.exe",
        WinShimRuntimeKind.Shell => "bash.exe",
        WinShimRuntimeKind.Python => "python.exe",
        WinShimRuntimeKind.DirectExecutable => "",
        WinShimRuntimeKind.Custom => raw_interp,
    };
}

/// Parses a `#!...` script Shebang header using `cap:simd` (`Simd.indexOfByte`,
/// `Simd.skipWhitespace`) and maps POSIX `/usr/bin/env [-S] <runtime> [args]`
/// to a concrete Windows executable launch plan.
pub fn parseScriptShebang(script_bytes: []const u8): WinShimCodecError!ParsedScriptShebang {
    if (script_bytes.len < 2 || script_bytes[0] != '#' || script_bytes[1] != '!') {
        return ParsedScriptShebang{};
    }

    const max_scan = if (script_bytes.len < 512) script_bytes.len else 512;
    let line_end = max_scan;
    if (nl := Simd.indexOfByte(script_bytes[0..max_scan], '\n')) {
        line_end = nl;
    }
    if (line_end > 2 && script_bytes[line_end - 1] == '\r') {
        line_end -= 1;
    }

    const line = script_bytes[2..line_end];
    let pos: usize = 0;
    const tok0 = wsNextToken(line, &pos);
    if (tok0.len == 0) return err.InvalidShebang;

    const base0 = wsBasename(tok0);
    let interp = base0;
    let used_env_split = false;

    if (wsBytesEql(base0, "env")) {
        let next_tok = wsNextToken(line, &pos);
        if (next_tok.len == 0) return err.InvalidShebang;
        if (wsBytesEql(next_tok, "-S") || wsBytesEql(next_tok, "--split-string")) {
            used_env_split = true;
            next_tok = wsNextToken(line, &pos);
            if (next_tok.len == 0) return err.InvalidShebang;
        }
        interp = wsBasename(next_tok);
    }

    // Remaining trimmed slice becomes extra_args
    const rem_ws = Simd.skipWhitespace(line[pos..]);
    pos += rem_ws;
    let extra_end = line.len;
    while (extra_end > pos && (line[extra_end - 1] == ' ' || line[extra_end - 1] == '\t')) {
        extra_end -= 1;
    }

    const rt = wsClassifyRuntime(interp);
    return ParsedScriptShebang{
        has_shebang: true,
        used_env_split: used_env_split,
        runtime: rt,
        interpreter: interp,
        windows_exe: wsWindowsExeForRuntime(rt, interp),
        extra_args: line[pos..extra_end],
    };
}

// ============================================================================
// Section 5: Diagnostic Summary Report Formatter
// ============================================================================

/// Formats a concise diagnostic summary for a decoded Windows shim trailer.
pub fn formatWinShimReport(view: WinShimTrailerView, out: []u8): WinShimCodecError!usize {
    let pos: usize = 0;
    wsAppendBytes(out, &pos, "WinShimTrailer(v=")?;
    wsAppendU64(out, &pos, @as(u64, view.version))?;
    wsAppendBytes(out, &pos, ", enc=")?;
    wsAppendBytes(out, &pos, if (view.isUtf16()) "utf16le" else "utf8")?;
    wsAppendBytes(out, &pos, ", offset=")?;
    wsAppendU64(out, &pos, @as(u64, view.trailer_offset))?;
    wsAppendBytes(out, &pos, ", bytes=")?;
    wsAppendU64(out, &pos, @as(u64, view.trailer_total_bytes))?;
    wsAppendBytes(out, &pos, ", target_len=")?;
    wsAppendU64(out, &pos, @as(u64, view.target_path_raw.len))?;
    wsAppendByte(out, &pos, ')')?;
    return pos;
}
