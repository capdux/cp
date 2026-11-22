// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Streaming CoT Multi-Channel Tag Parser & Thinking-Budget Guard) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation streaming Chain-of-Thought (CoT) tag parser and thinking-budget guard
// for reasoning LLMs (DeepSeek-R1, Qwen3, Claude, o-series):
//   - Incremental feeding of arbitrary chunk sizes (down to 1-byte chunks splitting `<think>` or ```` ``` ````).
//   - Configurable thinking/answer tag families: `<think>`, `<thinking>`, `<reflection>`, `<answer>`.
//   - Markdown triple-backtick code-fence awareness (` ``` `): ignores pseudo tags inside code blocks.
//   - Thinking byte/token budget guard (`max_thinking_bytes`): emits `CotSegmentKind.budget_exceeded`
//     and drops excess thinking bytes so callers can truncate early and steer to final answer.
//   - Full boundary guards: empty chunks, nested/duplicate open tags, unclosed tags on `finish()`,
//     orphan closing tags, and resumable partial-drain when output buffers fill up.

// ============================================================================
// Constants & Tag Family Bitmasks
// ============================================================================

pub const COT_TAG_THINK: u32 = 1;
pub const COT_TAG_THINKING: u32 = 2;
pub const COT_TAG_REFLECTION: u32 = 4;
pub const COT_TAG_ANSWER: u32 = 8;
pub const COT_TAG_ALL: u32 = 15;

const COT_PENDING_CAP: usize = 24;

const TAG_OPEN_THINK: string = "<think>";
const TAG_CLOSE_THINK: string = "</think>";
const TAG_OPEN_THINKING: string = "<thinking>";
const TAG_CLOSE_THINKING: string = "</thinking>";
const TAG_OPEN_REFLECTION: string = "<reflection>";
const TAG_CLOSE_REFLECTION: string = "</reflection>";
const TAG_OPEN_ANSWER: string = "<answer>";
const TAG_CLOSE_ANSWER: string = "</answer>";

const MATCH_NONE: u8 = 0;
const MATCH_OPEN_THINK: u8 = 1;
const MATCH_CLOSE_THINK: u8 = 2;
const MATCH_OPEN_THINKING: u8 = 3;
const MATCH_CLOSE_THINKING: u8 = 4;
const MATCH_OPEN_REFLECTION: u8 = 5;
const MATCH_CLOSE_REFLECTION: u8 = 6;
const MATCH_OPEN_ANSWER: u8 = 7;
const MATCH_CLOSE_ANSWER: u8 = 8;

// ============================================================================
// Internal Tag Matching Helpers
// ============================================================================

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isPrefixWithByte(buf: []const u8, len: usize, extra: u8, target: string): bool {
    const total = len + 1;
    if (total > target.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        if (asciiLower(buf[i]) != target[i]) {
            return false;
        }
    }
    return asciiLower(extra) == target[len];
}

fn isExactWithByte(buf: []const u8, len: usize, extra: u8, target: string): bool {
    if (len + 1 != target.len) {
        return false;
    }
    return isPrefixWithByte(buf, len, extra, target);
}

fn matchExactTag(mask: u32, buf: []const u8, len: usize, extra: u8): u8 {
    if ((mask & COT_TAG_THINK) != 0) {
        if (isExactWithByte(buf, len, extra, TAG_OPEN_THINK)) return MATCH_OPEN_THINK;
        if (isExactWithByte(buf, len, extra, TAG_CLOSE_THINK)) return MATCH_CLOSE_THINK;
    }
    if ((mask & COT_TAG_THINKING) != 0) {
        if (isExactWithByte(buf, len, extra, TAG_OPEN_THINKING)) return MATCH_OPEN_THINKING;
        if (isExactWithByte(buf, len, extra, TAG_CLOSE_THINKING)) return MATCH_CLOSE_THINKING;
    }
    if ((mask & COT_TAG_REFLECTION) != 0) {
        if (isExactWithByte(buf, len, extra, TAG_OPEN_REFLECTION)) return MATCH_OPEN_REFLECTION;
        if (isExactWithByte(buf, len, extra, TAG_CLOSE_REFLECTION)) return MATCH_CLOSE_REFLECTION;
    }
    if ((mask & COT_TAG_ANSWER) != 0) {
        if (isExactWithByte(buf, len, extra, TAG_OPEN_ANSWER)) return MATCH_OPEN_ANSWER;
        if (isExactWithByte(buf, len, extra, TAG_CLOSE_ANSWER)) return MATCH_CLOSE_ANSWER;
    }
    return MATCH_NONE;
}

fn isAnyTagPrefix(mask: u32, buf: []const u8, len: usize, extra: u8): bool {
    if ((mask & COT_TAG_THINK) != 0) {
        if (isPrefixWithByte(buf, len, extra, TAG_OPEN_THINK)) return true;
        if (isPrefixWithByte(buf, len, extra, TAG_CLOSE_THINK)) return true;
    }
    if ((mask & COT_TAG_THINKING) != 0) {
        if (isPrefixWithByte(buf, len, extra, TAG_OPEN_THINKING)) return true;
        if (isPrefixWithByte(buf, len, extra, TAG_CLOSE_THINKING)) return true;
    }
    if ((mask & COT_TAG_REFLECTION) != 0) {
        if (isPrefixWithByte(buf, len, extra, TAG_OPEN_REFLECTION)) return true;
        if (isPrefixWithByte(buf, len, extra, TAG_CLOSE_REFLECTION)) return true;
    }
    if ((mask & COT_TAG_ANSWER) != 0) {
        if (isPrefixWithByte(buf, len, extra, TAG_OPEN_ANSWER)) return true;
        if (isPrefixWithByte(buf, len, extra, TAG_CLOSE_ANSWER)) return true;
    }
    return false;
}

// ============================================================================
// 1. Public Enums, Configuration & Result Models
// ============================================================================

/// Output segment channel emitted by `CotStreamParser`.
pub enum CotSegmentKind {
    thinking,
    answer,
    code_fence,
    budget_exceeded,
}

/// Configuration for `CotStreamParser`.
pub struct CotParserConfig {
    /// Bitmask of enabled tag families (`COT_TAG_THINK`, `COT_TAG_THINKING`, `COT_TAG_REFLECTION`, `COT_TAG_ANSWER`).
    tag_mask: u32 = COT_TAG_ALL;
    /// Maximum allowed thinking bytes (`0` = unlimited).
    max_thinking_bytes: usize = 0;
    /// When `true`, ignores pseudo CoT tags inside Markdown triple-backtick code fences.
    respect_code_fences: bool = true;

    static fn defaults(): CotParserConfig {
        return CotParserConfig{};
    }

    fn isValid(): bool {
        return (_.tag_mask & COT_TAG_ALL) != 0;
    }
}

/// Zero-copy segment event indexing into caller-provided `out_buf`.
pub struct CotSegmentEvent {
    kind: CotSegmentKind = CotSegmentKind.answer;
    start: usize = 0;
    len: usize = 0;
}

/// Incremental result returned by `feed` and `finish`.
pub struct CotFeedResult {
    bytes_consumed: usize = 0;
    bytes_written: usize = 0;
    events_emitted: usize = 0;
    budget_exceeded: bool = false;
    needs_drain: bool = false;
}

/// Cumulative stream summary produced by `CotStreamParser`.
pub struct CotParseSummary {
    thinking_bytes: u64 = 0;
    estimated_thinking_tokens: u64 = 0;
    dropped_thinking_bytes: u64 = 0;
    answer_bytes: u64 = 0;
    code_fence_bytes: u64 = 0;
    thinking_blocks: u32 = 0;
    answer_blocks: u32 = 0;
    code_fence_blocks: u32 = 0;
    unclosed_tags_repaired: u32 = 0;
    orphan_close_tags: u32 = 0;
    nested_open_tags: u32 = 0;
    ignored_code_fence_tags: u32 = 0;
    thinking_budget_exceeded: bool = false;
}

// ============================================================================
// 2. Streaming CoT Multi-Channel Tag Parser & Thinking-Budget Guard
// ============================================================================

/// Zero-heap-allocation streaming CoT tag demuxer and thinking-budget guard.
pub struct CotStreamParser {
    config: CotParserConfig = CotParserConfig{};
    in_thinking: bool = false;
    in_explicit_answer: bool = false;
    in_code_fence: bool = false;
    budget_triggered: bool = false;
    budget_event_emitted: bool = false;
    finished: bool = false;
    pending: [24]u8 = [0; 24];
    pending_len: usize = 0;
    drain_buf: [24]u8 = [0; 24];
    drain_pos: usize = 0;
    drain_len: usize = 0;
    drain_kind: CotSegmentKind = CotSegmentKind.answer;
    stats: CotParseSummary = CotParseSummary{};

    /// Constructs a validated `CotStreamParser`. Returns `null` if `config` enables no tag family.
    static fn init(config: CotParserConfig): CotStreamParser? {
        if (!config.isValid()) {
            return null;
        }
        return CotStreamParser{
            config: config,
        };
    }

    /// Resets all streaming state and counters for reuse with the same configuration.
    fn reset(*_) {
        _.in_thinking = false;
        _.in_explicit_answer = false;
        _.in_code_fence = false;
        _.budget_triggered = false;
        _.budget_event_emitted = false;
        _.finished = false;
        _.pending_len = 0;
        _.drain_pos = 0;
        _.drain_len = 0;
        _.drain_kind = CotSegmentKind.answer;
        _.stats = CotParseSummary{};
    }

    /// Returns the currently active output channel (`code_fence`, `thinking`, or `answer`).
    fn currentKind(): CotSegmentKind {
        if (_.in_code_fence) {
            return CotSegmentKind.code_fence;
        }
        if (_.in_thinking) {
            return CotSegmentKind.thinking;
        }
        return CotSegmentKind.answer;
    }

    /// Returns whether the thinking byte budget has been exceeded.
    fn isBudgetExceeded(): bool {
        return _.budget_triggered;
    }

    /// Returns a snapshot of cumulative parsing and budget statistics.
    fn summary(): CotParseSummary {
        let s = _.stats;
        s.estimated_thinking_tokens = (s.thinking_bytes + 3) / 4;
        return s;
    }

    /// Incrementally feeds `chunk` into the parser, writing output bytes into `out_buf` and
    /// structured segment descriptors into `out_events`.
    ///
    /// If `out_buf` or `out_events` fills up mid-chunk, sets `res.needs_drain = true` and returns
    /// exact `res.bytes_consumed` so the caller can drain outputs and call `feed(chunk[consumed..], ...)`.
    fn feed(
        *_,
        chunk: []const u8,
        out_buf: *[]u8,
        out_events: *[]CotSegmentEvent,
    ): CotFeedResult {
        let out_pos: usize = 0;
        let ev_count: usize = 0;
        let in_pos: usize = 0;

        if (!_.drainPending(*out_buf, &out_pos, *out_events, &ev_count)) {
            return CotFeedResult{
                bytes_consumed: 0,
                bytes_written: out_pos,
                events_emitted: ev_count,
                budget_exceeded: _.budget_triggered,
                needs_drain: true,
            };
        }

        while (in_pos < chunk.len) {
            if (!_.drainPending(*out_buf, &out_pos, *out_events, &ev_count)) {
                return CotFeedResult{
                    bytes_consumed: in_pos,
                    bytes_written: out_pos,
                    events_emitted: ev_count,
                    budget_exceeded: _.budget_triggered,
                    needs_drain: true,
                };
            }

            const b = chunk[in_pos];

            if (_.pending_len == 0) {
                if (b == '<') {
                    _.pending[0] = b;
                    _.pending_len = 1;
                    in_pos += 1;
                } else if (_.config.respect_code_fences && b == '`') {
                    _.pending[0] = b;
                    _.pending_len = 1;
                    in_pos += 1;
                } else {
                    const kind = _.currentKind();
                    if (!_.emitByte(kind, b, *out_buf, &out_pos, *out_events, &ev_count)) {
                        return CotFeedResult{
                            bytes_consumed: in_pos,
                            bytes_written: out_pos,
                            events_emitted: ev_count,
                            budget_exceeded: _.budget_triggered,
                            needs_drain: true,
                        };
                    }
                    in_pos += 1;
                }
            } else if (_.pending[0] == '`') {
                if (b == '`' && _.pending_len < 3) {
                    _.pending[_.pending_len] = b;
                    _.pending_len += 1;
                    in_pos += 1;
                    if (_.pending_len == 3) {
                        _.in_code_fence = !_.in_code_fence;
                        if (_.in_code_fence) {
                            _.stats.code_fence_blocks += 1;
                        }
                        _.queuePendingForDrain(CotSegmentKind.code_fence);
                    }
                } else {
                    // False fence prefix (` or ``): drain as current kind and re-evaluate `b`.
                    _.queuePendingForDrain(_.currentKind());
                }
            } else {
                // _.pending[0] == '<'
                const matched = matchExactTag(_.config.tag_mask, _.pending[0..], _.pending_len, b);
                if (matched != MATCH_NONE) {
                    if (_.in_code_fence) {
                        // Pseudo tag inside code fence: ignore state change and emit verbatim as code_fence.
                        _.stats.ignored_code_fence_tags += 1;
                        _.pending[_.pending_len] = b;
                        _.pending_len += 1;
                        _.queuePendingForDrain(CotSegmentKind.code_fence);
                    } else {
                        _.pending_len = 0;
                        _.applyTagTransition(matched);
                    }
                    in_pos += 1;
                } else if (_.pending_len + 1 < COT_PENDING_CAP &&
                    isAnyTagPrefix(_.config.tag_mask, _.pending[0..], _.pending_len, b))
                {
                    _.pending[_.pending_len] = b;
                    _.pending_len += 1;
                    in_pos += 1;
                } else {
                    // False tag prefix: drain buffered bytes and re-evaluate `b`.
                    _.queuePendingForDrain(_.currentKind());
                }
            }
        }

        const drained_all = _.drainPending(*out_buf, &out_pos, *out_events, &ev_count);
        return CotFeedResult{
            bytes_consumed: in_pos,
            bytes_written: out_pos,
            events_emitted: ev_count,
            budget_exceeded: _.budget_triggered,
            needs_drain: !drained_all,
        };
    }

    /// Flushes any buffered partial-tag bytes at end-of-stream and auto-repairs unclosed tags.
    /// If output buffers are too small, returns `needs_drain = true` so the caller can call
    /// `finish` again until `needs_drain == false`.
    fn finish(
        *_,
        out_buf: *[]u8,
        out_events: *[]CotSegmentEvent,
    ): CotFeedResult {
        let out_pos: usize = 0;
        let ev_count: usize = 0;

        if (!_.drainPending(*out_buf, &out_pos, *out_events, &ev_count)) {
            return CotFeedResult{
                bytes_consumed: 0,
                bytes_written: out_pos,
                events_emitted: ev_count,
                budget_exceeded: _.budget_triggered,
                needs_drain: true,
            };
        }

        if (_.pending_len > 0) {
            _.queuePendingForDrain(_.currentKind());
            if (!_.drainPending(*out_buf, &out_pos, *out_events, &ev_count)) {
                return CotFeedResult{
                    bytes_consumed: 0,
                    bytes_written: out_pos,
                    events_emitted: ev_count,
                    budget_exceeded: _.budget_triggered,
                    needs_drain: true,
                };
            }
        }

        if (!_.finished) {
            if (_.in_thinking) {
                _.in_thinking = false;
                _.stats.unclosed_tags_repaired += 1;
            }
            if (_.in_explicit_answer) {
                _.in_explicit_answer = false;
                _.stats.unclosed_tags_repaired += 1;
            }
            if (_.in_code_fence) {
                _.in_code_fence = false;
                _.stats.unclosed_tags_repaired += 1;
            }
            _.finished = true;
        }

        return CotFeedResult{
            bytes_consumed: 0,
            bytes_written: out_pos,
            events_emitted: ev_count,
            budget_exceeded: _.budget_triggered,
            needs_drain: false,
        };
    }

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    fn queuePendingForDrain(*_, kind: CotSegmentKind) {
        let i: usize = 0;
        while (i < _.pending_len) : (i += 1) {
            _.drain_buf[i] = _.pending[i];
        }
        _.drain_pos = 0;
        _.drain_len = _.pending_len;
        _.drain_kind = kind;
        _.pending_len = 0;
    }

    fn drainPending(
        *_,
        out_buf: *[]u8,
        out_pos: *usize,
        out_events: *[]CotSegmentEvent,
        ev_count: *usize,
    ): bool {
        if (_.budget_triggered && !_.budget_event_emitted) {
            if (ev_count.* >= out_events.len) {
                return false;
            }
            out_events[ev_count.*] = CotSegmentEvent{
                kind: CotSegmentKind.budget_exceeded,
                start: out_pos.*,
                len: 0,
            };
            ev_count.* += 1;
            _.budget_event_emitted = true;
        }

        while (_.drain_pos < _.drain_len) {
            const b = _.drain_buf[_.drain_pos];
            if (!_.emitByte(_.drain_kind, b, *out_buf, out_pos, *out_events, ev_count)) {
                return false;
            }
            _.drain_pos += 1;
        }
        _.drain_pos = 0;
        _.drain_len = 0;
        return true;
    }

    fn applyTagTransition(*_, matched: u8) {
        if (matched == MATCH_OPEN_THINK ||
            matched == MATCH_OPEN_THINKING ||
            matched == MATCH_OPEN_REFLECTION)
        {
            if (_.in_thinking) {
                _.stats.nested_open_tags += 1;
            } else {
                _.in_thinking = true;
                _.in_explicit_answer = false;
                _.stats.thinking_blocks += 1;
            }
            return;
        }

        if (matched == MATCH_CLOSE_THINK ||
            matched == MATCH_CLOSE_THINKING ||
            matched == MATCH_CLOSE_REFLECTION)
        {
            if (_.in_thinking) {
                _.in_thinking = false;
            } else {
                _.stats.orphan_close_tags += 1;
            }
            return;
        }

        if (matched == MATCH_OPEN_ANSWER) {
            if (_.in_thinking) {
                _.in_thinking = false;
                _.stats.unclosed_tags_repaired += 1;
            }
            _.in_explicit_answer = true;
            _.stats.answer_blocks += 1;
            return;
        }

        if (matched == MATCH_CLOSE_ANSWER) {
            if (_.in_explicit_answer) {
                _.in_explicit_answer = false;
            } else {
                _.stats.orphan_close_tags += 1;
            }
        }
    }

    fn emitByte(
        *_,
        kind: CotSegmentKind,
        b: u8,
        out_buf: *[]u8,
        out_pos: *usize,
        out_events: *[]CotSegmentEvent,
        ev_count: *usize,
    ): bool {
        // Thinking-budget enforcement
        if (kind == CotSegmentKind.thinking && _.config.max_thinking_bytes > 0) {
            if (_.budget_triggered) {
                _.stats.dropped_thinking_bytes += 1;
                return true;
            }
            if (_.stats.thinking_bytes >= (_.config.max_thinking_bytes as u64)) {
                _.budget_triggered = true;
                _.stats.thinking_budget_exceeded = true;
                if (ev_count.* >= out_events.len) {
                    return false;
                }
                out_events[ev_count.*] = CotSegmentEvent{
                    kind: CotSegmentKind.budget_exceeded,
                    start: out_pos.*,
                    len: 0,
                };
                ev_count.* += 1;
                _.budget_event_emitted = true;
                _.stats.dropped_thinking_bytes += 1;
                return true;
            }
        }

        if (out_pos.* >= out_buf.len) {
            return false;
        }

        const can_coalesce =
            ev_count.* > 0 &&
            out_events[ev_count.* - 1].kind == kind &&
            out_events[ev_count.* - 1].start + out_events[ev_count.* - 1].len == out_pos.*;

        if (!can_coalesce) {
            if (ev_count.* >= out_events.len) {
                return false;
            }
            out_events[ev_count.*] = CotSegmentEvent{
                kind: kind,
                start: out_pos.*,
                len: 0,
            };
            ev_count.* += 1;
        }

        out_buf[out_pos.*] = b;
        out_pos.* += 1;
        out_events[ev_count.* - 1].len += 1;

        if (kind == CotSegmentKind.thinking) {
            _.stats.thinking_bytes += 1;
        } else if (kind == CotSegmentKind.code_fence) {
            _.stats.code_fence_bytes += 1;
        } else if (kind == CotSegmentKind.answer) {
            _.stats.answer_bytes += 1;
        }

        return true;
    }
}
