// Origin: Pure-CP Stdlib (Zero-Alloc Agent Tool-Output Compactor & Reference Store) | Created: 2026-11-23
// Pure CP zero-heap-allocation AI Agent Tool-Output Compactor & Reference Handle Store:
//   - Fixed 64-entry tool invocation output table tracking `(ref_id, tool_name, status,
//     original_bytes, original_tokens, active_tokens, content_hash, is_error, preview)`
//   - Budget-driven automatic compaction (`compactToBudget`):
//       * Always protects `pinned` entries, the most recent `keep_recent_full` calls,
//         and optionally `keep_errors_full` error outputs from compaction
//       * Replaces oldest compactable full entries with compact reference stubs
//         (e.g. `[ref#12: read_file, 4096B, ok]`) and accurately reports tokens freed
//   - Reference lookup, pinning lifecycle (`pinRef` / `unpinRef`) & zero-alloc rendering
//     (`renderEntry` / `renderAll`)
//   - Zero-heap global telemetry snapshot (`ToolHistoryStats`) including active/original/saved
//     tokens, compression ratio in basis points (`savings_bps`), and status breakdown

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the zero-heap tool output history table.
pub const TOOL_HISTORY_MAX_ENTRIES: usize = 64;

/// Maximum length in bytes of a stored tool name (longer names are truncated).
pub const TOOL_HISTORY_NAME_MAX_LEN: usize = 32;

/// Maximum length in bytes of an inline preview buffer per tool output.
pub const TOOL_HISTORY_PREVIEW_MAX_LEN: usize = 256;

/// Sentinel reference ID indicating no valid entry.
pub const TOOL_HISTORY_NO_REF: u64 = 0;

/// Default estimated token cost of a compacted reference stub.
pub const TOOL_HISTORY_DEFAULT_STUB_TOKENS: u32 = 12;

/// Basis-point scale (10000 = 100.00%) used for `savings_bps`.
pub const TOOL_HISTORY_BPS_SCALE: u32 = 10_000;

/// Lifecycle state of a stored tool invocation output.
pub enum ToolOutputStatus {
    full,
    compacted,
    pinned,
}

/// Single tool invocation output record stored in the fixed-capacity table.
pub struct ToolOutputEntry {
    active: bool = false;
    ref_id: u64 = 0;
    seq: u64 = 0;
    status: ToolOutputStatus = ToolOutputStatus.full;
    tool_name_buf: [TOOL_HISTORY_NAME_MAX_LEN]u8 = [0; TOOL_HISTORY_NAME_MAX_LEN];
    tool_name_len: u16 = 0;
    original_bytes: u32 = 0;
    original_tokens: u32 = 0;
    active_tokens: u32 = 0;
    content_hash: u64 = 0;
    is_error: bool = false;
    preview_truncated: bool = false;
    preview_buf: [TOOL_HISTORY_PREVIEW_MAX_LEN]u8 = [0; TOOL_HISTORY_PREVIEW_MAX_LEN];
    preview_len: u16 = 0;

    /// Return the stored tool name slice.
    fn toolName(): []const u8 {
        return tool_name_buf[0..@as(usize, tool_name_len)];
    }

    /// Return the inline preview or compacted reference stub slice.
    fn preview(): []const u8 {
        return preview_buf[0..@as(usize, preview_len)];
    }

    /// Return the number of tokens saved by compacting this entry.
    fn savedTokens(): u32 {
        if (original_tokens > active_tokens) {
            return original_tokens - active_tokens;
        }
        return 0;
    }

    /// Return true if this entry is pinned against compaction and eviction.
    fn isPinned(): bool {
        return status == ToolOutputStatus.pinned;
    }

    /// Return true if this entry has been compacted into a reference stub.
    fn isCompacted(): bool {
        return status == ToolOutputStatus.compacted;
    }
}

/// Tunable compaction & protection policy for `ToolHistoryStore`.
pub struct ToolHistoryConfig {
    /// Number of most recent active tool outputs always protected from compaction.
    keep_recent_full: usize = 4;
    /// When true, error outputs (`is_error == true`) are protected from compaction.
    keep_errors_full: bool = true;
    /// Estimated token cost assigned to a compacted reference stub.
    stub_tokens: u32 = TOOL_HISTORY_DEFAULT_STUB_TOKENS;

    /// Return production-balanced default configuration.
    static fn defaults(): ToolHistoryConfig {
        return ToolHistoryConfig{};
    }
}

/// Result summary returned by `ToolHistoryStore.compactToBudget`.
pub struct ToolCompactReport {
    compacted_count: u32 = 0;
    tokens_freed: u64 = 0;
    active_tokens_before: u64 = 0;
    active_tokens_after: u64 = 0;
    within_budget: bool = true;
}

/// Zero-heap global telemetry snapshot for `ToolHistoryStore`.
pub struct ToolHistoryStats {
    total_entries: usize = 0;
    capacity: usize = TOOL_HISTORY_MAX_ENTRIES;
    full_count: usize = 0;
    compacted_count: usize = 0;
    pinned_count: usize = 0;
    error_count: usize = 0;
    active_tokens: u64 = 0;
    original_tokens_total: u64 = 0;
    saved_tokens: u64 = 0;
    savings_bps: u32 = 0;
    recorded_total: u64 = 0;
    evicted_total: u64 = 0;
}

// ============================================================================
// 2. Pure Helper Functions
// ============================================================================

/// Compute deterministic 64-bit FNV-1a hash of a tool output payload.
pub fn hashToolPayload(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = h ^ @as(u64, data[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

/// Estimate token count for a UTF-8 tool payload (~4 bytes/token, rounded up).
pub fn estimateToolTokens(payload: []const u8): u32 {
    if (payload.len == 0) return 0;
    const est: usize = (payload.len + 3) / 4;
    if (est > 0xFFFFFFFF) return 0xFFFFFFFF;
    return @as(u32, @intCast(est));
}

/// Format a `ToolOutputStatus` variant as a canonical lowercase ASCII label.
pub fn formatToolStatus(status: ToolOutputStatus): []const u8 {
    if (status == ToolOutputStatus.full) return "full";
    if (status == ToolOutputStatus.compacted) return "compacted";
    return "pinned";
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

fn formatStubInto(
    ref_id: u64,
    tool_name: []const u8,
    original_bytes: u32,
    is_error: bool,
    out_buf: *[]u8,
    pos: *usize,
): bool {
    if (!appendBytes(out_buf, pos, "[ref#")) return false;
    if (!appendU64(out_buf, pos, ref_id)) return false;
    if (!appendBytes(out_buf, pos, ": ")) return false;
    if (tool_name.len == 0) {
        if (!appendBytes(out_buf, pos, "tool")) return false;
    } else {
        if (!appendBytes(out_buf, pos, tool_name)) return false;
    }
    if (!appendBytes(out_buf, pos, ", ")) return false;
    if (!appendU64(out_buf, pos, @as(u64, original_bytes))) return false;
    if (is_error) {
        if (!appendBytes(out_buf, pos, "B, err]")) return false;
    } else {
        if (!appendBytes(out_buf, pos, "B, ok]")) return false;
    }
    return true;
}

// ============================================================================
// 3. ToolHistoryStore — Zero-Heap Tool-Output Compactor & Reference Store
// ============================================================================

/// Zero-heap-allocation agent tool-output compactor and reference handle store.
pub struct ToolHistoryStore {
    cfg: ToolHistoryConfig = ToolHistoryConfig{},
    entries: [TOOL_HISTORY_MAX_ENTRIES]ToolOutputEntry = [ToolOutputEntry{}; TOOL_HISTORY_MAX_ENTRIES];
    entry_count: usize = 0;
    next_ref_id: u64 = 1;
    next_seq: u64 = 1;
    recorded_total: u64 = 0;
    evicted_total: u64 = 0;

    /// Initialize a new `ToolHistoryStore` with default compaction policy.
    static fn init(): ToolHistoryStore {
        return ToolHistoryStore{};
    }

    /// Initialize a new `ToolHistoryStore` with custom compaction policy.
    static fn initWithConfig(cfg: ToolHistoryConfig): ToolHistoryStore {
        let store = ToolHistoryStore{};
        store.cfg = cfg;
        return store;
    }

    /// Clear all stored entries and reset counters while preserving configuration.
    fn reset(*_) {
        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            entries[i] = ToolOutputEntry{};
        }
        entry_count = 0;
        next_ref_id = 1;
        next_seq = 1;
        recorded_total = 0;
        evicted_total = 0;
    }

    /// Return current number of active entries stored in the table.
    fn len(): usize {
        return entry_count;
    }

    /// Compute total active tokens currently occupied by all stored entries.
    fn activeTokens(): u64 {
        let total: u64 = 0;
        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active) {
                total += @as(u64, entries[i].active_tokens);
            }
        }
        return total;
    }

    /// Find internal slot index by `ref_id`.
    fn findSlot(ref_id: u64): usize? {
        if (ref_id == TOOL_HISTORY_NO_REF) return null;
        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active && entries[i].ref_id == ref_id) {
                return i;
            }
        }
        return null;
    }

    /// Select a slot for insertion: first free slot, otherwise oldest compacted entry,
    /// otherwise oldest non-pinned full entry. Returns `null` if all 64 slots are pinned.
    fn selectInsertSlot(*_): usize? {
        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) {
                return i;
            }
        }

        // Table is full: prefer evicting oldest compacted entry first.
        let found_compacted = false;
        let best_compacted_idx: usize = 0;
        let best_compacted_seq: u64 = 0;

        let found_full = false;
        let best_full_idx: usize = 0;
        let best_full_seq: u64 = 0;

        let j: usize = 0;
        while (j < TOOL_HISTORY_MAX_ENTRIES) : (j += 1) {
            if (!entries[j].active) continue;
            if (entries[j].status == ToolOutputStatus.compacted) {
                if (!found_compacted || entries[j].seq < best_compacted_seq) {
                    found_compacted = true;
                    best_compacted_idx = j;
                    best_compacted_seq = entries[j].seq;
                }
            } else if (entries[j].status == ToolOutputStatus.full) {
                if (!found_full || entries[j].seq < best_full_seq) {
                    found_full = true;
                    best_full_idx = j;
                    best_full_seq = entries[j].seq;
                }
            }
        }

        if (found_compacted) {
            entries[best_compacted_idx].active = false;
            if (entry_count > 0) entry_count -= 1;
            evicted_total += 1;
            return best_compacted_idx;
        }
        if (found_full) {
            entries[best_full_idx].active = false;
            if (entry_count > 0) entry_count -= 1;
            evicted_total += 1;
            return best_full_idx;
        }
        return null;
    }

    /// Record a new tool invocation output and return its monotonic `ref_id`.
    /// Returns `null` only if all 64 slots are occupied by `pinned` entries.
    fn recordOutput(
        *_,
        tool_name: []const u8,
        payload: []const u8,
        token_count: u32,
        is_error: bool,
        pin: bool,
    ): u64? {
        const slot_opt = selectInsertSlot();
        const slot = slot_opt ?? return null;

        let e = ToolOutputEntry{};
        e.active = true;
        e.ref_id = next_ref_id;
        e.seq = next_seq;
        e.status = if (pin) ToolOutputStatus.pinned else ToolOutputStatus.full;
        e.original_bytes = if (payload.len > 0xFFFFFFFF) 0xFFFFFFFF else @as(u32, @intCast(payload.len));
        e.original_tokens = token_count;
        e.active_tokens = token_count;
        e.content_hash = hashToolPayload(payload);
        e.is_error = is_error;

        // Copy tool name (bounded to TOOL_HISTORY_NAME_MAX_LEN).
        const copy_name_len: usize = if (tool_name.len > TOOL_HISTORY_NAME_MAX_LEN) TOOL_HISTORY_NAME_MAX_LEN else tool_name.len;
        let i: usize = 0;
        while (i < copy_name_len) : (i += 1) {
            e.tool_name_buf[i] = tool_name[i];
        }
        e.tool_name_len = @as(u16, @intCast(copy_name_len));

        // Copy inline preview (bounded to TOOL_HISTORY_PREVIEW_MAX_LEN).
        const copy_prev_len: usize = if (payload.len > TOOL_HISTORY_PREVIEW_MAX_LEN) TOOL_HISTORY_PREVIEW_MAX_LEN else payload.len;
        i = 0;
        while (i < copy_prev_len) : (i += 1) {
            e.preview_buf[i] = payload[i];
        }
        e.preview_len = @as(u16, @intCast(copy_prev_len));
        e.preview_truncated = payload.len > TOOL_HISTORY_PREVIEW_MAX_LEN;

        entries[slot] = e;
        entry_count += 1;
        next_ref_id += 1;
        next_seq += 1;
        recorded_total += 1;
        return e.ref_id;
    }

    /// Count how many active entries have a strictly newer `seq` than `target_seq`.
    fn countNewerActive(target_seq: u64): usize {
        let newer: usize = 0;
        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active && entries[i].seq > target_seq) {
                newer += 1;
            }
        }
        return newer;
    }

    /// Compact oldest eligible `full` entries into compact reference stubs until
    /// total active tokens <= `max_active_tokens` or no eligible entries remain.
    /// Always preserves `pinned` entries, the most recent `keep_recent_full` entries,
    /// and (when `keep_errors_full` is enabled) error outputs.
    fn compactToBudget(*_, max_active_tokens: u64): ToolCompactReport {
        const before = activeTokens();
        let current = before;
        let compacted: u32 = 0;
        let freed_total: u64 = 0;

        while (current > max_active_tokens) {
            let found = false;
            let victim_idx: usize = 0;
            let victim_seq: u64 = 0;
            let victim_new_tokens: u32 = 0;

            let i: usize = 0;
            while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
                if (!entries[i].active) continue;
                if (entries[i].status != ToolOutputStatus.full) continue;
                if (cfg.keep_errors_full && entries[i].is_error) continue;

                // Protect the most recent `keep_recent_full` active calls.
                if (countNewerActive(entries[i].seq) < cfg.keep_recent_full) continue;

                const new_tok: u32 = if (cfg.stub_tokens < entries[i].original_tokens) cfg.stub_tokens else entries[i].original_tokens;
                if (entries[i].active_tokens <= new_tok) continue;

                if (!found || entries[i].seq < victim_seq) {
                    found = true;
                    victim_idx = i;
                    victim_seq = entries[i].seq;
                    victim_new_tokens = new_tok;
                }
            }

            if (!found) break;

            const saved: u64 = @as(u64, entries[victim_idx].active_tokens - victim_new_tokens);
            entries[victim_idx].status = ToolOutputStatus.compacted;
            entries[victim_idx].active_tokens = victim_new_tokens;
            entries[victim_idx].preview_truncated = false;

            // Overwrite inline preview with the compact reference stub.
            let stub_buf: [TOOL_HISTORY_PREVIEW_MAX_LEN]u8 = [0; TOOL_HISTORY_PREVIEW_MAX_LEN];
            let stub_s: []u8 = stub_buf[0..];
            let stub_pos: usize = 0;
            const name_len: usize = @as(usize, entries[victim_idx].tool_name_len);
            if (formatStubInto(
                entries[victim_idx].ref_id,
                entries[victim_idx].tool_name_buf[0..name_len],
                entries[victim_idx].original_bytes,
                entries[victim_idx].is_error,
                *stub_s,
                *stub_pos,
            )) {
                let k: usize = 0;
                while (k < stub_pos) : (k += 1) {
                    entries[victim_idx].preview_buf[k] = stub_buf[k];
                }
                entries[victim_idx].preview_len = @as(u16, @intCast(stub_pos));
            } else {
                entries[victim_idx].preview_len = 0;
            }

            compacted += 1;
            freed_total += saved;
            current -= saved;
        }

        return ToolCompactReport{
            compacted_count: compacted,
            tokens_freed: freed_total,
            active_tokens_before: before,
            active_tokens_after: current,
            within_budget: current <= max_active_tokens,
        };
    }

    /// Retrieve a copy of a stored entry by `ref_id`.
    fn getEntry(ref_id: u64): ToolOutputEntry? {
        if (idx := findSlot(ref_id)) {
            return entries[idx];
        }
        return null;
    }

    /// Pin an existing uncompacted entry so it is protected from compaction and eviction.
    /// Returns `false` if `ref_id` does not exist or has already been compacted.
    fn pinRef(*_, ref_id: u64): bool {
        if (idx := findSlot(ref_id)) {
            if (entries[idx].status == ToolOutputStatus.compacted) return false;
            entries[idx].status = ToolOutputStatus.pinned;
            return true;
        }
        return false;
    }

    /// Unpin a previously pinned entry back to `full` status so it can be compacted if needed.
    /// Returns `false` if `ref_id` does not exist or is not currently pinned.
    fn unpinRef(*_, ref_id: u64): bool {
        if (idx := findSlot(ref_id)) {
            if (entries[idx].status != ToolOutputStatus.pinned) return false;
            entries[idx].status = ToolOutputStatus.full;
            return true;
        }
        return false;
    }

    /// Remove an entry by `ref_id`. Returns `true` if removed.
    fn removeRef(*_, ref_id: u64): bool {
        if (idx := findSlot(ref_id)) {
            entries[idx] = ToolOutputEntry{};
            if (entry_count > 0) entry_count -= 1;
            return true;
        }
        return false;
    }

    /// Zero-allocation context rendering of a single tool output entry into `out_buf`.
    /// - Compacted entries render as `[ref#<id>: <tool>, <bytes>B, ok|err]`
    /// - Full entries render as `[ref#<id>: <tool>, <bytes>B, ok|err] <preview>[...]`
    /// - Pinned entries render as `[ref#<id>: <tool>, <bytes>B, ok|err, pinned] <preview>[...]`
    /// Returns the number of bytes written, or `null` if `ref_id` is missing or `out_buf` is too small.
    fn renderEntry(ref_id: u64, out_buf: *[]u8): usize? {
        const idx = findSlot(ref_id) ?? return null;
        let pos: usize = 0;
        const name_len: usize = @as(usize, entries[idx].tool_name_len);
        const tool_name: []const u8 = entries[idx].tool_name_buf[0..name_len];

        if (entries[idx].status == ToolOutputStatus.compacted) {
            if (!formatStubInto(
                entries[idx].ref_id,
                tool_name,
                entries[idx].original_bytes,
                entries[idx].is_error,
                out_buf,
                *pos,
            )) {
                return null;
            }
            return pos;
        }

        if (!appendBytes(out_buf, *pos, "[ref#")) return null;
        if (!appendU64(out_buf, *pos, entries[idx].ref_id)) return null;
        if (!appendBytes(out_buf, *pos, ": ")) return null;
        if (tool_name.len == 0) {
            if (!appendBytes(out_buf, *pos, "tool")) return null;
        } else {
            if (!appendBytes(out_buf, *pos, tool_name)) return null;
        }
        if (!appendBytes(out_buf, *pos, ", ")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, entries[idx].original_bytes))) return null;
        if (entries[idx].is_error) {
            if (!appendBytes(out_buf, *pos, "B, err")) return null;
        } else {
            if (!appendBytes(out_buf, *pos, "B, ok")) return null;
        }
        if (entries[idx].status == ToolOutputStatus.pinned) {
            if (!appendBytes(out_buf, *pos, ", pinned")) return null;
        }
        if (!appendBytes(out_buf, *pos, "] ")) return null;

        const prev_len: usize = @as(usize, entries[idx].preview_len);
        if (prev_len > 0) {
            if (!appendBytes(out_buf, *pos, entries[idx].preview_buf[0..prev_len])) return null;
        }
        if (entries[idx].preview_truncated) {
            if (!appendBytes(out_buf, *pos, "...")) return null;
        }
        return pos;
    }

    /// Render all active entries in chronological order (oldest to newest), separated by `\n`.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    fn renderAll(out_buf: *[]u8): usize? {
        let pos: usize = 0;
        let last_seq: u64 = 0;
        let rendered: usize = 0;

        while (rendered < entry_count) {
            let found = false;
            let best_idx: usize = 0;
            let best_seq: u64 = 0;

            let i: usize = 0;
            while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
                if (!entries[i].active) continue;
                if (entries[i].seq <= last_seq) continue;
                if (!found || entries[i].seq < best_seq) {
                    found = true;
                    best_idx = i;
                    best_seq = entries[i].seq;
                }
            }
            if (!found) break;

            if (rendered > 0) {
                if (!appendBytes(out_buf, *pos, "\n")) return null;
            }
            let rem: []u8 = out_buf[pos..];
            const written = renderEntry(entries[best_idx].ref_id, *rem) ?? return null;
            pos += written;
            last_seq = best_seq;
            rendered += 1;
        }

        return pos;
    }

    /// Return a zero-heap global telemetry snapshot.
    fn stats(): ToolHistoryStats {
        let st = ToolHistoryStats{};
        st.total_entries = entry_count;
        st.capacity = TOOL_HISTORY_MAX_ENTRIES;
        st.recorded_total = recorded_total;
        st.evicted_total = evicted_total;

        let i: usize = 0;
        while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            if (entries[i].status == ToolOutputStatus.full) {
                st.full_count += 1;
            } else if (entries[i].status == ToolOutputStatus.compacted) {
                st.compacted_count += 1;
            } else {
                st.pinned_count += 1;
            }
            if (entries[i].is_error) {
                st.error_count += 1;
            }
            st.active_tokens += @as(u64, entries[i].active_tokens);
            st.original_tokens_total += @as(u64, entries[i].original_tokens);
        }

        if (st.original_tokens_total > st.active_tokens) {
            st.saved_tokens = st.original_tokens_total - st.active_tokens;
        } else {
            st.saved_tokens = 0;
        }

        if (st.original_tokens_total > 0) {
            const bps_u64: u64 = (st.saved_tokens * @as(u64, TOOL_HISTORY_BPS_SCALE)) / st.original_tokens_total;
            st.savings_bps = if (bps_u64 > @as(u64, TOOL_HISTORY_BPS_SCALE)) TOOL_HISTORY_BPS_SCALE else @as(u32, @intCast(bps_u64));
        } else {
            st.savings_bps = 0;
        }

        return st;
    }
}
