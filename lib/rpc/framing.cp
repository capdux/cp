// Source: cp/lib/rpc/framing.cp (LSP & MCP Content-Length / NDJSON Incremental Framers)
// Pure CP incremental streaming framers for Content-Length (LSP/DAP/MCP HTTP-style)
// and newline-delimited JSON-RPC (MCP stdio NDJSON) with DoS frame size protection.

const std = @import("std");

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
        return ContentLengthFramer{
            allocator: alloc,
            buf: .empty,
            max_frame_bytes: max_frame_bytes,
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
            const crlf_opt = std.mem.indexOf(u8, rem, "\r\n");
            let line: []const u8 = "";
            if (crlf_pos := crlf_opt) {
                line = rem[0..crlf_pos];
                pos += crlf_pos + 2;
            } else {
                line = rem;
                pos = header_bytes.len + 1;
            }

            if (line.len == 0) continue;

            const colon_pos = std.mem.indexOfScalar(u8, line, ':') ?? return err.InvalidHeader;
            const name = std.mem.trim(u8, line[0..colon_pos], " \t");
            const val = std.mem.trim(u8, line[colon_pos + 1 ..], " \t");
            if (name.len == 0) return err.InvalidHeader;

            if (std.ascii.eqlIgnoreCase(name, "content-length")) {
                const parsed = ContentLengthFramer.parseUnsignedDecimal(val) ?? return err.InvalidContentLength;
                if (prev := found_len) {
                    if (prev != parsed) return err.InvalidHeader;
                }
                found_len = parsed;
            }
        }
        return found_len ?? return err.MissingContentLength;
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
                const sep_opt = std.mem.indexOf(u8, rem, "\r\n\r\n");
                if (sep_pos := sep_opt) {
                    if (sep_pos > _.max_header_bytes) {
                        _.reset();
                        return FramingError.HeaderTooLarge;
                    }
                    const header_bytes = rem[0..sep_pos];
                    const content_len = ContentLengthFramer.parseHeaderBlock(header_bytes) catch e => {
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
        let len_buf: [32]u8 = undefined;
        let len_str: []const u8 = "";
        zig {
            len_str = std.fmt.bufPrint(&len_buf, "{d}", .{payload.len}) catch unreachable;
        }
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
        return LineRpcFramer{
            allocator: alloc,
            buf: .empty,
            max_frame_bytes: max_frame_bytes,
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
            const nl_opt = std.mem.indexOfScalar(u8, rem, '\n');
            if (nl_pos := nl_opt) {
                let raw_line = rem[0..nl_pos];
                read_pos += nl_pos + 1;
                if (raw_line.len > 0 && raw_line[raw_line.len - 1] == '\r') {
                    raw_line = raw_line[0 .. raw_line.len - 1];
                }
                const trimmed = std.mem.trim(u8, raw_line, " \t\r");
                if (trimmed.len == 0) continue;
                if (trimmed.len > _.max_frame_bytes) {
                    _.reset();
                    return FramingError.FrameTooLarge;
                }
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
        for (c of payload) {
            if (c == '\n' || c == '\r') {
                return FramingError.InvalidLine;
            }
        }
        let out = alloc.alloc(u8, payload.len + 1)?;
        if (payload.len > 0) {
            std.mem.copyForwards(u8, out[0..payload.len], payload);
        }
        out[payload.len] = '\n';
        return out;
    }
}
