// Reference-hardened against paritytech/jsonrpsee (MIT) & JSON-RPC 2.0 / MCP
// Source: cp/lib/rpc/framing.cp (LSP & MCP Content-Length / NDJSON Incremental Framers + 16-Lane SIMD)
// Pure CP incremental streaming framers for Content-Length (LSP/DAP/MCP HTTP-style)
// and newline-delimited JSON-RPC (MCP stdio NDJSON) with 16 MiB resource-exhaustion protection.

const std = @import("std");

pub type RpcVec16u8 = @Vector(16, u8);

pub const DEFAULT_MAX_FRAME_BYTES: usize = 16 * 1024 * 1024; // 16 MiB
pub const DEFAULT_MAX_HEADER_BYTES: usize = 16 * 1024;       // 16 KiB

pub err FramingError {
    InvalidHeader,
    MissingContentLength,
    InvalidContentLength,
    FrameTooLarge,
    HeaderTooLarge,
    InvalidLine,
    OutOfMemory,
}

fn rpcLoadVec16(data: []const u8, offset: usize): RpcVec16u8 {
    let arr: [16]u8 = [0; 16];
    let j: usize = 0;
    while (j < 16) : (j += 1) {
        arr[j] = data[offset + j];
    }
    return arr;
}

fn rpcAsciiLowerByte(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

/// 16-lane `@Vector(16, u8)` SIMD scan for the first `\r` (`0x0D`) or `\n` (`0x0A`) at or after `start`.
/// Hot path for `ContentLengthFramer` and `LineRpcFramer`.
pub fn simdRpcFindCrlf(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let pos: usize = start;
    const v_cr: RpcVec16u8 = @splat(0x0D);
    const v_lf: RpcVec16u8 = @splat(0x0A);
    const v_one: RpcVec16u8 = @splat(1);
    const v_zero: RpcVec16u8 = @splat(0);

    while (pos + 16 <= data.len) {
        const v = rpcLoadVec16(data, pos);
        const hit = @select(u8, v == v_cr, v_one, v_zero) | @select(u8, v == v_lf, v_one, v_zero);
        if (@reduce(.Max, hit) != 0) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const c = data[pos + j];
                if (c == '\r' || c == '\n') return pos + j;
            }
        }
        pos += 16;
    }
    while (pos < data.len) : (pos += 1) {
        const c = data[pos];
        if (c == '\r' || c == '\n') return pos;
    }
    return null;
}

/// Scalar reference for `simdRpcFindCrlf` (0-diff differential verification).
pub fn scalarRpcFindCrlf(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let pos: usize = start;
    while (pos < data.len) : (pos += 1) {
        const c = data[pos];
        if (c == '\r' || c == '\n') return pos;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD ASCII case-insensitive equality.
/// Hot path for matching HTTP/LSP header names such as `Content-Length`.
pub fn simdRpcAsciiLowerEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let pos: usize = 0;
    const v_upper_a: RpcVec16u8 = @splat('A');
    const v_upper_z: RpcVec16u8 = @splat('Z');
    const v_diff32: RpcVec16u8 = @splat(32);
    const v_zero: RpcVec16u8 = @splat(0);

    while (pos + 16 <= a.len) {
        const va = rpcLoadVec16(a, pos);
        const vb = rpcLoadVec16(b, pos);
        const a_is_upper = (va >= v_upper_a) & (va <= v_upper_z);
        const b_is_upper = (vb >= v_upper_a) & (vb <= v_upper_z);
        const la = va + @select(u8, a_is_upper, v_diff32, v_zero);
        const lb = vb + @select(u8, b_is_upper, v_diff32, v_zero);
        const diff: RpcVec16u8 = la ^ lb;
        if (@reduce(.Or, diff) != 0) return false;
        pos += 16;
    }
    while (pos < a.len) : (pos += 1) {
        if (rpcAsciiLowerByte(a[pos]) != rpcAsciiLowerByte(b[pos])) return false;
    }
    return true;
}

/// Scalar reference for `simdRpcAsciiLowerEql` (0-diff differential verification).
pub fn scalarRpcAsciiLowerEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let pos: usize = 0;
    while (pos < a.len) : (pos += 1) {
        if (rpcAsciiLowerByte(a[pos]) != rpcAsciiLowerByte(b[pos])) return false;
    }
    return true;
}

fn rpcFindHeaderTerminator(data: []const u8): usize? {
    let pos: usize = 0;
    while (pos + 4 <= data.len) {
        const hit = simdRpcFindCrlf(data, pos) ?? return null;
        if (hit + 3 < data.len
            && data[hit] == '\r'
            && data[hit + 1] == '\n'
            && data[hit + 2] == '\r'
            && data[hit + 3] == '\n')
        {
            return hit;
        }
        pos = hit + 1;
    }
    return null;
}

fn rpcFormatUsize(buf: *[32]u8, value: usize): []const u8 {
    if (value == 0) {
        buf[31] = '0';
        return buf[31..32];
    }
    let v = value;
    let idx: usize = 32;
    while (v > 0) {
        idx -= 1;
        buf[idx] = @as(u8, '0') + @as(u8, @intCast(v % 10));
        v /= 10;
    }
    return buf[idx..32];
}

pub struct ContentLengthFramer {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    max_frame_bytes: usize = DEFAULT_MAX_FRAME_BYTES,
    max_header_bytes: usize = DEFAULT_MAX_HEADER_BYTES,
    pending_content_length: usize? = null,

    pub static fn init(
        alloc: std.mem.Allocator,
        max_frame_bytes: usize = DEFAULT_MAX_FRAME_BYTES,
    ): ContentLengthFramer {
        const eff_max: usize = if (max_frame_bytes > DEFAULT_MAX_FRAME_BYTES) DEFAULT_MAX_FRAME_BYTES else max_frame_bytes;
        return ContentLengthFramer{
            allocator: alloc,
            buf: .empty,
            max_frame_bytes: eff_max,
            max_header_bytes: DEFAULT_MAX_HEADER_BYTES,
            pending_content_length: null,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
        _.pending_content_length = null;
    }

    pub fn reset(*_) {
        _.buf.clearRetainingCapacity();
        _.pending_content_length = null;
    }

    pub fn bufferedBytes(): usize {
        return _.buf.items.len;
    }

    pub static fn freeFrames(alloc: std.mem.Allocator, frames: [][]u8) {
        for (f of frames) {
            if (f.len > 0) alloc.free(f);
        }
        if (frames.len > 0) alloc.free(frames);
    }

    static fn parseUnsignedDecimal(raw: []const u8): usize? {
        if (raw.len == 0) return null;
        let acc: usize = 0;
        for (c of raw) {
            if (c < '0' || c > '9') return null;
            const digit = (c - '0') as usize;
            const max_div = (@as(usize, 0) -% 1) / 10;
            if (acc > max_div) return null;
            const mul = acc * 10;
            if (mul > (@as(usize, 0) -% 1) - digit) return null;
            acc = mul + digit;
        }
        return acc;
    }

    static fn parseHeaderBlock(header_bytes: []const u8): usize! {
        let found_len: usize? = null;
        let pos: usize = 0;
        while (pos <= header_bytes.len) {
            const rem = header_bytes[pos..];
            let line: []const u8 = "";
            if (crlf_pos := simdRpcFindCrlf(rem, 0)) {
                if (rem[crlf_pos] != '\r' || crlf_pos + 1 >= rem.len || rem[crlf_pos + 1] != '\n') {
                    return FramingError.InvalidHeader;
                }
                line = rem[0..crlf_pos];
                pos += crlf_pos + 2;
            } else {
                line = rem;
                pos = header_bytes.len + 1;
            }

            if (line.len == 0) continue;

            const colon_pos = std.mem.indexOfScalar(u8, line, ':') ?? return FramingError.InvalidHeader;
            const name = std.mem.trim(u8, line[0..colon_pos], " \t");
            const val = std.mem.trim(u8, line[colon_pos + 1 ..], " \t");
            if (name.len == 0) return FramingError.InvalidHeader;

            if (simdRpcAsciiLowerEql(name, "content-length")) {
                const parsed = ContentLengthFramer.parseUnsignedDecimal(val) ?? return FramingError.InvalidContentLength;
                if (prev := found_len) {
                    if (prev != parsed) return FramingError.InvalidHeader;
                }
                found_len = parsed;
            }
        }
        return found_len ?? return FramingError.MissingContentLength;
    }

    pub fn feed(*_, chunk: []const u8): ([][]u8)! {
        if (chunk.len > 0) {
            _.buf.appendSlice(_.allocator, chunk)?;
        }

        let frames: std.ArrayList([]u8) = .empty;
        errdefer {
            for (f of frames.items) {
                if (f.len > 0) _.allocator.free(f);
            }
            frames.deinit(_.allocator);
        }

        let read_pos: usize = 0;
        while (read_pos < _.buf.items.len || _.pending_content_length != null) {
            if (_.pending_content_length == null) {
                const rem = _.buf.items[read_pos..];
                const sep_opt = rpcFindHeaderTerminator(rem);
                if (sep_pos := sep_opt) {
                    if (sep_pos > _.max_header_bytes) {
                        _.reset();
                        return FramingError.HeaderTooLarge;
                    }
                    const header_bytes = rem[0..sep_pos];
                    const content_len = ContentLengthFramer.parseHeaderBlock(header_bytes) catch |e| {
                        _.reset();
                        return e;
                    };
                    if (content_len > _.max_frame_bytes) {
                        _.reset();
                        return FramingError.FrameTooLarge;
                    }
                    read_pos += sep_pos + 4;
                    _.pending_content_length = content_len;
                } else {
                    if (rem.len > _.max_header_bytes) {
                        _.reset();
                        return FramingError.HeaderTooLarge;
                    }
                    break;
                }
            }

            if (need_len := _.pending_content_length) {
                const avail = _.buf.items.len - read_pos;
                if (avail < need_len) {
                    break;
                }
                const payload = _.buf.items[read_pos .. read_pos + need_len];
                const owned = _.allocator.dupe(u8, payload)?;
                errdefer if (owned.len > 0) _.allocator.free(owned);
                frames.append(_.allocator, owned)?;
                read_pos += need_len;
                _.pending_content_length = null;
            }
        }

        if (read_pos > 0) {
            const remaining = _.buf.items.len - read_pos;
            if (remaining > 0) {
                std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[read_pos..]);
            }
            _.buf.shrinkRetainingCapacity(remaining);
        }

        return frames.toOwnedSlice(_.allocator);
    }

    pub static fn encodeFrame(alloc: std.mem.Allocator, payload: []const u8): ([]u8)! {
        let len_buf: [32]u8 = [0; 32];
        const len_str = rpcFormatUsize(&len_buf, payload.len);
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "Content-Length: ")?;
        out.appendSlice(alloc, len_str)?;
        out.appendSlice(alloc, "\r\n\r\n")?;
        out.appendSlice(alloc, payload)?;
        return out.toOwnedSlice(alloc);
    }
}

pub struct LineRpcFramer {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    max_frame_bytes: usize = DEFAULT_MAX_FRAME_BYTES,

    pub static fn init(
        alloc: std.mem.Allocator,
        max_frame_bytes: usize = DEFAULT_MAX_FRAME_BYTES,
    ): LineRpcFramer {
        const eff_max: usize = if (max_frame_bytes > DEFAULT_MAX_FRAME_BYTES) DEFAULT_MAX_FRAME_BYTES else max_frame_bytes;
        return LineRpcFramer{
            allocator: alloc,
            buf: .empty,
            max_frame_bytes: eff_max,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    pub fn reset(*_) {
        _.buf.clearRetainingCapacity();
    }

    pub fn bufferedBytes(): usize {
        return _.buf.items.len;
    }

    pub static fn freeLines(alloc: std.mem.Allocator, lines: [][]u8) {
        for (l of lines) {
            if (l.len > 0) alloc.free(l);
        }
        if (lines.len > 0) alloc.free(lines);
    }

    pub fn feed(*_, chunk: []const u8): ([][]u8)! {
        if (chunk.len > 0) {
            _.buf.appendSlice(_.allocator, chunk)?;
        }

        let lines: std.ArrayList([]u8) = .empty;
        errdefer {
            for (l of lines.items) {
                if (l.len > 0) _.allocator.free(l);
            }
            lines.deinit(_.allocator);
        }

        let read_pos: usize = 0;
        while (read_pos < _.buf.items.len) {
            const rem = _.buf.items[read_pos..];
            let nl_opt: usize? = null;
            let scan: usize = 0;
            while (scan < rem.len) {
                const hit = simdRpcFindCrlf(rem, scan) ?? break;
                if (rem[hit] == '\n') {
                    nl_opt = hit;
                    break;
                }
                scan = hit + 1;
            }
            if (nl_pos := nl_opt) {
                let raw_line = rem[0..nl_pos];
                read_pos += nl_pos + 1;
                if (raw_line.len > 0 && raw_line[raw_line.len - 1] == '\r') {
                    raw_line = raw_line[0 .. raw_line.len - 1];
                }
                if (raw_line.len > _.max_frame_bytes) {
                    _.reset();
                    return FramingError.FrameTooLarge;
                }
                const trimmed = std.mem.trim(u8, raw_line, " \t\r");
                if (trimmed.len == 0) continue;
                const owned = _.allocator.dupe(u8, trimmed)?;
                errdefer if (owned.len > 0) _.allocator.free(owned);
                lines.append(_.allocator, owned)?;
            } else {
                if (rem.len > _.max_frame_bytes) {
                    _.reset();
                    return FramingError.FrameTooLarge;
                }
                break;
            }
        }

        if (read_pos > 0) {
            const remaining = _.buf.items.len - read_pos;
            if (remaining > 0) {
                std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[read_pos..]);
            }
            _.buf.shrinkRetainingCapacity(remaining);
        }

        return lines.toOwnedSlice(_.allocator);
    }

    pub static fn encodeLine(alloc: std.mem.Allocator, payload: []const u8): ([]u8)! {
        if (simdRpcFindCrlf(payload, 0) != null) {
            return FramingError.InvalidLine;
        }
        let out = alloc.alloc(u8, payload.len + 1)?;
        if (payload.len > 0) {
            std.mem.copyForwards(u8, out[0..payload.len], payload);
        }
        out[payload.len] = '\n';
        return out;
    }
}
