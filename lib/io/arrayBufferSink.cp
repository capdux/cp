// Source: Absorbed from /data/dev/bun/src/runtime/webcore/ArrayBufferSink.cp | Synced: 2026-11-22T16:38:27Z
// Pure-CP zero-allocation incremental byte writer mirroring `Bun.ArrayBufferSink`:
// supports UTF-8 / Latin-1 / UTF-16 incremental writes, streaming flush mode,
// highWaterMark backpressure threshold, and terminal end() semantics.

pub const ARRAY_BUFFER_SINK_CAP: usize = 8192;
pub const ARRAY_BUFFER_SINK_DEFAULT_HWM: usize = 1024;

pub err ArrayBufferSinkError {
    SinkEnded,
    BufferOverflow,
    OutputTooSmall,
}

pub struct ArrayBufferSinkOptions {
    high_water_mark: usize = ARRAY_BUFFER_SINK_DEFAULT_HWM,
    stream: bool = false,
    as_uint8array: bool = false,
}

pub struct SinkFlushResult {
    bytes_flushed: usize = 0,
    is_stream_chunk: bool = false,
    as_uint8array: bool = false,
}

/// Zero-allocation incremental buffer sink aligned with `Bun.ArrayBufferSink`.
pub struct ArrayBufferSink {
    buf: [ARRAY_BUFFER_SINK_CAP]u8 = [0; ARRAY_BUFFER_SINK_CAP],
    len: usize = 0,
    high_water_mark: usize = ARRAY_BUFFER_SINK_DEFAULT_HWM,
    streaming: bool = false,
    as_uint8array: bool = false,
    ended: bool = false,
    total_written: usize = 0,

    static fn init(): ArrayBufferSink {
        return ArrayBufferSink {};
    }

    /// Resets the sink and applies streaming / highWaterMark / output-kind options.
    fn start(*_, opts: ArrayBufferSinkOptions) {
        _.len = 0;
        _.ended = false;
        _.streaming = opts.stream;
        _.as_uint8array = opts.as_uint8array;
        if (opts.high_water_mark == 0) {
            _.high_water_mark = ARRAY_BUFFER_SINK_DEFAULT_HWM;
        } else if (opts.high_water_mark > ARRAY_BUFFER_SINK_CAP) {
            _.high_water_mark = ARRAY_BUFFER_SINK_CAP;
        } else {
            _.high_water_mark = opts.high_water_mark;
        }
    }

    fn bufferedLen(): usize {
        return _.len;
    }

    fn capacity(): usize {
        return ARRAY_BUFFER_SINK_CAP;
    }

    fn remainingCap(): usize {
        return ARRAY_BUFFER_SINK_CAP - _.len;
    }

    fn totalWritten(): usize {
        return _.total_written;
    }

    fn isEnded(): bool {
        return _.ended;
    }

    fn isStreaming(): bool {
        return _.streaming;
    }

    fn highWaterMark(): usize {
        return _.high_water_mark;
    }

    fn shouldFlush(): bool {
        return _.len >= _.high_water_mark;
    }

    fn peek(): []const u8 {
        return _.buf[0.._.len];
    }

    fn pushByte(*_, b: u8): ! {
        if (_.len >= ARRAY_BUFFER_SINK_CAP) return ArrayBufferSinkError.BufferOverflow;
        _.buf[_.len] = b;
        _.len += 1;
    }

    /// Appends raw UTF-8 / binary bytes and returns number of bytes written.
    fn write(*_, data: []const u8): usize! {
        if (_.ended) return ArrayBufferSinkError.SinkEnded;
        if (_.len + data.len > ARRAY_BUFFER_SINK_CAP) return ArrayBufferSinkError.BufferOverflow;
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.len + i] = data[i];
            i += 1;
        }
        _.len += data.len;
        _.total_written += data.len;
        return data.len;
    }

    fn writeByte(*_, b: u8): usize! {
        if (_.ended) return ArrayBufferSinkError.SinkEnded;
        _.pushByte(b)?;
        _.total_written += 1;
        return 1;
    }

    /// Transcodes Latin-1 (ISO-8859-1) bytes into UTF-8 and appends to the sink.
    /// Returns the number of UTF-8 bytes written.
    fn writeLatin1(*_, latin1: []const u8): usize! {
        if (_.ended) return ArrayBufferSinkError.SinkEnded;
        let needed: usize = 0;
        let k: usize = 0;
        while (k < latin1.len) {
            needed += if (latin1[k] < 0x80) @as(usize, 1) else @as(usize, 2);
            k += 1;
        }
        if (_.len + needed > ARRAY_BUFFER_SINK_CAP) return ArrayBufferSinkError.BufferOverflow;

        let i: usize = 0;
        while (i < latin1.len) {
            const c = latin1[i];
            if (c < 0x80) {
                _.buf[_.len] = c;
                _.len += 1;
            } else {
                _.buf[_.len] = 0xC0 | (c >> 6);
                _.buf[_.len + 1] = 0x80 | (c & 0x3F);
                _.len += 2;
            }
            i += 1;
        }
        _.total_written += needed;
        return needed;
    }

    /// Transcodes UTF-16 code units (including surrogate pairs; lone surrogates -> U+FFFD)
    /// into UTF-8 and appends to the sink. Returns UTF-8 bytes written.
    fn writeUtf16(*_, units: []const u16): usize! {
        if (_.ended) return ArrayBufferSinkError.SinkEnded;
        const start_len = _.len;
        let i: usize = 0;
        while (i < units.len) {
            const u = units[i];
            let cp: u32 = @as(u32, u);
            if (u >= 0xD800 && u <= 0xDBFF) {
                if (i + 1 < units.len) {
                    const low = units[i + 1];
                    if (low >= 0xDC00 && low <= 0xDFFF) {
                        cp = @as(u32, 0x10000) + ((@as(u32, u - 0xD800) << 10) | @as(u32, low - 0xDC00));
                        i += 1;
                    } else {
                        cp = 0xFFFD;
                    }
                } else {
                    cp = 0xFFFD;
                }
            } else if (u >= 0xDC00 && u <= 0xDFFF) {
                cp = 0xFFFD;
            }

            if (cp < 0x80) {
                if (_.len + 1 > ARRAY_BUFFER_SINK_CAP) {
                    _.len = start_len;
                    return ArrayBufferSinkError.BufferOverflow;
                }
                _.buf[_.len] = @as(u8, cp);
                _.len += 1;
            } else if (cp < 0x800) {
                if (_.len + 2 > ARRAY_BUFFER_SINK_CAP) {
                    _.len = start_len;
                    return ArrayBufferSinkError.BufferOverflow;
                }
                _.buf[_.len] = @as(u8, 0xC0 | (cp >> 6));
                _.buf[_.len + 1] = @as(u8, 0x80 | (cp & 0x3F));
                _.len += 2;
            } else if (cp < 0x10000) {
                if (_.len + 3 > ARRAY_BUFFER_SINK_CAP) {
                    _.len = start_len;
                    return ArrayBufferSinkError.BufferOverflow;
                }
                _.buf[_.len] = @as(u8, 0xE0 | (cp >> 12));
                _.buf[_.len + 1] = @as(u8, 0x80 | ((cp >> 6) & 0x3F));
                _.buf[_.len + 2] = @as(u8, 0x80 | (cp & 0x3F));
                _.len += 3;
            } else {
                if (_.len + 4 > ARRAY_BUFFER_SINK_CAP) {
                    _.len = start_len;
                    return ArrayBufferSinkError.BufferOverflow;
                }
                _.buf[_.len] = @as(u8, 0xF0 | (cp >> 18));
                _.buf[_.len + 1] = @as(u8, 0x80 | ((cp >> 12) & 0x3F));
                _.buf[_.len + 2] = @as(u8, 0x80 | ((cp >> 6) & 0x3F));
                _.buf[_.len + 3] = @as(u8, 0x80 | (cp & 0x3F));
                _.len += 4;
            }
            i += 1;
        }
        const written = _.len - start_len;
        _.total_written += written;
        return written;
    }

    /// Flushes buffered data:
    /// - In streaming mode (`stream: true`): copies buffered bytes into `out`, resets `len = 0`,
    ///   and returns `SinkFlushResult{ bytes_flushed: n, is_stream_chunk: true, ... }`.
    /// - In non-streaming mode: preserves buffered bytes and reports current buffered count.
    fn flush(*_, out: []u8): SinkFlushResult! {
        if (!_.streaming) {
            return SinkFlushResult {
                bytes_flushed: _.len,
                is_stream_chunk: false,
                as_uint8array: _.as_uint8array,
            };
        }
        if (_.len > out.len) return ArrayBufferSinkError.OutputTooSmall;
        let i: usize = 0;
        while (i < _.len) {
            out[i] = _.buf[i];
            i += 1;
        }
        const n = _.len;
        _.len = 0;
        return SinkFlushResult {
            bytes_flushed: n,
            is_stream_chunk: true,
            as_uint8array: _.as_uint8array,
        };
    }

    /// Finalizes the sink, copying remaining buffered bytes into `out` and marking `ended = true`.
    /// Subsequent writes fail with `SinkEnded` until `start()` is called again.
    fn end(*_, out: []u8): usize! {
        if (_.len > out.len) return ArrayBufferSinkError.OutputTooSmall;
        let i: usize = 0;
        while (i < _.len) {
            out[i] = _.buf[i];
            i += 1;
        }
        const n = _.len;
        _.len = 0;
        _.ended = true;
        return n;
    }
}
