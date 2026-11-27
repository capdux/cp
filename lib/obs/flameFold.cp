// Origin: Pure-CP Stdlib (Zero-Alloc Sampling Profiler Stack Aggregator & Brendan Gregg Folded Flamegraph Exporter) | Created: 2026-11-22
// Pure CP zero-heap-allocation CPU/wall-clock sampling profiler stack aggregator and flamegraph exporter:
//   - Compact inline symbol pool (`FLAME_MAX_SYMBOLS = 128`, `FLAME_SYMBOL_POOL_BYTES = 4096`)
//   - FNV-1a 64-bit stack signature aggregation (`FLAME_MAX_STACKS = 128`, `FLAME_MAX_DEPTH = 32`)
//   - Exact Self-Time and recursion-deduplicated Inclusive/Total-Time Top-K hotspot ranking
//   - Standard Brendan Gregg Folded Stack text exporter (`frameA;frameB;frameC <weight>\n`)
//     with automatic `sym#<id>` fallback for unresolved symbols
//   - Differential flamegraph regression comparator (`diffHotspots`) between baseline and current profiles

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const FLAME_MAX_SYMBOLS: usize = 128;
pub const FLAME_SYMBOL_POOL_BYTES: usize = 4096;
pub const FLAME_MAX_STACKS: usize = 128;
pub const FLAME_MAX_DEPTH: usize = 32;

fn computeStackSig(frames: []const u32): u64 {
    let h: u64 = 0xCBF29CE484222325;
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        const f: u64 = frames[i] as u64;
        h ^= f & 0xFF;
        h = h *% 0x100000001B3;
        h ^= (f >> 8) & 0xFF;
        h = h *% 0x100000001B3;
        h ^= (f >> 16) & 0xFF;
        h = h *% 0x100000001B3;
        h ^= (f >> 24) & 0xFF;
        h = h *% 0x100000001B3;
    }
    return h ^ ((frames.len as u64) *% 0x9E3779B97F4A7C15);
}

fn appendBytes(dst: *[]u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > dst.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        let ch: u8 = src[i];
        // Sanitize folded-format delimiter / newline characters
        if (ch == 59 || ch == 10 || ch == 13) {
            ch = 95; // '_'
        }
        dst[pos + i] = ch;
    }
    return pos + src.len;
}

fn appendByte(dst: *[]u8, pos: usize, ch: u8): usize? {
    if (pos >= dst.len) {
        return null;
    }
    dst[pos] = ch;
    return pos + 1;
}

fn appendU64Dec(dst: *[]u8, pos: usize, val_in: u64): usize? {
    let tmp: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = val_in;
    if (v == 0) {
        tmp[0] = 48; // '0'
        len = 1;
    } else {
        while (v > 0) : (v /= 10) {
            tmp[len] = (48 + (v % 10)) as u8;
            len += 1;
        }
    }
    if (pos + len > dst.len) {
        return null;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        dst[pos + i] = tmp[len - 1 - i];
    }
    return pos + len;
}

fn calcBps(part: u64, total: u64): u32 {
    if (total == 0) {
        return 0;
    }
    const bps: u64 = (part * 10000) / total;
    return if (bps > 10000) 10000 else (bps as u32);
}

// ============================================================================
// 1. Models & Telemetry Structs
// ============================================================================

/// Outcome returned by `FlameFoldProfiler.recordStack`.
pub enum FlameRecordStatus {
    ok,
    ok_truncated,
    empty_stack,
    zero_weight,
    table_full,
}

/// Compact symbol table entry pointing into `FlameFoldProfiler.symbol_pool`.
pub struct FlameSymbolEntry {
    symbol_id: u32 = 0;
    name_offset: u16 = 0;
    name_len: u16 = 0;
    occupied: bool = false;
}

/// Aggregated call-stack sample entry (`frames[0]` = root, `frames[depth - 1]` = leaf).
pub struct FlameStackEntry {
    sig_hash: u64 = 0;
    frames: [FLAME_MAX_DEPTH]u32 = [0; FLAME_MAX_DEPTH];
    depth: u8 = 0;
    weight: u64 = 0;
    sample_count: u64 = 0;
    occupied: bool = false;
}

/// Per-function hotspot summary with Self-Time and recursion-safe Inclusive-Time.
pub struct FlameHotspot {
    symbol_id: u32 = 0;
    self_weight: u64 = 0;
    inclusive_weight: u64 = 0;
    self_bps: u32 = 0;
    inclusive_bps: u32 = 0;
}

/// Differential flamegraph hotspot comparison entry (positive `delta_weight` = regression).
pub struct FlameHotspotDiff {
    symbol_id: u32 = 0;
    baseline_weight: u64 = 0;
    current_weight: u64 = 0;
    delta_weight: i64 = 0;
    baseline_bps: u32 = 0;
    current_bps: u32 = 0;
    delta_bps: i32 = 0;
}

/// Aggregate profiler statistics.
pub struct FlameFoldStats {
    symbol_count: usize = 0;
    symbol_pool_used: usize = 0;
    unique_stacks: usize = 0;
    total_samples: u64 = 0;
    total_weight: u64 = 0;
    truncated_samples: u64 = 0;
    dropped_overflow: u64 = 0;
}

fn isBetterHotspot(a: FlameHotspot, b: FlameHotspot, by_self: bool): bool {
    const pa: u64 = if (by_self) a.self_weight else a.inclusive_weight;
    const pb: u64 = if (by_self) b.self_weight else b.inclusive_weight;
    if (pa > pb) return true;
    if (pa < pb) return false;
    const sa: u64 = if (by_self) a.inclusive_weight else a.self_weight;
    const sb: u64 = if (by_self) b.inclusive_weight else b.self_weight;
    if (sa > sb) return true;
    if (sa < sb) return false;
    return a.symbol_id < b.symbol_id;
}

fn isBiggerRegression(a: FlameHotspotDiff, b: FlameHotspotDiff): bool {
    if (a.delta_weight > b.delta_weight) return true;
    if (a.delta_weight < b.delta_weight) return false;
    if (a.current_weight > b.current_weight) return true;
    if (a.current_weight < b.current_weight) return false;
    return a.symbol_id < b.symbol_id;
}

// ============================================================================
// 2. Zero-Alloc Sampling Profiler & Folded Flamegraph Exporter
// ============================================================================

/// Zero-heap-allocation sampling profiler stack aggregator and Brendan Gregg Folded
/// Flamegraph exporter.
pub struct FlameFoldProfiler {
    symbols: [FLAME_MAX_SYMBOLS]FlameSymbolEntry = [FlameSymbolEntry {}; FLAME_MAX_SYMBOLS];
    symbol_pool: [FLAME_SYMBOL_POOL_BYTES]u8 = [0; FLAME_SYMBOL_POOL_BYTES];
    symbol_count: usize = 0;
    symbol_pool_used: usize = 0;

    stacks: [FLAME_MAX_STACKS]FlameStackEntry = [FlameStackEntry {}; FLAME_MAX_STACKS];
    stack_count: usize = 0;
    total_samples: u64 = 0;
    total_weight: u64 = 0;
    truncated_samples: u64 = 0;
    dropped_overflow: u64 = 0;

    static fn init(): FlameFoldProfiler {
        return FlameFoldProfiler {};
    }

    /// Registers a function symbol name in the compact inline string pool.
    /// Rejects empty names, duplicate `symbol_id`, full symbol table, or exhausted pool.
    fn registerSymbol(*_, symbol_id: u32, name: []const u8): bool {
        if (name.len == 0 || _.symbol_count >= FLAME_MAX_SYMBOLS) {
            return false;
        }
        if (_.symbol_pool_used + name.len > FLAME_SYMBOL_POOL_BYTES) {
            return false;
        }
        let i: usize = 0;
        while (i < _.symbol_count) : (i += 1) {
            if (_.symbols[i].occupied && _.symbols[i].symbol_id == symbol_id) {
                return false;
            }
        }

        const off: usize = _.symbol_pool_used;
        let k: usize = 0;
        while (k < name.len) : (k += 1) {
            _.symbol_pool[off + k] = name[k];
        }
        _.symbols[_.symbol_count] = FlameSymbolEntry {
            symbol_id: symbol_id,
            name_offset: off as u16,
            name_len: name.len as u16,
            occupied: true,
        };
        _.symbol_count += 1;
        _.symbol_pool_used += name.len;
        return true;
    }

    /// Resolves a registered `symbol_id` to its zero-copy name slice, or `null` if unregistered.
    fn resolveSymbolName(symbol_id: u32): ([]const u8)? {
        let i: usize = 0;
        while (i < _.symbol_count) : (i += 1) {
            const sym = _.symbols[i];
            if (sym.occupied && sym.symbol_id == symbol_id) {
                const start: usize = sym.name_offset as usize;
                const end: usize = start + (sym.name_len as usize);
                return _.symbol_pool[start..end];
            }
        }
        return null;
    }

    fn stackMatches(entry: FlameStackEntry, sig: u64, frames: []const u32): bool {
        if (!entry.occupied || entry.sig_hash != sig || (entry.depth as usize) != frames.len) {
            return false;
        }
        let i: usize = 0;
        while (i < frames.len) : (i += 1) {
            if (entry.frames[i] != frames[i]) {
                return false;
            }
        }
        return true;
    }

    /// Records a sampled call stack (`frame_ids[0]` = root, `frame_ids[len - 1]` = leaf)
    /// with positive `sample_weight`, aggregating identical stacks by FNV-1a 64-bit signature.
    fn recordStack(*_, frame_ids: []const u32, sample_weight: u64): FlameRecordStatus {
        if (frame_ids.len == 0) {
            return FlameRecordStatus.empty_stack;
        }
        if (sample_weight == 0) {
            return FlameRecordStatus.zero_weight;
        }

        const truncated: bool = frame_ids.len > FLAME_MAX_DEPTH;
        const eff_len: usize = if (truncated) FLAME_MAX_DEPTH else frame_ids.len;
        const eff_slice: []const u32 = frame_ids[0..eff_len];
        const sig: u64 = computeStackSig(eff_slice);

        // Search existing aggregated stack.
        let i: usize = 0;
        while (i < _.stack_count) : (i += 1) {
            if (_.stackMatches(_.stacks[i], sig, eff_slice)) {
                _.stacks[i].weight += sample_weight;
                _.stacks[i].sample_count += 1;
                _.total_samples += 1;
                _.total_weight += sample_weight;
                if (truncated) {
                    _.truncated_samples += 1;
                    return FlameRecordStatus.ok_truncated;
                }
                return FlameRecordStatus.ok;
            }
        }

        // Insert new unique stack.
        if (_.stack_count >= FLAME_MAX_STACKS) {
            _.dropped_overflow += 1;
            return FlameRecordStatus.table_full;
        }

        let entry = FlameStackEntry {
            sig_hash: sig,
            depth: eff_len as u8,
            weight: sample_weight,
            sample_count: 1,
            occupied: true,
        };
        let f: usize = 0;
        while (f < eff_len) : (f += 1) {
            entry.frames[f] = eff_slice[f];
        }
        _.stacks[_.stack_count] = entry;
        _.stack_count += 1;
        _.total_samples += 1;
        _.total_weight += sample_weight;

        if (truncated) {
            _.truncated_samples += 1;
            return FlameRecordStatus.ok_truncated;
        }
        return FlameRecordStatus.ok;
    }

    /// Computes per-function Self-Time and recursion-deduplicated Inclusive-Time across all
    /// aggregated stacks and writes up to `out_hotspots.len` ranked entries.
    fn topHotspots(by_self: bool, out_hotspots: *[]FlameHotspot): usize {
        if (_.stack_count == 0 || out_hotspots.len == 0) {
            return 0;
        }

        let table: [FLAME_MAX_SYMBOLS]FlameHotspot = [FlameHotspot {}; FLAME_MAX_SYMBOLS];
        let count: usize = 0;

        let s: usize = 0;
        while (s < _.stack_count) : (s += 1) {
            const st = _.stacks[s];
            if (!st.occupied || st.depth == 0) {
                continue;
            }
            const d_len: usize = st.depth as usize;
            const leaf_sym: u32 = st.frames[d_len - 1];

            // Accumulate Self-Time on leaf frame.
            let found_leaf: usize = count;
            let k: usize = 0;
            while (k < count) : (k += 1) {
                if (table[k].symbol_id == leaf_sym) {
                    found_leaf = k;
                    break;
                }
            }
            if (found_leaf == count && count < FLAME_MAX_SYMBOLS) {
                table[count] = FlameHotspot { symbol_id: leaf_sym };
                count += 1;
            }
            if (found_leaf < count) {
                table[found_leaf].self_weight += st.weight;
            }

            // Accumulate Inclusive-Time once per distinct symbol in this stack (recursion-safe).
            let f: usize = 0;
            while (f < d_len) : (f += 1) {
                const sym: u32 = st.frames[f];
                let seen_earlier: bool = false;
                let prev: usize = 0;
                while (prev < f) : (prev += 1) {
                    if (st.frames[prev] == sym) {
                        seen_earlier = true;
                        break;
                    }
                }
                if (seen_earlier) {
                    continue;
                }

                let idx: usize = count;
                k = 0;
                while (k < count) : (k += 1) {
                    if (table[k].symbol_id == sym) {
                        idx = k;
                        break;
                    }
                }
                if (idx == count && count < FLAME_MAX_SYMBOLS) {
                    table[count] = FlameHotspot { symbol_id: sym };
                    count += 1;
                }
                if (idx < count) {
                    table[idx].inclusive_weight += st.weight;
                }
            }
        }

        // Compute basis-point shares.
        let i: usize = 0;
        while (i < count) : (i += 1) {
            table[i].self_bps = calcBps(table[i].self_weight, _.total_weight);
            table[i].inclusive_bps = calcBps(table[i].inclusive_weight, _.total_weight);
        }

        // Deterministic Top-K selection.
        let used: [FLAME_MAX_SYMBOLS]bool = [false; FLAME_MAX_SYMBOLS];
        const limit: usize = if (count < out_hotspots.len) count else out_hotspots.len;
        let written: usize = 0;
        while (written < limit) : (written += 1) {
            let best: usize = count;
            i = 0;
            while (i < count) : (i += 1) {
                if (used[i]) continue;
                if (best == count || isBetterHotspot(table[i], table[best], by_self)) {
                    best = i;
                }
            }
            if (best == count) break;
            used[best] = true;
            out_hotspots[written] = table[best];
        }
        return written;
    }

    /// Exports all aggregated stacks in standard Brendan Gregg Folded Stack format:
    /// `frame0;frame1;...;frameN <weight>\n` ordered by descending weight.
    /// Unregistered symbol IDs fall back to `sym#<id>`. Returns bytes written, or `null` if `out_buf` is too small.
    fn exportFoldedText(out_buf: *[]u8): usize? {
        let used: [FLAME_MAX_STACKS]bool = [false; FLAME_MAX_STACKS];
        let pos: usize = 0;

        let emitted: usize = 0;
        while (emitted < _.stack_count) : (emitted += 1) {
            let best: usize = _.stack_count;
            let i: usize = 0;
            while (i < _.stack_count) : (i += 1) {
                if (used[i] || !_.stacks[i].occupied) continue;
                if (best == _.stack_count
                    || _.stacks[i].weight > _.stacks[best].weight
                    || (_.stacks[i].weight == _.stacks[best].weight && _.stacks[i].sig_hash < _.stacks[best].sig_hash))
                {
                    best = i;
                }
            }
            if (best == _.stack_count) break;
            used[best] = true;

            const st = _.stacks[best];
            const d_len: usize = st.depth as usize;
            let f: usize = 0;
            while (f < d_len) : (f += 1) {
                if (f > 0) {
                    pos = appendByte(out_buf, pos, 59) ?? return null; // ';'
                }
                const sym_id: u32 = st.frames[f];
                if (name := _.resolveSymbolName(sym_id)) {
                    pos = appendBytes(out_buf, pos, name) ?? return null;
                } else {
                    pos = appendBytes(out_buf, pos, "sym#") ?? return null;
                    pos = appendU64Dec(out_buf, pos, sym_id as u64) ?? return null;
                }
            }
            pos = appendByte(out_buf, pos, 32) ?? return null; // ' '
            pos = appendU64Dec(out_buf, pos, st.weight) ?? return null;
            pos = appendByte(out_buf, pos, 10) ?? return null; // '\n'
        }
        return pos;
    }

    fn stats(): FlameFoldStats {
        return FlameFoldStats {
            symbol_count: _.symbol_count,
            symbol_pool_used: _.symbol_pool_used,
            unique_stacks: _.stack_count,
            total_samples: _.total_samples,
            total_weight: _.total_weight,
            truncated_samples: _.truncated_samples,
            dropped_overflow: _.dropped_overflow,
        };
    }

    /// Clears recorded stack samples while preserving the registered symbol table.
    fn clearSamples(*_) {
        let i: usize = 0;
        while (i < _.stack_count) : (i += 1) {
            _.stacks[i] = FlameStackEntry {};
        }
        _.stack_count = 0;
        _.total_samples = 0;
        _.total_weight = 0;
        _.truncated_samples = 0;
        _.dropped_overflow = 0;
    }

    /// Resets both samples and symbol table.
    fn reset(*_) {
        _.clearSamples();
        let i: usize = 0;
        while (i < _.symbol_count) : (i += 1) {
            _.symbols[i] = FlameSymbolEntry {};
        }
        _.symbol_count = 0;
        _.symbol_pool_used = 0;
    }
}

// ============================================================================
// 3. Differential Flamegraph Hotspot Comparator
// ============================================================================

/// Compares `baseline` vs `current` profiles and outputs per-symbol regression deltas
/// sorted by descending `delta_weight` (largest performance regressions first).
pub fn diffHotspots(
    baseline: FlameFoldProfiler,
    current: FlameFoldProfiler,
    by_self: bool,
    out_diffs: *[]FlameHotspotDiff,
): usize {
    if (out_diffs.len == 0) {
        return 0;
    }

    let base_hot: [FLAME_MAX_SYMBOLS]FlameHotspot = [FlameHotspot {}; FLAME_MAX_SYMBOLS];
    let base_s: []FlameHotspot = base_hot[0..];
    const base_n: usize = baseline.topHotspots(by_self, *base_s);

    let cur_hot: [FLAME_MAX_SYMBOLS]FlameHotspot = [FlameHotspot {}; FLAME_MAX_SYMBOLS];
    let cur_s: []FlameHotspot = cur_hot[0..];
    const cur_n: usize = current.topHotspots(by_self, *cur_s);

    let merged: [FLAME_MAX_SYMBOLS]FlameHotspotDiff = [FlameHotspotDiff {}; FLAME_MAX_SYMBOLS];
    let m_count: usize = 0;

    // Seed from baseline hotspots.
    let i: usize = 0;
    while (i < base_n && m_count < FLAME_MAX_SYMBOLS) : (i += 1) {
        const bw: u64 = if (by_self) base_hot[i].self_weight else base_hot[i].inclusive_weight;
        const bb: u32 = if (by_self) base_hot[i].self_bps else base_hot[i].inclusive_bps;
        merged[m_count] = FlameHotspotDiff {
            symbol_id: base_hot[i].symbol_id,
            baseline_weight: bw,
            current_weight: 0,
            delta_weight: -(bw as i64),
            baseline_bps: bb,
            current_bps: 0,
            delta_bps: -(bb as i32),
        };
        m_count += 1;
    }

    // Merge current hotspots.
    i = 0;
    while (i < cur_n) : (i += 1) {
        const sym: u32 = cur_hot[i].symbol_id;
        const cw: u64 = if (by_self) cur_hot[i].self_weight else cur_hot[i].inclusive_weight;
        const cb: u32 = if (by_self) cur_hot[i].self_bps else cur_hot[i].inclusive_bps;

        let idx: usize = m_count;
        let k: usize = 0;
        while (k < m_count) : (k += 1) {
            if (merged[k].symbol_id == sym) {
                idx = k;
                break;
            }
        }
        if (idx < m_count) {
            merged[idx].current_weight = cw;
            merged[idx].current_bps = cb;
            merged[idx].delta_weight = (cw as i64) - (merged[idx].baseline_weight as i64);
            merged[idx].delta_bps = (cb as i32) - (merged[idx].baseline_bps as i32);
        } else if (m_count < FLAME_MAX_SYMBOLS) {
            merged[m_count] = FlameHotspotDiff {
                symbol_id: sym,
                baseline_weight: 0,
                current_weight: cw,
                delta_weight: cw as i64,
                baseline_bps: 0,
                current_bps: cb,
                delta_bps: cb as i32,
            };
            m_count += 1;
        }
    }

    // Sort descending by regression delta_weight.
    let used: [FLAME_MAX_SYMBOLS]bool = [false; FLAME_MAX_SYMBOLS];
    const limit: usize = if (m_count < out_diffs.len) m_count else out_diffs.len;
    let written: usize = 0;
    while (written < limit) : (written += 1) {
        let best: usize = m_count;
        let j: usize = 0;
        while (j < m_count) : (j += 1) {
            if (used[j]) continue;
            if (best == m_count || isBiggerRegression(merged[j], merged[best])) {
                best = j;
            }
        }
        if (best == m_count) break;
        used[best] = true;
        out_diffs[written] = merged[best];
    }
    return written;
}
