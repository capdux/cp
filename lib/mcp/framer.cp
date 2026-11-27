// Source: cp/lib/mcp/framer.cp (Incremental Stdio NDJSON and Content-Length stream framer)
const std = @import("std");

pub enum FrameMode {
    Ndjson,
    ContentLength,
}

pub struct McpStreamFramer {
    alloc: std.mem.Allocator,
    mode: FrameMode,
    max_frame_bytes: usize,
    buf: std.ArrayList(u8),

    pub static fn init(alloc: std.mem.Allocator, mode: FrameMode, max_frame_bytes: usize): McpStreamFramer {
        return McpStreamFramer{
            .alloc = alloc,
            .mode = mode,
            .max_frame_bytes = max_frame_bytes,
            .buf = std.ArrayList(u8).empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub static fn freeFrames(alloc: std.mem.Allocator, frames: [][]u8) {
        for (f of frames) {
            alloc.free(f);
        }
        alloc.free(frames);
    }

    pub static fn encodeFrame(alloc: std.mem.Allocator, mode: FrameMode, payload: []const u8): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        switch (mode) {
            FrameMode.Ndjson => {
                try out.appendSlice(alloc, payload);
                try out.append(alloc, '\n');
            },
            FrameMode.ContentLength => {
                const header = try std.fmt.allocPrint(alloc, "Content-Length: {d}\r\n\r\n", .{payload.len});
                defer alloc.free(header);
                try out.appendSlice(alloc, header);
                try out.appendSlice(alloc, payload);
            },
        }
        return try out.toOwnedSlice(alloc);
    }

    fn shiftBuffer(*_, consumed: usize) {
        if (consumed == 0) {
            return;
        }
        const remaining = _.buf.items.len - consumed;
        if (remaining > 0) {
            std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[consumed..]);
        }
        _.buf.items.len = remaining;
    }

    static fn asciiLowerEql(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) {
            return false;
        }
        let i: usize = 0;
        while (i < a.len) {
            const ca = if (a[i] >= 'A' and a[i] <= 'Z') a[i] + 32 else a[i];
            const cb = if (b[i] >= 'A' and b[i] <= 'Z') b[i] + 32 else b[i];
            if (ca != cb) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    static fn parseContentLengthHeader(headers: []const u8): !usize {
        let i: usize = 0;
        let found_len: usize? = null;
        while (i < headers.len) {
            let line_end = i;
            while (line_end < headers.len and headers[line_end] != '\n') {
                line_end += 1;
            }
            let line = headers[i..line_end];
            if (line.len > 0 and line[line.len - 1] == '\r') {
                line = line[0 .. line.len - 1];
            }
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const key = std.mem.trim(u8, line[0..colon], " \t");
                const val = std.mem.trim(u8, line[colon + 1 ..], " \t");
                if (asciiLowerEql(key, "content-length")) {
                    found_len = std.fmt.parseInt(usize, val, 10) catch return error.InvalidHeader;
                }
            }
            i = if (line_end < headers.len) line_end + 1 else line_end;
        }
        return found_len orelse return error.MissingContentLength;
    }

    pub fn feed(*_, chunk: []const u8): ![][]u8 {
        if (chunk.len > 0) {
            try _.buf.appendSlice(_.alloc, chunk);
        }

        let frames = std.ArrayList([]u8).empty;
        errdefer {
            for (f of frames.items) {
                _.alloc.free(f);
            }
            frames.deinit(_.alloc);
        }

        switch (_.mode) {
            FrameMode.Ndjson => {
                while (true) {
                    if (nl_idx := std.mem.indexOfScalar(u8, _.buf.items, '\n')) {
                        let line = _.buf.items[0..nl_idx];
                        if (line.len > 0 and line[line.len - 1] == '\r') {
                            line = line[0 .. line.len - 1];
                        }
                        if (line.len > _.max_frame_bytes) {
                            return error.FrameTooLarge;
                        }
                        const trimmed = std.mem.trim(u8, line, " \t\r");
                        if (trimmed.len > 0) {
                            const copy = try _.alloc.dupe(u8, trimmed);
                            try frames.append(_.alloc, copy);
                        }
                        _.shiftBuffer(nl_idx + 1);
                    } else {
                        if (_.buf.items.len > _.max_frame_bytes) {
                            return error.FrameTooLarge;
                        }
                        break;
                    }
                }
            },
            FrameMode.ContentLength => {
                while (true) {
                    const data = _.buf.items;
                    let header_end: usize? = null;
                    let sep_len: usize = 0;

                    if (idx := std.mem.indexOf(u8, data, "\r\n\r\n")) {
                        header_end = idx;
                        sep_len = 4;
                    } else if (idx := std.mem.indexOf(u8, data, "\n\n")) {
                        header_end = idx;
                        sep_len = 2;
                    }

                    if (h_end := header_end) {
                        const payload_start = h_end + sep_len;
                        const content_len = try parseContentLengthHeader(data[0..h_end]);
                        if (content_len > _.max_frame_bytes) {
                            return error.FrameTooLarge;
                        }
                        if (data.len < payload_start + content_len) {
                            break;
                        }
                        const copy = try _.alloc.dupe(u8, data[payload_start .. payload_start + content_len]);
                        try frames.append(_.alloc, copy);
                        _.shiftBuffer(payload_start + content_len);
                    } else {
                        if (data.len > _.max_frame_bytes) {
                            return error.FrameTooLarge;
                        }
                        break;
                    }
                }
            },
        }

        return try frames.toOwnedSlice(_.alloc);
    }
}
