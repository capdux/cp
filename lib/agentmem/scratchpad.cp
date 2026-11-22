// Origin: Pure-CP Stdlib (Zero-Alloc Agent ReAct / Plan-Execute Structured Scratchpad & TAO Ledger) | Created: 2026-11-23
// Pure CP zero-heap-allocation structured reasoning scratchpad and Thought-Action-Observation ledger:
//   - Fixed 64-step ReAct / Plan-Execute trajectory ledger with monotonic step IDs & parent links
//   - Fixed 8-slot checkpoint table (`saveCheckpoint` / `rollbackToCheckpoint`) supporting safe
//     branch rollback on failed reasoning paths while preserving cumulative rollback telemetry
//   - Sliding-window repeated action loop detector (`detectRepeatedAction`) to prevent tool loops
//   - Token-budget-aware prompt renderer (`renderWithinTokenBudget`):
//       * Prioritizes all `pinned` and `milestone` steps first
//       * Backfills recent steps from newest to oldest within remaining `max_tokens`
//       * Emits chronological compact `[Thought]` / `[Action]` / `[Observation]` / `[Reflection]`
//         / `[Milestone]` / `[Error]` prompt lines with automatic omission summary markers
//   - Zero-heap telemetry snapshot (`ScratchpadStats`) and clean full reset (`clear`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed maximum number of active reasoning steps stored in `AgentScratchpad`.
pub const SCRATCHPAD_MAX_STEPS: usize = 64;

/// Fixed maximum number of saved rollback checkpoints in `AgentScratchpad`.
pub const SCRATCHPAD_MAX_CHECKPOINTS: usize = 8;

/// Maximum inline UTF-8 byte length of a step's content payload.
pub const SCRATCHPAD_TEXT_MAX_LEN: usize = 192;

/// Maximum inline UTF-8 byte length of a step tag or checkpoint label.
pub const SCRATCHPAD_TAG_MAX_LEN: usize = 40;

/// Sentinel step ID indicating no parent / no valid step.
pub const SCRATCHPAD_NO_STEP: u32 = 0;

/// Semantic category of a single step in an Agent ReAct / Plan-Execute trace.
pub enum ScratchStepKind {
    thought,
    action,
    observation,
    reflection,
    milestone,
    step_error,
}

/// Single structured Thought-Action-Observation step stored inline without heap allocation.
pub struct ScratchStep {
    active: bool = false;
    step_id: u32 = 0;
    parent_step_id: u32 = 0;
    kind: ScratchStepKind = ScratchStepKind.thought;
    token_estimate: u32 = 0;
    pinned: bool = false;
    timestamp_ms: u64 = 0;
    tag_buf: [SCRATCHPAD_TAG_MAX_LEN]u8 = [0; SCRATCHPAD_TAG_MAX_LEN];
    tag_len: u16 = 0;
    tag_truncated: bool = false;
    content_buf: [SCRATCHPAD_TEXT_MAX_LEN]u8 = [0; SCRATCHPAD_TEXT_MAX_LEN];
    content_len: u16 = 0;
    content_truncated: bool = false;

    /// Return the stored tag slice (e.g. tool name or phase label).
    fn tag(): []const u8 {
        return tag_buf[0..@as(usize, tag_len)];
    }

    /// Return the stored step content text slice.
    fn content(): []const u8 {
        return content_buf[0..@as(usize, content_len)];
    }

    /// Return true if this step receives priority retention during budget rendering.
    fn isPriority(): bool {
        return pinned || kind == ScratchStepKind.milestone;
    }
}

/// Saved rollback checkpoint snapshot inside `AgentScratchpad`.
pub struct ScratchCheckpoint {
    active: bool = false;
    checkpoint_id: u32 = 0;
    step_count: usize = 0;
    last_step_id: u32 = 0;
    label_buf: [SCRATCHPAD_TAG_MAX_LEN]u8 = [0; SCRATCHPAD_TAG_MAX_LEN];
    label_len: u16 = 0;

    /// Return the checkpoint label slice.
    fn label(): []const u8 {
        return label_buf[0..@as(usize, label_len)];
    }
}

/// Zero-heap global telemetry snapshot for `AgentScratchpad`.
pub struct ScratchpadStats {
    active_steps: usize = 0;
    capacity: usize = SCRATCHPAD_MAX_STEPS;
    checkpoint_count: usize = 0;
    pinned_steps: usize = 0;
    milestone_steps: usize = 0;
    error_steps: usize = 0;
    total_tokens: u64 = 0;
    total_appended: u64 = 0;
    total_rollbacks: u64 = 0;
    total_rolled_back_steps: u64 = 0;
}

/// Summary report returned by `AgentScratchpad.renderWithinTokenBudget`.
pub struct ScratchRenderReport {
    bytes_written: usize = 0;
    rendered_steps: usize = 0;
    omitted_steps: usize = 0;
    pinned_rendered: usize = 0;
    milestone_rendered: usize = 0;
    tokens_used: u64 = 0;
    truncated_by_budget: bool = false;
    truncated_by_buffer: bool = false;
}

// ============================================================================
// 2. Pure Helper Functions
// ============================================================================

/// Format a `ScratchStepKind` variant as its canonical prompt bracket label.
pub fn formatScratchStepKind(kind: ScratchStepKind): []const u8 {
    if (kind == ScratchStepKind.thought) return "Thought";
    if (kind == ScratchStepKind.action) return "Action";
    if (kind == ScratchStepKind.observation) return "Observation";
    if (kind == ScratchStepKind.reflection) return "Reflection";
    if (kind == ScratchStepKind.milestone) return "Milestone";
    return "Error";
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (pos + n > out_buf.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    pos += n;
    return true;
}

fn formatOmissionLine(omitted_count: usize, out_buf: *[]u8, pos: *usize): bool {
    let cur: usize = pos + 0;
    if (!appendBytes(out_buf, *cur, "... (")) return false;
    if (!appendU64(out_buf, *cur, @as(u64, omitted_count))) return false;
    const suffix: []const u8 = if (omitted_count == 1) " step omitted)\n" else " steps omitted)\n";
    if (!appendBytes(out_buf, *cur, suffix)) return false;
    pos = cur;
    return true;
}

fn formatStepLine(step: ScratchStep, out_buf: *[]u8, pos: *usize): bool {
    let cur: usize = pos + 0;
    if (!appendBytes(out_buf, *cur, "[")) return false;
    if (!appendBytes(out_buf, *cur, formatScratchStepKind(step.kind))) return false;
    const tag_len: usize = @as(usize, step.tag_len);
    if (tag_len > 0) {
        if (!appendBytes(out_buf, *cur, ":")) return false;
        if (!appendBytes(out_buf, *cur, step.tag_buf[0..tag_len])) return false;
    }
    if (!appendBytes(out_buf, *cur, "] ")) return false;
    const content_len: usize = @as(usize, step.content_len);
    if (content_len > 0) {
        if (!appendBytes(out_buf, *cur, step.content_buf[0..content_len])) return false;
    }
    if (!appendBytes(out_buf, *cur, "\n")) return false;
    pos = cur;
    return true;
}

// ============================================================================
// 3. AgentScratchpad — Zero-Heap Structured ReAct / Plan-Execute Ledger
// ============================================================================

/// Zero-heap-allocation structured reasoning scratchpad & TAO ledger for AI Agents.
pub struct AgentScratchpad {
    steps: [SCRATCHPAD_MAX_STEPS]ScratchStep = [ScratchStep{}; SCRATCHPAD_MAX_STEPS];
    step_count: usize = 0;
    checkpoints: [SCRATCHPAD_MAX_CHECKPOINTS]ScratchCheckpoint = [ScratchCheckpoint{}; SCRATCHPAD_MAX_CHECKPOINTS];
    checkpoint_count: usize = 0;
    next_step_id: u32 = 1;
    next_checkpoint_id: u32 = 1;
    total_appended: u64 = 0;
    total_rollbacks: u64 = 0;
    total_rolled_back_steps: u64 = 0;

    /// Initialize an empty `AgentScratchpad`.
    static fn init(): AgentScratchpad {
        return AgentScratchpad{};
    }

    /// Return the current number of active steps in the scratchpad.
    fn len(): usize {
        return step_count;
    }

    /// Append a new reasoning step to the scratchpad.
    /// Automatically links `parent_step_id` to the previous active step (or `0` if first).
    /// Truncates `tag` / `content` exceeding inline buffer bounds without failing.
    /// Returns the assigned monotonic `step_id`, or `null` if the 64-step ledger is full.
    fn appendStep(
        *_,
        kind: ScratchStepKind,
        tag: []const u8,
        content: []const u8,
        token_estimate: u32,
        timestamp_ms: u64,
    ): u32? {
        if (step_count >= SCRATCHPAD_MAX_STEPS) return null;

        let st = ScratchStep{};
        st.active = true;
        st.step_id = next_step_id;
        st.parent_step_id = if (step_count > 0) steps[step_count - 1].step_id else SCRATCHPAD_NO_STEP;
        st.kind = kind;
        st.token_estimate = token_estimate;
        st.pinned = false;
        st.timestamp_ms = timestamp_ms;

        const copy_tag: usize = if (tag.len > SCRATCHPAD_TAG_MAX_LEN) SCRATCHPAD_TAG_MAX_LEN else tag.len;
        let i: usize = 0;
        while (i < copy_tag) : (i += 1) {
            st.tag_buf[i] = tag[i];
        }
        st.tag_len = @as(u16, @intCast(copy_tag));
        st.tag_truncated = tag.len > SCRATCHPAD_TAG_MAX_LEN;

        const copy_content: usize = if (content.len > SCRATCHPAD_TEXT_MAX_LEN) SCRATCHPAD_TEXT_MAX_LEN else content.len;
        let j: usize = 0;
        while (j < copy_content) : (j += 1) {
            st.content_buf[j] = content[j];
        }
        st.content_len = @as(u16, @intCast(copy_content));
        st.content_truncated = content.len > SCRATCHPAD_TEXT_MAX_LEN;

        steps[step_count] = st;
        step_count += 1;
        next_step_id += 1;
        total_appended += 1;

        return st.step_id;
    }

    /// Pin or unpin an existing active step by `step_id`.
    /// Pinned steps are always prioritized during `renderWithinTokenBudget`.
    /// Returns `true` if the step was found and updated, `false` otherwise.
    fn pinStep(*_, step_id: u32, pinned: bool): bool {
        if (step_id == SCRATCHPAD_NO_STEP) return false;
        let i: usize = 0;
        while (i < step_count) : (i += 1) {
            if (steps[i].active && steps[i].step_id == step_id) {
                steps[i].pinned = pinned;
                return true;
            }
        }
        return false;
    }

    /// Look up an active step by `step_id`. Returns `null` if not found or rolled back.
    fn getStep(step_id: u32): ScratchStep? {
        if (step_id == SCRATCHPAD_NO_STEP) return null;
        let i: usize = 0;
        while (i < step_count) : (i += 1) {
            if (steps[i].active && steps[i].step_id == step_id) {
                return steps[i];
            }
        }
        return null;
    }

    /// Look up an active step by 0-based chronological index.
    fn getStepAt(idx: usize): ScratchStep? {
        if (idx >= step_count) return null;
        if (!steps[idx].active) return null;
        return steps[idx];
    }

    /// Save a rollback checkpoint at the current trajectory depth.
    /// Returns the assigned monotonic `checkpoint_id`, or `null` if all 8 checkpoint slots are full.
    fn saveCheckpoint(*_, label: []const u8): u32? {
        if (checkpoint_count >= SCRATCHPAD_MAX_CHECKPOINTS) return null;

        let cp = ScratchCheckpoint{};
        cp.active = true;
        cp.checkpoint_id = next_checkpoint_id;
        cp.step_count = step_count;
        cp.last_step_id = if (step_count > 0) steps[step_count - 1].step_id else SCRATCHPAD_NO_STEP;

        const copy_len: usize = if (label.len > SCRATCHPAD_TAG_MAX_LEN) SCRATCHPAD_TAG_MAX_LEN else label.len;
        let i: usize = 0;
        while (i < copy_len) : (i += 1) {
            cp.label_buf[i] = label[i];
        }
        cp.label_len = @as(u16, @intCast(copy_len));

        checkpoints[checkpoint_count] = cp;
        checkpoint_count += 1;
        next_checkpoint_id += 1;

        return cp.checkpoint_id;
    }

    /// Look up an active checkpoint by `checkpoint_id`.
    fn getCheckpoint(checkpoint_id: u32): ScratchCheckpoint? {
        if (checkpoint_id == 0) return null;
        let i: usize = 0;
        while (i < checkpoint_count) : (i += 1) {
            if (checkpoints[i].active && checkpoints[i].checkpoint_id == checkpoint_id) {
                return checkpoints[i];
            }
        }
        return null;
    }

    /// Roll back the scratchpad trajectory to a previously saved checkpoint.
    /// Drops all steps appended after the checkpoint and invalidates any checkpoints
    /// created after it, while preserving the target checkpoint itself for repeated retries
    /// and updating cumulative rollback telemetry counters.
    /// Returns the number of steps removed (or `0` if `checkpoint_id` is not found).
    fn rollbackToCheckpoint(*_, checkpoint_id: u32): usize {
        if (checkpoint_id == 0) return 0;

        let found = false;
        let cp_idx: usize = 0;
        let i: usize = 0;
        while (i < checkpoint_count) : (i += 1) {
            if (checkpoints[i].active && checkpoints[i].checkpoint_id == checkpoint_id) {
                found = true;
                cp_idx = i;
                break;
            }
        }
        if (!found) return 0;

        const target_count = checkpoints[cp_idx].step_count;
        if (target_count > step_count) return 0;

        const removed: usize = step_count - target_count;
        let s: usize = target_count;
        while (s < step_count) : (s += 1) {
            steps[s] = ScratchStep{};
        }
        step_count = target_count;

        // Invalidate any nested checkpoints that belonged to the rolled-back branch.
        let c: usize = cp_idx + 1;
        while (c < checkpoint_count) : (c += 1) {
            checkpoints[c] = ScratchCheckpoint{};
        }
        checkpoint_count = cp_idx + 1;

        total_rollbacks += 1;
        total_rolled_back_steps += @as(u64, removed);

        return removed;
    }

    /// Count how many `action` steps with matching `tag` occurred within the most recent
    /// `window_size` steps of the active trajectory. Aids anti-loop / stall detection.
    fn detectRepeatedAction(tag: []const u8, window_size: usize): usize {
        if (window_size == 0 || tag.len == 0 || step_count == 0) return 0;

        const cmp_len: usize = if (tag.len > SCRATCHPAD_TAG_MAX_LEN) SCRATCHPAD_TAG_MAX_LEN else tag.len;
        const target_tag: []const u8 = tag[0..cmp_len];

        const start: usize = if (step_count > window_size) step_count - window_size else 0;
        let count: usize = 0;
        let i: usize = start;
        while (i < step_count) : (i += 1) {
            if (steps[i].active && steps[i].kind == ScratchStepKind.action) {
                const st_tag: []const u8 = steps[i].tag_buf[0..@as(usize, steps[i].tag_len)];
                if (bytesEqual(st_tag, target_tag)) {
                    count += 1;
                }
            }
        }
        return count;
    }

    /// Render active scratchpad steps into `out_buf` within `max_tokens`:
    ///   1. Priority pass: select all `pinned` and `milestone` steps that fit in `max_tokens`.
    ///   2. Recency pass: backfill remaining steps from newest to oldest within leftover budget.
    ///   3. Emit selected steps in chronological order as compact `[Kind]` / `[Kind:tag]` lines,
    ///      inserting `... (N steps omitted)` markers wherever older steps were dropped.
    fn renderWithinTokenBudget(max_tokens: u64, out_buf: *[]u8): ScratchRenderReport {
        let rep = ScratchRenderReport{};
        if (step_count == 0) return rep;

        let selected: [SCRATCHPAD_MAX_STEPS]bool = [false; SCRATCHPAD_MAX_STEPS];
        let budget_used: u64 = 0;

        // Pass 1: Prioritize all pinned and milestone steps (chronological order).
        let i: usize = 0;
        while (i < step_count) : (i += 1) {
            if (!steps[i].active) continue;
            if (steps[i].pinned || steps[i].kind == ScratchStepKind.milestone) {
                const tok: u64 = @as(u64, steps[i].token_estimate);
                if (budget_used + tok <= max_tokens) {
                    selected[i] = true;
                    budget_used += tok;
                }
            }
        }

        // Pass 2: Backfill non-priority steps from newest to oldest within remaining token budget.
        let r: usize = step_count;
        while (r > 0) {
            r -= 1;
            if (!steps[r].active || selected[r]) continue;
            const tok: u64 = @as(u64, steps[r].token_estimate);
            if (budget_used + tok <= max_tokens) {
                selected[r] = true;
                budget_used += tok;
            }
        }

        // Pass 3: Render in chronological order with contiguous omission summaries.
        let pos: usize = 0;
        let omitted_run: usize = 0;
        let idx: usize = 0;
        while (idx < step_count) : (idx += 1) {
            if (!steps[idx].active) continue;

            if (!selected[idx]) {
                omitted_run += 1;
                rep.omitted_steps += 1;
                rep.truncated_by_budget = true;
                continue;
            }

            if (omitted_run > 0) {
                if (!formatOmissionLine(omitted_run, out_buf, *pos)) {
                    rep.truncated_by_buffer = true;
                    break;
                }
                omitted_run = 0;
            }

            if (!formatStepLine(steps[idx], out_buf, *pos)) {
                rep.truncated_by_buffer = true;
                break;
            }

            rep.rendered_steps += 1;
            if (steps[idx].pinned) rep.pinned_rendered += 1;
            if (steps[idx].kind == ScratchStepKind.milestone) rep.milestone_rendered += 1;
            rep.tokens_used += @as(u64, steps[idx].token_estimate);
        }

        if (!rep.truncated_by_buffer && omitted_run > 0) {
            if (!formatOmissionLine(omitted_run, out_buf, *pos)) {
                rep.truncated_by_buffer = true;
            }
        }

        rep.bytes_written = pos;
        return rep;
    }

    /// Return a zero-heap telemetry snapshot of the scratchpad state.
    fn stats(): ScratchpadStats {
        let st = ScratchpadStats{};
        st.active_steps = step_count;
        st.capacity = SCRATCHPAD_MAX_STEPS;
        st.checkpoint_count = checkpoint_count;
        st.total_appended = total_appended;
        st.total_rollbacks = total_rollbacks;
        st.total_rolled_back_steps = total_rolled_back_steps;

        let i: usize = 0;
        while (i < step_count) : (i += 1) {
            if (!steps[i].active) continue;
            if (steps[i].pinned) st.pinned_steps += 1;
            if (steps[i].kind == ScratchStepKind.milestone) st.milestone_steps += 1;
            if (steps[i].kind == ScratchStepKind.step_error) st.error_steps += 1;
            st.total_tokens += @as(u64, steps[i].token_estimate);
        }
        return st;
    }

    /// Reset all active steps, checkpoints, IDs, and telemetry counters.
    fn clear(*_) {
        let i: usize = 0;
        while (i < SCRATCHPAD_MAX_STEPS) : (i += 1) {
            steps[i] = ScratchStep{};
        }
        step_count = 0;

        let c: usize = 0;
        while (c < SCRATCHPAD_MAX_CHECKPOINTS) : (c += 1) {
            checkpoints[c] = ScratchCheckpoint{};
        }
        checkpoint_count = 0;

        next_step_id = 1;
        next_checkpoint_id = 1;
        total_appended = 0;
        total_rollbacks = 0;
        total_rolled_back_steps = 0;
    }
}
