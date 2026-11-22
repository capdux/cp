// Origin: Pure-CP Stdlib (AI Agent ReAct State Machine, Step Trace & Binary Checkpoint) | Created: 2026-11-22

/// Binary checkpoint header magic ("CPAG").
pub const CHECKPOINT_MAGIC: u32 = 0x43504147;

const CHECKPOINT_VERSION: u16 = 1;
const CHECKPOINT_HEADER_LEN: usize = 36;
const CHECKPOINT_ENTRY_LEN: usize = 30;
const CHECKPOINT_FOOTER_LEN: usize = 8;

const FNV1A_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME: u64 = 0x00000100000001B3;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV1A_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ (data[i] as u64)) *% FNV1A_PRIME;
        i += 1;
    }
    return h;
}

fn writeU16LE(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = (v & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = (v & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64LE(buf: *[]u8, off: usize, v: u64) {
    buf.*[off] = (v & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 3] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 4] = ((v >> 32) & 0xFF) as u8;
    buf.*[off + 5] = ((v >> 40) & 0xFF) as u8;
    buf.*[off + 6] = ((v >> 48) & 0xFF) as u8;
    buf.*[off + 7] = ((v >> 56) & 0xFF) as u8;
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU64LE(data: []const u8, off: usize): u64 {
    return (data[off] as u64) |
        ((data[off + 1] as u64) << 8) |
        ((data[off + 2] as u64) << 16) |
        ((data[off + 3] as u64) << 24) |
        ((data[off + 4] as u64) << 32) |
        ((data[off + 5] as u64) << 40) |
        ((data[off + 6] as u64) << 48) |
        ((data[off + 7] as u64) << 56);
}

fn encodeZigZagI64(v: i64): u64 {
    if (v >= 0) {
        return (v as u64) << 1;
    }
    const mag = (-(v + 1)) as u64;
    return (mag << 1) | 1;
}

fn decodeZigZagI64(u: u64): i64 {
    if ((u & 1) == 0) {
        return (u >> 1) as i64;
    }
    return -((u >> 1) as i64) - 1;
}

/// Lifecycle phase of a ReAct agent execution session.
pub enum ReactPhase {
    Idle,
    Thinking,
    AwaitingTool,
    Observing,
    Completed,
    Failed,
    Halted,
}

fn phaseToU8(p: ReactPhase): u8 {
    match (p) {
        ReactPhase.Idle => return 0,
        ReactPhase.Thinking => return 1,
        ReactPhase.AwaitingTool => return 2,
        ReactPhase.Observing => return 3,
        ReactPhase.Completed => return 4,
        ReactPhase.Failed => return 5,
        ReactPhase.Halted => return 6,
    }
}

fn phaseFromU8(b: u8): ReactPhase? {
    if (b == 0) return ReactPhase.Idle;
    if (b == 1) return ReactPhase.Thinking;
    if (b == 2) return ReactPhase.AwaitingTool;
    if (b == 3) return ReactPhase.Observing;
    if (b == 4) return ReactPhase.Completed;
    if (b == 5) return ReactPhase.Failed;
    if (b == 6) return ReactPhase.Halted;
    return null;
}

/// Category of an individual trace record inside `ReactSession`.
pub enum StepKind {
    Thought,
    ToolCall,
    ToolResult,
    FinalAnswer,
    ErrorNote,
}

fn kindToU8(k: StepKind): u8 {
    match (k) {
        StepKind.Thought => return 0,
        StepKind.ToolCall => return 1,
        StepKind.ToolResult => return 2,
        StepKind.FinalAnswer => return 3,
        StepKind.ErrorNote => return 4,
    }
}

fn kindFromU8(b: u8): StepKind? {
    if (b == 0) return StepKind.Thought;
    if (b == 1) return StepKind.ToolCall;
    if (b == 2) return StepKind.ToolResult;
    if (b == 3) return StepKind.FinalAnswer;
    if (b == 4) return StepKind.ErrorNote;
    return null;
}

/// Compact execution trace record stored inside the caller-provided ring buffer.
pub struct TraceEntry {
    step_index: u32,
    kind: StepKind,
    tool_hash: u64,
    tokens_used: u32,
    duration_ms: u32,
    ok: bool,
    timestamp_ms: i64,
}

/// Zero-allocation ReAct execution state machine with fixed ring-buffer step tracing
/// and deterministic binary checkpoint support.
pub struct ReactSession {
    session_id: u64,
    cur_phase: ReactPhase,
    step_count: u32,
    max_steps: u32,
    total_tokens: u64,
    trace_ring: []TraceEntry,
    trace_write: usize,
    trace_count: usize,

    /// Initializes a new `ReactSession` in `Idle` phase backed by `trace_buf`.
    fn init(session_id: u64, max_steps: u32, trace_buf: *[]TraceEntry): ReactSession {
        return ReactSession {
            session_id: session_id,
            cur_phase: ReactPhase.Idle,
            step_count: 0,
            max_steps: max_steps,
            total_tokens: 0,
            trace_ring: trace_buf.*,
            trace_write: 0,
            trace_count: 0,
        };
    }

    /// Returns the current lifecycle phase.
    fn phase(): ReactPhase {
        return _.cur_phase;
    }

    /// Returns the number of tool-call steps executed so far.
    fn stepCount(): u32 {
        return _.step_count;
    }

    /// Returns `true` if the session has reached `Completed`, `Failed`, or `Halted`.
    fn isTerminal(): bool {
        return _.cur_phase == ReactPhase.Completed ||
            _.cur_phase == ReactPhase.Failed ||
            _.cur_phase == ReactPhase.Halted;
    }

    /// Returns the number of retained trace entries currently stored in the ring buffer.
    fn traceLen(): usize {
        return _.trace_count;
    }

    /// Returns the retained trace entry at chronological index `idx` (`0` = oldest retained).
    fn traceAt(idx: usize): TraceEntry? {
        if (idx >= _.trace_count || _.trace_ring.len == 0) {
            return null;
        }
        const oldest = if (_.trace_count < _.trace_ring.len) 0 else _.trace_write;
        const phys = (oldest + idx) % _.trace_ring.len;
        return _.trace_ring[phys];
    }

    fn pushTrace(
        *_,
        kind: StepKind,
        tool_hash: u64,
        tokens_used: u32,
        duration_ms: u32,
        ok: bool,
        timestamp_ms: i64,
    ) {
        if (_.trace_ring.len == 0) {
            return;
        }
        _.trace_ring[_.trace_write] = TraceEntry {
            step_index: _.step_count,
            kind: kind,
            tool_hash: tool_hash,
            tokens_used: tokens_used,
            duration_ms: duration_ms,
            ok: ok,
            timestamp_ms: timestamp_ms,
        };
        _.trace_write = (_.trace_write + 1) % _.trace_ring.len;
        if (_.trace_count < _.trace_ring.len) {
            _.trace_count += 1;
        }
    }

    /// Starts a ReAct turn (`Idle -> Thinking`).
    fn startTurn(*_, now_ms: i64): bool {
        * = now_ms;
        if (_.cur_phase != ReactPhase.Idle) {
            return false;
        }
        _.cur_phase = ReactPhase.Thinking;
        return true;
    }

    /// Records a reasoning thought step (`Thinking -> Thinking` or `Observing -> Thinking`).
    fn recordThought(*_, tokens_used: u32, duration_ms: u32, now_ms: i64): bool {
        if (_.cur_phase != ReactPhase.Thinking && _.cur_phase != ReactPhase.Observing) {
            return false;
        }
        _.cur_phase = ReactPhase.Thinking;
        _.total_tokens = satAddU64(_.total_tokens, tokens_used as u64);
        _.pushTrace(StepKind.Thought, 0, tokens_used, duration_ms, true, now_ms);
        return true;
    }

    /// Requests a tool invocation (`Thinking` or `Observing` -> `AwaitingTool`).
    /// Enforces `max_steps` (`0` = unlimited): if `step_count >= max_steps`, transitions to `Halted` and returns `false`.
    fn requestToolCall(*_, tool_hash: u64, now_ms: i64): bool {
        if (_.cur_phase != ReactPhase.Thinking && _.cur_phase != ReactPhase.Observing) {
            return false;
        }
        if (_.max_steps > 0 && _.step_count >= _.max_steps) {
            _.cur_phase = ReactPhase.Halted;
            _.pushTrace(StepKind.ErrorNote, tool_hash, 0, 0, false, now_ms);
            return false;
        }
        _.step_count += 1;
        _.cur_phase = ReactPhase.AwaitingTool;
        _.pushTrace(StepKind.ToolCall, tool_hash, 0, 0, true, now_ms);
        return true;
    }

    /// Records the result of a pending tool call (`AwaitingTool -> Observing`).
    fn recordToolResult(*_, tool_hash: u64, ok: bool, duration_ms: u32, now_ms: i64): bool {
        if (_.cur_phase != ReactPhase.AwaitingTool) {
            return false;
        }
        _.cur_phase = ReactPhase.Observing;
        _.pushTrace(StepKind.ToolResult, tool_hash, 0, duration_ms, ok, now_ms);
        return true;
    }

    /// Completes the session with a final answer (`Thinking` or `Observing` -> `Completed`).
    fn finishWithAnswer(*_, tokens_used: u32, now_ms: i64): bool {
        if (_.cur_phase != ReactPhase.Thinking && _.cur_phase != ReactPhase.Observing) {
            return false;
        }
        _.total_tokens = satAddU64(_.total_tokens, tokens_used as u64);
        _.cur_phase = ReactPhase.Completed;
        _.pushTrace(StepKind.FinalAnswer, 0, tokens_used, 0, true, now_ms);
        return true;
    }

    /// Transitions any non-terminal session to `Failed`.
    fn failSession(*_, now_ms: i64): bool {
        if (_.isTerminal()) {
            return false;
        }
        _.cur_phase = ReactPhase.Failed;
        _.pushTrace(StepKind.ErrorNote, 0, 0, 0, false, now_ms);
        return true;
    }

    /// Transitions any non-terminal session to `Halted`.
    fn haltSession(*_, now_ms: i64): bool {
        if (_.isTerminal()) {
            return false;
        }
        _.cur_phase = ReactPhase.Halted;
        _.pushTrace(StepKind.ErrorNote, 0, 0, 0, false, now_ms);
        return true;
    }
}

/// Serializes `sess` and all retained `TraceEntry` records in chronological order into `out_buf`
/// using a deterministic little-endian binary format with a trailing 64-bit FNV-1a checksum.
/// Returns the number of bytes written, or `null` if `out_buf` is too small.
pub fn saveCheckpoint(sess: ReactSession, out_buf: *[]u8): usize? {
    const count = sess.traceLen();
    if (count > 0xFFFFFFFF) {
        return null;
    }
    const total_len = CHECKPOINT_HEADER_LEN + count * CHECKPOINT_ENTRY_LEN + CHECKPOINT_FOOTER_LEN;
    if (out_buf.*.len < total_len) {
        return null;
    }

    // Header (36 bytes)
    writeU32LE(out_buf, 0, CHECKPOINT_MAGIC);
    writeU16LE(out_buf, 4, CHECKPOINT_VERSION);
    out_buf.*[6] = phaseToU8(sess.cur_phase);
    out_buf.*[7] = 0;
    writeU64LE(out_buf, 8, sess.session_id);
    writeU32LE(out_buf, 16, sess.step_count);
    writeU32LE(out_buf, 20, sess.max_steps);
    writeU64LE(out_buf, 24, sess.total_tokens);
    writeU32LE(out_buf, 32, count as u32);

    // Entries (30 bytes each, chronological order)
    let i: usize = 0;
    while (i < count) {
        const entry = sess.traceAt(i) ?? return null;
        const off = CHECKPOINT_HEADER_LEN + i * CHECKPOINT_ENTRY_LEN;
        writeU32LE(out_buf, off, entry.step_index);
        out_buf.*[off + 4] = kindToU8(entry.kind);
        out_buf.*[off + 5] = if (entry.ok) 1 else 0;
        writeU64LE(out_buf, off + 6, entry.tool_hash);
        writeU32LE(out_buf, off + 14, entry.tokens_used);
        writeU32LE(out_buf, off + 18, entry.duration_ms);
        writeU64LE(out_buf, off + 22, encodeZigZagI64(entry.timestamp_ms));
        i += 1;
    }

    // Footer: FNV-1a 64-bit checksum over header + entries
    const payload_len = CHECKPOINT_HEADER_LEN + count * CHECKPOINT_ENTRY_LEN;
    const checksum = fnv1a64(out_buf.*[0..payload_len]);
    writeU64LE(out_buf, payload_len, checksum);
    return total_len;
}

/// Validates magic, version, length, and FNV-1a checksum of `data`, and restores a `ReactSession`
/// backed by `trace_buf`. Returns `null` on corruption, truncation, or insufficient `trace_buf`.
pub fn loadCheckpoint(data: []const u8, trace_buf: *[]TraceEntry): ReactSession? {
    const min_len = CHECKPOINT_HEADER_LEN + CHECKPOINT_FOOTER_LEN;
    if (data.len < min_len) {
        return null;
    }

    const magic = readU32LE(data, 0);
    if (magic != CHECKPOINT_MAGIC) {
        return null;
    }
    const version = readU16LE(data, 4);
    if (version != CHECKPOINT_VERSION) {
        return null;
    }
    const phase = phaseFromU8(data[6]) ?? return null;
    if (data[7] != 0) {
        return null;
    }

    const session_id = readU64LE(data, 8);
    const step_count = readU32LE(data, 16);
    const max_steps = readU32LE(data, 20);
    const total_tokens = readU64LE(data, 24);
    const trace_count = readU32LE(data, 32) as usize;

    const payload_len = CHECKPOINT_HEADER_LEN + trace_count * CHECKPOINT_ENTRY_LEN;
    const expected_len = payload_len + CHECKPOINT_FOOTER_LEN;
    if (data.len != expected_len) {
        return null;
    }
    if (trace_buf.*.len < trace_count) {
        return null;
    }

    const expected_checksum = readU64LE(data, payload_len);
    const actual_checksum = fnv1a64(data[0..payload_len]);
    if (actual_checksum != expected_checksum) {
        return null;
    }

    let i: usize = 0;
    while (i < trace_count) {
        const off = CHECKPOINT_HEADER_LEN + i * CHECKPOINT_ENTRY_LEN;
        const step_index = readU32LE(data, off);
        const kind = kindFromU8(data[off + 4]) ?? return null;
        const ok_byte = data[off + 5];
        if (ok_byte > 1) {
            return null;
        }
        const tool_hash = readU64LE(data, off + 6);
        const tokens_used = readU32LE(data, off + 14);
        const duration_ms = readU32LE(data, off + 18);
        const ts_bits = readU64LE(data, off + 22);

        trace_buf.*[i] = TraceEntry {
            step_index: step_index,
            kind: kind,
            tool_hash: tool_hash,
            tokens_used: tokens_used,
            duration_ms: duration_ms,
            ok: ok_byte == 1,
            timestamp_ms: decodeZigZagI64(ts_bits),
        };
        i += 1;
    }

    const write_pos = if (trace_buf.*.len == 0) 0 else (trace_count % trace_buf.*.len);
    return ReactSession {
        session_id: session_id,
        cur_phase: phase,
        step_count: step_count,
        max_steps: max_steps,
        total_tokens: total_tokens,
        trace_ring: trace_buf.*,
        trace_write: write_pos,
        trace_count: trace_count,
    };
}
