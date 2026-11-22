// Origin: Pure-CP Stdlib (Zero-Alloc Streaming LLM Tool-Call Delta Assembler) | Created: 2026-11-22
// Pure CP zero-heap-allocation multi-tool streaming delta assembler for OpenAI & Anthropic tool calls:
//   - Tracks up to 16 concurrent tool calls keyed by arbitrary `tool_index` (out-of-order & sparse safe)
//   - Incrementally appends `call_id`, `tool_name`, and `arguments_json` fragments with zero heap allocation
//   - Built-in streaming JSON brace/bracket/string/escape FSM for real-time `isArgumentsComplete` detection
//   - Rejects unbalanced delimiters, trailing non-whitespace garbage, unescaped control chars & buffer overflow
//   - Batch stream finalization (`finalizeStream`: `complete | truncated | malformed`) and O(1) `reset()` reuse

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum concurrent tool calls tracked in a single streaming assistant turn.
pub const TOOL_CALL_MAX_SLOTS: usize = 16;

/// Maximum byte capacity for each assembled `call_id` (e.g. `call_abc123` / `toolu_01...`).
pub const TOOL_CALL_ID_MAX_LEN: usize = 64;

/// Maximum byte capacity for each assembled `tool_name`.
pub const TOOL_CALL_NAME_MAX_LEN: usize = 64;

/// Maximum byte capacity for each assembled `arguments_json` payload.
pub const TOOL_CALL_ARGS_MAX_LEN: usize = 1024;

/// Maximum JSON object/array nesting depth tracked by the streaming FSM.
pub const TOOL_CALL_MAX_JSON_DEPTH: usize = 32;

/// Lifecycle state of a single tool call's streaming JSON arguments.
pub enum ToolCallArgsState {
    empty,
    streaming,
    complete,
    truncated,
    malformed,
    overflow,
}

/// Immediate outcome returned by `ToolCallAssembler.feed` / `feedChunk`.
pub enum ToolCallFeedStatus {
    ok,
    completed,
    malformed,
    overflow,
    slots_full,
    stream_finalized,
}

/// Stream-level health verdict returned by `ToolCallAssembler.finalizeStream`.
pub enum ToolStreamFinalStatus {
    complete,
    truncated,
    malformed,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Single streaming delta fragment (OpenAI `delta.tool_calls[i]` or Anthropic `input_json_delta`).
pub struct ToolCallDelta {
    tool_index: u16 = 0;
    call_id_delta: []const u8 = "";
    name_delta: []const u8 = "";
    arguments_delta: []const u8 = "";
}

/// Assembled state and inline buffers for one streaming tool call slot.
pub struct AssembledToolCall {
    active: bool = false;
    tool_index: u16 = 0;

    call_id_buf: [TOOL_CALL_ID_MAX_LEN]u8 = [0; TOOL_CALL_ID_MAX_LEN];
    call_id_len: usize = 0;

    name_buf: [TOOL_CALL_NAME_MAX_LEN]u8 = [0; TOOL_CALL_NAME_MAX_LEN];
    name_len: usize = 0;

    args_buf: [TOOL_CALL_ARGS_MAX_LEN]u8 = [0; TOOL_CALL_ARGS_MAX_LEN];
    args_len: usize = 0;

    // Streaming JSON balance FSM state
    delim_stack: [TOOL_CALL_MAX_JSON_DEPTH]u8 = [0; TOOL_CALL_MAX_JSON_DEPTH];
    depth: usize = 0;
    in_string: bool = false;
    escape_next: bool = false;
    args_state: ToolCallArgsState = ToolCallArgsState.empty;
    delta_count: u32 = 0;

    /// Returns the assembled `call_id` as a read-only byte slice.
    fn callId(): []const u8 {
        return _.call_id_buf[0.._.call_id_len];
    }

    /// Returns the assembled `tool_name` as a read-only byte slice.
    fn toolName(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    /// Returns the assembled `arguments_json` payload as a read-only byte slice.
    fn argumentsJson(): []const u8 {
        return _.args_buf[0.._.args_len];
    }

    /// Returns `true` iff the tool call's JSON arguments are balanced and completely closed.
    fn isArgumentsComplete(): bool {
        return _.args_state == ToolCallArgsState.complete;
    }

    /// Returns the current JSON delimiter nesting depth.
    fn nestingDepth(): usize {
        return _.depth;
    }
}

/// Batch summary report produced by `ToolCallAssembler.finalizeStream`.
pub struct ToolCallStreamReport {
    status: ToolStreamFinalStatus = ToolStreamFinalStatus.complete;
    total_calls: usize = 0;
    complete_calls: usize = 0;
    truncated_calls: usize = 0;
    malformed_calls: usize = 0;
    overflow_calls: usize = 0;
    total_arg_bytes: usize = 0;
    delta_chunks_processed: u64 = 0;
}

// ============================================================================
// 3. Internal Helpers
// ============================================================================

fn isJsonWhitespace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn appendBounded(
    dst: *[]u8,
    dst_len: *usize,
    src: []const u8,
): bool {
    if (src.len == 0) {
        return true;
    }
    const cur_len: usize = dst_len.*;
    if (cur_len > dst.len || src.len > dst.len - cur_len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[cur_len + i] = src[i];
    }
    dst_len.* = cur_len + src.len;
    return true;
}

// ============================================================================
// 4. Zero-Alloc Multi-Tool Streaming Delta Assembler
// ============================================================================

/// Zero-heap-allocation streaming LLM tool-call delta assembler.
/// Supports parallel OpenAI/Anthropic tool-call deltas indexed by `tool_index`,
/// real-time JSON delimiter/string/escape completeness tracking, stream finalization,
/// and deterministic reset for multi-turn reuse.
pub struct ToolCallAssembler {
    slots: [TOOL_CALL_MAX_SLOTS]AssembledToolCall = [AssembledToolCall{}; TOOL_CALL_MAX_SLOTS];
    active_count: usize = 0;
    finalized: bool = false;
    delta_chunks_processed: u64 = 0;

    /// Initializes an empty `ToolCallAssembler`.
    static fn init(): ToolCallAssembler {
        return ToolCallAssembler{};
    }

    /// Resets all tracked slots and counters in place for reuse on the next LLM turn.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.active_count) : (i += 1) {
            _.slots[i] = AssembledToolCall{};
        }
        _.active_count = 0;
        _.finalized = false;
        _.delta_chunks_processed = 0;
    }

    /// Returns the number of distinct active `tool_index` calls currently tracked.
    fn activeCount(): usize {
        return _.active_count;
    }

    /// Returns `true` if `finalizeStream` has been called since the last `init`/`reset`.
    fn isFinalized(): bool {
        return _.finalized;
    }

    /// Finds the internal slot position for `tool_index`, or `null` if not present.
    fn findSlotIdx(tool_index: u16): usize? {
        let i: usize = 0;
        while (i < _.active_count) : (i += 1) {
            if (_.slots[i].active && _.slots[i].tool_index == tool_index) {
                return i;
            }
        }
        return null;
    }

    /// Feeds a `ToolCallDelta` fragment into the matching `tool_index` slot.
    fn feed(*_, delta: ToolCallDelta): ToolCallFeedStatus {
        if (_.finalized) {
            return ToolCallFeedStatus.stream_finalized;
        }

        // Locate existing slot or allocate a new inline slot.
        let slot_idx: usize = 0;
        if (existing := _.findSlotIdx(delta.tool_index)) {
            slot_idx = existing;
        } else {
            if (_.active_count >= TOOL_CALL_MAX_SLOTS) {
                return ToolCallFeedStatus.slots_full;
            }
            slot_idx = _.active_count;
            _.slots[slot_idx] = AssembledToolCall{
                active: true,
                tool_index: delta.tool_index,
            };
            _.active_count += 1;
        }

        _.delta_chunks_processed += 1;
        _.slots[slot_idx].delta_count += 1;

        // If slot already entered a terminal error state, stay sticky.
        if (_.slots[slot_idx].args_state == ToolCallArgsState.overflow) {
            return ToolCallFeedStatus.overflow;
        }
        if (_.slots[slot_idx].args_state == ToolCallArgsState.malformed) {
            return ToolCallFeedStatus.malformed;
        }

        // 1. Append `call_id` delta fragment.
        if (delta.call_id_delta.len > 0) {
            let id_slice: []u8 = _.slots[slot_idx].call_id_buf[0..];
            if (!appendBounded(*id_slice, *_.slots[slot_idx].call_id_len, delta.call_id_delta)) {
                _.slots[slot_idx].args_state = ToolCallArgsState.overflow;
                return ToolCallFeedStatus.overflow;
            }
        }

        // 2. Append `tool_name` delta fragment.
        if (delta.name_delta.len > 0) {
            let name_slice: []u8 = _.slots[slot_idx].name_buf[0..];
            if (!appendBounded(*name_slice, *_.slots[slot_idx].name_len, delta.name_delta)) {
                _.slots[slot_idx].args_state = ToolCallArgsState.overflow;
                return ToolCallFeedStatus.overflow;
            }
        }

        // 3. Feed `arguments_json` bytes through the zero-alloc streaming JSON balance FSM.
        const args = delta.arguments_delta;
        let i: usize = 0;
        while (i < args.len) : (i += 1) {
            const b: u8 = args[i];
            const st = _.slots[slot_idx].args_state;

            if (st == ToolCallArgsState.complete) {
                if (isJsonWhitespace(b)) {
                    continue;
                }
                // Non-whitespace trailing garbage after root JSON container closed.
                _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                return ToolCallFeedStatus.malformed;
            }

            if (st == ToolCallArgsState.empty) {
                if (isJsonWhitespace(b)) {
                    continue;
                }
                if (b != '{' && b != '[') {
                    _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                    return ToolCallFeedStatus.malformed;
                }
                if (_.slots[slot_idx].args_len >= TOOL_CALL_ARGS_MAX_LEN) {
                    _.slots[slot_idx].args_state = ToolCallArgsState.overflow;
                    return ToolCallFeedStatus.overflow;
                }
                _.slots[slot_idx].args_buf[_.slots[slot_idx].args_len] = b;
                _.slots[slot_idx].args_len += 1;
                _.slots[slot_idx].delim_stack[0] = if (b == '{') '}' else ']';
                _.slots[slot_idx].depth = 1;
                _.slots[slot_idx].args_state = ToolCallArgsState.streaming;
                continue;
            }

            // Streaming state: append byte and advance JSON delimiter/string/escape FSM.
            if (_.slots[slot_idx].args_len >= TOOL_CALL_ARGS_MAX_LEN) {
                _.slots[slot_idx].args_state = ToolCallArgsState.overflow;
                return ToolCallFeedStatus.overflow;
            }
            _.slots[slot_idx].args_buf[_.slots[slot_idx].args_len] = b;
            _.slots[slot_idx].args_len += 1;

            if (_.slots[slot_idx].in_string) {
                if (_.slots[slot_idx].escape_next) {
                    _.slots[slot_idx].escape_next = false;
                } else if (b == '\\') {
                    _.slots[slot_idx].escape_next = true;
                } else if (b == '"') {
                    _.slots[slot_idx].in_string = false;
                } else if (b < 0x20) {
                    // RFC 8259 forbids unescaped ASCII control chars inside JSON strings.
                    _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                    return ToolCallFeedStatus.malformed;
                }
                continue;
            }

            if (b == '"') {
                _.slots[slot_idx].in_string = true;
                _.slots[slot_idx].escape_next = false;
            } else if (b == '{' || b == '[') {
                if (_.slots[slot_idx].depth >= TOOL_CALL_MAX_JSON_DEPTH) {
                    _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                    return ToolCallFeedStatus.malformed;
                }
                _.slots[slot_idx].delim_stack[_.slots[slot_idx].depth] = if (b == '{') '}' else ']';
                _.slots[slot_idx].depth += 1;
            } else if (b == '}' || b == ']') {
                if (_.slots[slot_idx].depth == 0) {
                    _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                    return ToolCallFeedStatus.malformed;
                }
                const expected: u8 = _.slots[slot_idx].delim_stack[_.slots[slot_idx].depth - 1];
                if (b != expected) {
                    _.slots[slot_idx].args_state = ToolCallArgsState.malformed;
                    return ToolCallFeedStatus.malformed;
                }
                _.slots[slot_idx].depth -= 1;
                if (_.slots[slot_idx].depth == 0) {
                    _.slots[slot_idx].args_state = ToolCallArgsState.complete;
                }
            }
        }

        if (_.slots[slot_idx].args_state == ToolCallArgsState.complete) {
            return ToolCallFeedStatus.completed;
        }
        return ToolCallFeedStatus.ok;
    }

    /// Convenience wrapper around `feed` accepting positional delta slices.
    fn feedChunk(
        *_,
        tool_index: u16,
        call_id_delta: []const u8,
        name_delta: []const u8,
        arguments_delta: []const u8,
    ): ToolCallFeedStatus {
        return _.feed(ToolCallDelta{
            tool_index: tool_index,
            call_id_delta: call_id_delta,
            name_delta: name_delta,
            arguments_delta: arguments_delta,
        });
    }

    /// Returns `true` iff `tool_index` exists and its JSON arguments have balanced and closed.
    fn isArgumentsComplete(tool_index: u16): bool {
        if (idx := _.findSlotIdx(tool_index)) {
            return _.slots[idx].args_state == ToolCallArgsState.complete;
        }
        return false;
    }

    /// Returns the current `ToolCallArgsState` for `tool_index`, or `null` if unknown.
    fn toolState(tool_index: u16): ToolCallArgsState? {
        if (idx := _.findSlotIdx(tool_index)) {
            return _.slots[idx].args_state;
        }
        return null;
    }

    /// Returns a value snapshot of the assembled tool call for `tool_index`, or `null` if unknown.
    fn getToolCall(tool_index: u16): AssembledToolCall? {
        if (idx := _.findSlotIdx(tool_index)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Returns the assembled tool call at dense internal slot `slot_idx < activeCount()`.
    fn slotAt(slot_idx: usize): AssembledToolCall? {
        if (slot_idx >= _.active_count) {
            return null;
        }
        return _.slots[slot_idx];
    }

    /// Finalizes the stream, transitions any unfinished (`empty` or `streaming`) slots to `truncated`,
    /// and returns a batch `ToolCallStreamReport` with aggregate `complete | truncated | malformed` verdict.
    fn finalizeStream(*_): ToolCallStreamReport {
        _.finalized = true;

        let report = ToolCallStreamReport{
            status: ToolStreamFinalStatus.complete,
            total_calls: _.active_count,
            delta_chunks_processed: _.delta_chunks_processed,
        };

        let i: usize = 0;
        while (i < _.active_count) : (i += 1) {
            report.total_arg_bytes += _.slots[i].args_len;

            if (_.slots[i].args_state == ToolCallArgsState.empty || _.slots[i].args_state == ToolCallArgsState.streaming) {
                _.slots[i].args_state = ToolCallArgsState.truncated;
            }

            const st = _.slots[i].args_state;
            if (st == ToolCallArgsState.complete) {
                report.complete_calls += 1;
            } else if (st == ToolCallArgsState.truncated) {
                report.truncated_calls += 1;
            } else if (st == ToolCallArgsState.malformed) {
                report.malformed_calls += 1;
            } else if (st == ToolCallArgsState.overflow) {
                report.overflow_calls += 1;
            }
        }

        if (report.malformed_calls > 0 || report.overflow_calls > 0) {
            report.status = ToolStreamFinalStatus.malformed;
        } else if (report.truncated_calls > 0) {
            report.status = ToolStreamFinalStatus.truncated;
        } else {
            report.status = ToolStreamFinalStatus.complete;
        }

        return report;
    }
}
