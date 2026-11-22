// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T05:31:41Z
// lib/http/chunked.cp
// RFC 9112 §7.1 Chunked Transfer-Encoding Streaming Decoder & Encoder (cap:http)

const std = @import("std");

pub const MAX_CHUNK_TRAILERS: usize = 16;

pub enum ChunkedDecoderState: u8 {
    ChunkSize = 0,
    ChunkExt = 1,
    ChunkExtLf = 2,
    ChunkData = 3,
    ChunkDataCr = 4,
    ChunkDataLf = 5,
    TrailerHead = 6,
    TrailerHeadLf = 7,
    TrailerLine = 8,
    TrailerLineLf = 9,
    Done = 10,
    Error = 11,
}

pub struct ChunkTrailer {
    pub name: []const u8 = "",
    pub value: []const u8 = "",
}

pub struct ChunkTrailerSpan {
    pub name_off: usize = 0,
    pub name_len: usize = 0,
    pub val_off: usize = 0,
    pub val_len: usize = 0,
}

pub struct ChunkedDecodeResult {
    pub bytes_written: usize = 0,
    pub bytes_consumed: usize = 0,
    pub is_done: bool = false,
    pub trailer_count: usize = 0,
}

pub struct ChunkedDecoder {
    pub state: ChunkedDecoderState = ChunkedDecoderState.ChunkSize,
    pub bytes_left_in_chunk: usize = 0,
    pub hex_count: u8 = 0,
    pub total_decoded: usize = 0,
    pub consume_trailer: bool = true,
    pub trailer_spans: [MAX_CHUNK_TRAILERS]ChunkTrailerSpan = undefined,
    pub trailer_count: usize = 0,
    pub trailer_storage: [2048]u8 = undefined,
    pub trailer_storage_len: usize = 0,
    pub trailer_line_buf: [512]u8 = undefined,
    pub trailer_line_len: usize = 0,

    pub static fn init(consume_trailer: bool = true): ChunkedDecoder {
        let decoder = ChunkedDecoder{
            state: ChunkedDecoderState.ChunkSize,
            bytes_left_in_chunk: 0,
            hex_count: 0,
            total_decoded: 0,
            consume_trailer: consume_trailer,
            trailer_spans: undefined,
            trailer_count: 0,
            trailer_storage: undefined,
            trailer_storage_len: 0,
            trailer_line_buf: [0 as u8; 512],
            trailer_line_len: 0,
        };
        let i: usize = 0;
        while (i < MAX_CHUNK_TRAILERS) : (i += 1) {
            decoder.trailer_spans[i] = ChunkTrailerSpan{};
        }
        return decoder;
    }

    pub fn reset(*_) {
        _.state = ChunkedDecoderState.ChunkSize;
        _.bytes_left_in_chunk = 0;
        _.hex_count = 0;
        _.total_decoded = 0;
        _.trailer_count = 0;
        _.trailer_storage_len = 0;
        _.trailer_line_len = 0;
    }

    pub fn isDone(): bool {
        return _.state == ChunkedDecoderState.Done;
    }

    pub fn isInData(): bool {
        return _.state == ChunkedDecoderState.ChunkData;
    }

    pub fn getTrailer(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.trailer_count) : (i += 1) {
            const sp = _.trailer_spans[i];
            const t_name = _.trailer_storage[sp.name_off .. sp.name_off + sp.name_len];
            if (std.ascii.eqlIgnoreCase(t_name, name)) {
                return _.trailer_storage[sp.val_off .. sp.val_off + sp.val_len];
            }
        }
        return null;
    }

    pub fn trailerAt(idx: usize): ChunkTrailer? {
        if (idx >= _.trailer_count) return null;
        const sp = _.trailer_spans[idx];
        return ChunkTrailer{
            name: _.trailer_storage[sp.name_off .. sp.name_off + sp.name_len],
            value: _.trailer_storage[sp.val_off .. sp.val_off + sp.val_len],
        };
    }

    static fn hexDigit(ch: u8): usize? {
        if (ch >= '0' && ch <= '9') return (ch - '0') as usize;
        if (ch >= 'a' && ch <= 'f') return (ch - 'a' + 10) as usize;
        if (ch >= 'A' && ch <= 'F') return (ch - 'A' + 10) as usize;
        return null;
    }

    static fn trimWs(s: []const u8): []const u8 {
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
        return s[start..end];
    }

    fn recordTrailerFromSlice(*_, line: []const u8) {
        if (_.trailer_count >= MAX_CHUNK_TRAILERS) return;
        let colon: usize = 0;
        let found = false;
        while (colon < line.len) : (colon += 1) {
            if (line[colon] == ':') {
                found = true;
                break;
            }
        }
        if (!found || colon == 0) return;

        let n_start: usize = 0;
        let n_end: usize = colon;
        while (n_start < n_end && (line[n_start] == ' ' || line[n_start] == '\t')) : (n_start += 1) {}
        while (n_end > n_start && (line[n_end - 1] == ' ' || line[n_end - 1] == '\t')) : (n_end -= 1) {}

        let v_start: usize = colon + 1;
        let v_end: usize = line.len;
        while (v_start < v_end && (line[v_start] == ' ' || line[v_start] == '\t')) : (v_start += 1) {}
        while (v_end > v_start && (line[v_end - 1] == ' ' || line[v_end - 1] == '\t')) : (v_end -= 1) {}

        const n_len = n_end - n_start;
        const v_len = v_end - v_start;
        if (_.trailer_storage_len + n_len + v_len > _.trailer_storage.len) return;

        const n_off = _.trailer_storage_len;
        let i: usize = 0;
        while (i < n_len) : (i += 1) {
            _.trailer_storage[n_off + i] = line[n_start + i];
        }
        _.trailer_storage_len += n_len;

        const v_off = _.trailer_storage_len;
        let j: usize = 0;
        while (j < v_len) : (j += 1) {
            _.trailer_storage[v_off + j] = line[v_start + j];
        }
        _.trailer_storage_len += v_len;

        _.trailer_spans[_.trailer_count] = ChunkTrailerSpan{
            name_off: n_off,
            name_len: n_len,
            val_off: v_off,
            val_len: v_len,
        };
        _.trailer_count += 1;
    }

    /// Incrementally decodes chunked wire bytes from `input` into `out`.
    /// Note: trailer slices in `trailers` point into `input` when parsed directly from a single chunked buffer.
    pub fn feed(*_, input: []const u8, out: []u8): ChunkedDecodeResult! {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        let trailer_line_start: usize = 0;

        while (in_pos < input.len) {
            switch (_.state) {
                .Done => {
                    return ChunkedDecodeResult{
                        bytes_written: out_pos,
                        bytes_consumed: in_pos,
                        is_done: true,
                        trailer_count: _.trailer_count,
                    };
                },
                .Error => {
                    return error.InvalidChunkedEncoding;
                },
                .ChunkSize => {
                    const ch = input[in_pos];
                    if (d := hexDigit(ch)) {
                        if (_.hex_count >= 16) {
                            _.state = ChunkedDecoderState.Error;
                            return error.ChunkSizeOverflow;
                        }
                        _.bytes_left_in_chunk = (_.bytes_left_in_chunk << 4) | d;
                        _.hex_count += 1;
                        in_pos += 1;
                    } else if (ch == ';') {
                        if (_.hex_count == 0) {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                        _.state = ChunkedDecoderState.ChunkExt;
                        in_pos += 1;
                    } else if (ch == ' ' || ch == '\t') {
                        if (_.hex_count == 0) {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                        _.state = ChunkedDecoderState.ChunkExt;
                        in_pos += 1;
                    } else if (ch == '\r') {
                        if (_.hex_count == 0) {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                        _.state = ChunkedDecoderState.ChunkExt;
                    } else if (ch == '\n') {
                        if (_.hex_count == 0) {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                        in_pos += 1;
                        _.hex_count = 0;
                        if (_.bytes_left_in_chunk == 0) {
                            _.state = if (_.consume_trailer) ChunkedDecoderState.TrailerHead else ChunkedDecoderState.Done;
                            trailer_line_start = in_pos;
                        } else {
                            _.state = ChunkedDecoderState.ChunkData;
                        }
                    } else {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                },
                .ChunkExt => {
                    const ch = input[in_pos];
                    if (ch == '\r') {
                        in_pos += 1;
                        if (in_pos < input.len && input[in_pos] == '\n') {
                            in_pos += 1;
                            _.hex_count = 0;
                            if (_.bytes_left_in_chunk == 0) {
                                _.state = if (_.consume_trailer) ChunkedDecoderState.TrailerHead else ChunkedDecoderState.Done;
                                trailer_line_start = in_pos;
                            } else {
                                _.state = ChunkedDecoderState.ChunkData;
                            }
                        } else if (in_pos == input.len) {
                            _.state = ChunkedDecoderState.ChunkExtLf;
                            return ChunkedDecodeResult{
                                bytes_written: out_pos,
                                bytes_consumed: in_pos,
                                is_done: false,
                                trailer_count: _.trailer_count,
                            };
                        } else {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                    } else if (ch == '\n') {
                        in_pos += 1;
                        _.hex_count = 0;
                        if (_.bytes_left_in_chunk == 0) {
                            _.state = if (_.consume_trailer) ChunkedDecoderState.TrailerHead else ChunkedDecoderState.Done;
                            trailer_line_start = in_pos;
                        } else {
                            _.state = ChunkedDecoderState.ChunkData;
                        }
                    } else {
                        // Skip extension bytes (e.g. ";ext=1;foo=bar")
                        in_pos += 1;
                    }
                },
                .ChunkExtLf => {
                    if (input[in_pos] != '\n') {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                    in_pos += 1;
                    _.hex_count = 0;
                    if (_.bytes_left_in_chunk == 0) {
                        _.state = if (_.consume_trailer) ChunkedDecoderState.TrailerHead else ChunkedDecoderState.Done;
                        trailer_line_start = in_pos;
                    } else {
                        _.state = ChunkedDecoderState.ChunkData;
                    }
                },
                .ChunkData => {
                    const avail_in = input.len - in_pos;
                    const avail_out = out.len - out_pos;
                    const to_copy = @min(_.bytes_left_in_chunk, @min(avail_in, avail_out));
                    if (to_copy == 0) {
                        if (avail_out == 0) {
                            return error.BufferTooSmall;
                        }
                        break;
                    }
                    @memcpy(out[out_pos .. out_pos + to_copy], input[in_pos .. in_pos + to_copy]);
                    in_pos += to_copy;
                    out_pos += to_copy;
                    _.total_decoded += to_copy;
                    _.bytes_left_in_chunk -= to_copy;
                    if (_.bytes_left_in_chunk == 0) {
                        _.state = ChunkedDecoderState.ChunkDataCr;
                    }
                },
                .ChunkDataCr => {
                    const ch = input[in_pos];
                    if (ch == '\r') {
                        in_pos += 1;
                        _.state = ChunkedDecoderState.ChunkDataLf;
                    } else if (ch == '\n') {
                        in_pos += 1;
                        _.state = ChunkedDecoderState.ChunkSize;
                        _.hex_count = 0;
                    } else {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                },
                .ChunkDataLf => {
                    if (input[in_pos] != '\n') {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                    in_pos += 1;
                    _.state = ChunkedDecoderState.ChunkSize;
                    _.hex_count = 0;
                },
                .TrailerHead => {
                    const ch = input[in_pos];
                    if (ch == '\r') {
                        in_pos += 1;
                        if (in_pos < input.len) {
                            if (input[in_pos] != '\n') {
                                _.state = ChunkedDecoderState.Error;
                                return error.InvalidChunkedEncoding;
                            }
                            in_pos += 1;
                            _.state = ChunkedDecoderState.Done;
                            return ChunkedDecodeResult{
                                bytes_written: out_pos,
                                bytes_consumed: in_pos,
                                is_done: true,
                                trailer_count: _.trailer_count,
                            };
                        }
                        _.state = ChunkedDecoderState.TrailerHeadLf;
                    } else if (ch == '\n') {
                        in_pos += 1;
                        _.state = ChunkedDecoderState.Done;
                        return ChunkedDecodeResult{
                            bytes_written: out_pos,
                            bytes_consumed: in_pos,
                            is_done: true,
                            trailer_count: _.trailer_count,
                        };
                    } else {
                        trailer_line_start = in_pos;
                        _.trailer_line_len = 0;
                        _.state = ChunkedDecoderState.TrailerLine;
                    }
                },
                .TrailerHeadLf => {
                    if (input[in_pos] != '\n') {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                    in_pos += 1;
                    _.state = ChunkedDecoderState.Done;
                    return ChunkedDecodeResult{
                        bytes_written: out_pos,
                        bytes_consumed: in_pos,
                        is_done: true,
                        trailer_count: _.trailer_count,
                    };
                },
                .TrailerLine => {
                    const ch = input[in_pos];
                    if (ch == '\r') {
                        const line_end = in_pos;
                        in_pos += 1;
                        if (trailer_line_start <= line_end && _.trailer_line_len == 0) {
                            _.recordTrailerFromSlice(input[trailer_line_start..line_end]);
                        } else if (_.trailer_line_len > 0) {
                            let local_line: [512]u8 = undefined;
                            const ll_len = _.trailer_line_len;
                            @memcpy(local_line[0..ll_len], _.trailer_line_buf[0..ll_len]);
                            _.trailer_line_len = 0;
                            _.recordTrailerFromSlice(local_line[0..ll_len]);
                        }
                        if (in_pos < input.len && input[in_pos] == '\n') {
                            in_pos += 1;
                            trailer_line_start = in_pos;
                            _.state = ChunkedDecoderState.TrailerHead;
                        } else if (in_pos == input.len) {
                            _.state = ChunkedDecoderState.TrailerLineLf;
                        } else {
                            _.state = ChunkedDecoderState.Error;
                            return error.InvalidChunkedEncoding;
                        }
                    } else if (ch == '\n') {
                        const line_end = in_pos;
                        in_pos += 1;
                        if (trailer_line_start <= line_end && _.trailer_line_len == 0) {
                            _.recordTrailerFromSlice(input[trailer_line_start..line_end]);
                        } else if (_.trailer_line_len > 0) {
                            let local_line: [512]u8 = undefined;
                            const ll_len = _.trailer_line_len;
                            @memcpy(local_line[0..ll_len], _.trailer_line_buf[0..ll_len]);
                            _.trailer_line_len = 0;
                            _.recordTrailerFromSlice(local_line[0..ll_len]);
                        }
                        trailer_line_start = in_pos;
                        _.state = ChunkedDecoderState.TrailerHead;
                    } else {
                        if (_.trailer_line_len < 512) {
                            _.trailer_line_buf[_.trailer_line_len] = ch;
                            _.trailer_line_len += 1;
                        }
                        in_pos += 1;
                    }
                },
                .TrailerLineLf => {
                    if (input[in_pos] != '\n') {
                        _.state = ChunkedDecoderState.Error;
                        return error.InvalidChunkedEncoding;
                    }
                    in_pos += 1;
                    trailer_line_start = in_pos;
                    _.state = ChunkedDecoderState.TrailerHead;
                },
            }
        }

        return ChunkedDecodeResult{
            bytes_written: out_pos,
            bytes_consumed: in_pos,
            is_done: _.state == ChunkedDecoderState.Done,
            trailer_count: _.trailer_count,
        };
    }

    /// One-shot convenience decoder requiring complete `0\r\n...\r\n` terminator.
    pub static fn decodeAll(input: []const u8, out: []u8): ChunkedDecodeResult! {
        let decoder = ChunkedDecoder.init(true);
        const res = decoder.feed(input, out)?;
        if (!res.is_done) {
            return error.IncompleteChunkedBody;
        }
        return res;
    }
}

pub struct ChunkedEncoder {
    /// Encodes a single non-empty chunk as `<hex-len>\r\n<data>\r\n`.
    pub static fn encodeChunk(data: []const u8, out: []u8): usize! {
        if (data.len == 0) return 0 as usize;
        let hdr_buf: [24]u8 = undefined;
        const hdr = std.fmt.bufPrint(&hdr_buf, "{x}\r\n", .{data.len}) catch return error.BufferTooSmall;
        const total = hdr.len + data.len + 2;
        if (out.len < total) return error.BufferTooSmall;
        @memcpy(out[0..hdr.len], hdr);
        @memcpy(out[hdr.len .. hdr.len + data.len], data);
        out[hdr.len + data.len] = '\r';
        out[hdr.len + data.len + 1] = '\n';
        return total;
    }

    /// Encodes the terminal empty chunk `0\r\n\r\n`.
    pub static fn encodeFinal(out: []u8): usize! {
        if (out.len < 5) return error.BufferTooSmall;
        @memcpy(out[0..5], "0\r\n\r\n");
        return 5 as usize;
    }

    /// Encodes the terminal chunk with trailer headers `0\r\nName: Value\r\n...\r\n`.
    pub static fn encodeFinalWithTrailers(trailers: []const ChunkTrailer, out: []u8): usize! {
        if (out.len < 3) return error.BufferTooSmall;
        @memcpy(out[0..3], "0\r\n");
        let pos: usize = 3;
        for (t of trailers) {
            const line = std.fmt.bufPrint(out[pos..], "{s}: {s}\r\n", .{ t.name, t.value }) catch return error.BufferTooSmall;
            pos += line.len;
        }
        if (pos + 2 > out.len) return error.BufferTooSmall;
        out[pos] = '\r';
        out[pos + 1] = '\n';
        return pos + 2;
    }

    /// Splits `data` into chunks of `max_chunk_size` and appends the `0\r\n\r\n` terminator.
    pub static fn encodeBody(data: []const u8, max_chunk_size: usize, out: []u8): usize! {
        const chunk_sz: usize = if (max_chunk_size == 0) 4096 else max_chunk_size;
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < data.len) {
            const take = @min(chunk_sz, data.len - in_pos);
            const written = encodeChunk(data[in_pos .. in_pos + take], out[out_pos..])?;
            out_pos += written;
            in_pos += take;
        }
        const fin = encodeFinal(out[out_pos..])?;
        return out_pos + fin;
    }
}
