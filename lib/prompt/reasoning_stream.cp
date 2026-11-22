// Origin: Pure-CP Stdlib (AI Reasoning Model Streaming <think>/<tool_call> Tag Demuxer) | Created: 2026-11-22
// Zero-heap-allocation incremental streaming tag demuxer for reasoning LLMs (DeepSeek-R1, Claude, Qwen3).
// Splits arbitrary chunked streams (including 1-byte chunks splitting tags like "<thi" + "nk>")
// into Thinking, ToolCall, and Answer channels with safe false-prefix rollback and flush.

/// Output channel produced by `ReasoningStreamDemuxer`.
pub enum StreamChannel {
    Answer,
    Thinking,
    ToolCall,
}

/// Zero-copy slice descriptor indexing into the caller-provided `out_buf` of a `feed`/`flush` call.
pub struct DemuxEvent {
    channel: StreamChannel = StreamChannel.Answer;
    start: usize = 0;
    len: usize = 0;
}

/// Cumulative byte and block telemetry tracked by `ReasoningStreamDemuxer`.
pub struct DemuxStats {
    thinking_bytes: u64 = 0;
    tool_bytes: u64 = 0;
    answer_bytes: u64 = 0;
    thought_blocks: u32 = 0;
    tool_blocks: u32 = 0;
}

const TAG_OPEN_THINK: string = "<think>";
const TAG_CLOSE_THINK: string = "</think>";
const TAG_OPEN_TOOL: string = "<tool_call>";
const TAG_CLOSE_TOOL: string = "</tool_call>";

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isCaseInsensitivePrefix(buf: []const u8, len: usize, target: string): bool {
    if (len > target.len) return false;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        if (asciiLower(buf[i]) != target[i]) return false;
    }
    return true;
}

fn isCaseInsensitiveEqual(buf: []const u8, len: usize, target: string): bool {
    if (len != target.len) return false;
    return isCaseInsensitivePrefix(buf, len, target);
}

/// Zero-heap-allocation incremental demuxer for `<think>...</think>` and `<tool_call>...</tool_call>`.
pub struct ReasoningStreamDemuxer {
    current_channel: StreamChannel = StreamChannel.Answer;
    pending: [16]u8 = [0; 16];
    pending_len: usize = 0;
    stats_acc: DemuxStats = DemuxStats{};

    /// Initializes a fresh demuxer in `StreamChannel.Answer`.
    static fn init(): ReasoningStreamDemuxer {
        return ReasoningStreamDemuxer{
            current_channel: StreamChannel.Answer,
            pending: [0; 16],
            pending_len: 0,
            stats_acc: DemuxStats{},
        };
    }

    /// Returns the currently active stream channel.
    fn channel(): StreamChannel {
        return _.current_channel;
    }

    /// Returns the number of buffered partial-tag bytes awaiting disambiguation.
    fn pendingLen(): usize {
        return _.pending_len;
    }

    /// Returns a snapshot of cumulative byte and block counts.
    fn stats(): DemuxStats {
        return _.stats_acc;
    }

    /// Resets channel state, partial-tag buffer, and statistics for stream reuse.
    fn reset(*_) {
        _.current_channel = StreamChannel.Answer;
        _.pending_len = 0;
        _.stats_acc = DemuxStats{};
    }

    fn emitByte(
        *_,
        b: u8,
        out_buf: *[]u8,
        out_pos: *usize,
        out_events: *[]DemuxEvent,
        ev_count: *usize,
    ): bool {
        if (out_pos.* >= out_buf.len) return false;

        // Coalesce contiguous bytes on the same channel into a single DemuxEvent
        if (ev_count.* > 0 && out_events[ev_count.* - 1].channel == _.current_channel) {
            out_buf[out_pos.*] = b;
            out_pos.* += 1;
            out_events[ev_count.* - 1].len += 1;
        } else {
            if (ev_count.* >= out_events.len) return false;
            out_buf[out_pos.*] = b;
            out_events[ev_count.*] = DemuxEvent{
                channel: _.current_channel,
                start: out_pos.*,
                len: 1,
            };
            out_pos.* += 1;
            ev_count.* += 1;
        }

        if (_.current_channel == StreamChannel.Thinking) {
            _.stats_acc.thinking_bytes += 1;
        } else if (_.current_channel == StreamChannel.ToolCall) {
            _.stats_acc.tool_bytes += 1;
        } else {
            _.stats_acc.answer_bytes += 1;
        }
        return true;
    }

    fn shiftPendingLeftOne(*_) {
        if (_.pending_len == 0) return;
        let i: usize = 1;
        while (i < _.pending_len) : (i += 1) {
            _.pending[i - 1] = _.pending[i];
        }
        _.pending_len -= 1;
    }

    /// Evaluates current `pending[0..pending_len]` against tags valid in `current_channel`.
    /// Emits non-matching prefix bytes safely and performs channel transitions on exact matches.
    fn resolvePending(
        *_,
        out_buf: *[]u8,
        out_pos: *usize,
        out_events: *[]DemuxEvent,
        ev_count: *usize,
    ): bool {
        while (_.pending_len > 0) {
            const p_slice: []const u8 = _.pending[0.._.pending_len];
            if (_.current_channel == StreamChannel.Answer) {
                if (isCaseInsensitiveEqual(p_slice, _.pending_len, TAG_OPEN_THINK)) {
                    _.pending_len = 0;
                    _.current_channel = StreamChannel.Thinking;
                    _.stats_acc.thought_blocks += 1;
                    return true;
                }
                if (isCaseInsensitiveEqual(p_slice, _.pending_len, TAG_OPEN_TOOL)) {
                    _.pending_len = 0;
                    _.current_channel = StreamChannel.ToolCall;
                    _.stats_acc.tool_blocks += 1;
                    return true;
                }
                if (isCaseInsensitivePrefix(p_slice, _.pending_len, TAG_OPEN_THINK)
                    || isCaseInsensitivePrefix(p_slice, _.pending_len, TAG_OPEN_TOOL))
                {
                    return true;
                }
            } else if (_.current_channel == StreamChannel.Thinking) {
                if (isCaseInsensitiveEqual(p_slice, _.pending_len, TAG_CLOSE_THINK)) {
                    _.pending_len = 0;
                    _.current_channel = StreamChannel.Answer;
                    return true;
                }
                if (isCaseInsensitivePrefix(p_slice, _.pending_len, TAG_CLOSE_THINK)) {
                    return true;
                }
            } else {
                // StreamChannel.ToolCall
                if (isCaseInsensitiveEqual(p_slice, _.pending_len, TAG_CLOSE_TOOL)) {
                    _.pending_len = 0;
                    _.current_channel = StreamChannel.Answer;
                    return true;
                }
                if (isCaseInsensitivePrefix(p_slice, _.pending_len, TAG_CLOSE_TOOL)) {
                    return true;
                }
            }

            // Current pending buffer is NOT a valid tag prefix: emit its first byte and retry remainder
            const first = _.pending[0];
            if (!_.emitByte(first, out_buf, out_pos, out_events, ev_count)) {
                return false;
            }
            _.shiftPendingLeftOne();
        }
        return true;
    }

    /// Feeds an arbitrary chunk into the streaming demuxer, writing demuxed payload bytes into
    /// `out_buf` and channel spans into `out_events`.
    /// Returns the number of `DemuxEvent` entries written, or `null` if `out_buf` or `out_events`
    /// runs out of capacity. Empty `chunk` returns `0`.
    fn feed(
        *_,
        chunk: string,
        out_buf: *[]u8,
        out_events: *[]DemuxEvent,
    ): usize? {
        if (chunk.len == 0) return 0;

        let out_pos: usize = 0;
        let ev_count: usize = 0;

        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            const b = chunk[i];
            if (_.pending_len == 0 && b != '<') {
                if (!_.emitByte(b, out_buf, *out_pos, out_events, *ev_count)) {
                    return null;
                }
                continue;
            }

            // Append byte to partial-tag buffer and resolve
            _.pending[_.pending_len] = b;
            _.pending_len += 1;
            if (!_.resolvePending(out_buf, *out_pos, out_events, *ev_count)) {
                return null;
            }
        }

        return ev_count;
    }

    /// Flushes any unclosed partial-tag bytes buffered at stream EOF into the current channel.
    /// Returns the number of `DemuxEvent` entries written (0 or 1), or `null` if output capacity is insufficient.
    fn flush(
        *_,
        out_buf: *[]u8,
        out_events: *[]DemuxEvent,
    ): usize? {
        if (_.pending_len == 0) return 0;

        let out_pos: usize = 0;
        let ev_count: usize = 0;

        let i: usize = 0;
        while (i < _.pending_len) : (i += 1) {
            if (!_.emitByte(_.pending[i], out_buf, *out_pos, out_events, *ev_count)) {
                return null;
            }
        }
        _.pending_len = 0;
        return ev_count;
    }
}
