// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateChunksInParallel.cp | Synced: 2026-11-23T14:41:08Z
// ============================================================================
// lib/bundler/chunkEmitPool.cp — Parallel Chunk Emit Pool, Content Hashing & Manifest
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven parallel chunk emission engine for
// `cap build`:
//   1. `cap:simd`-accelerated placeholder scanner (`scanChunkPlaceholdersFast`)
//      and safe placeholder substitution (`substituteChunkPlaceholders`) for
//      cross-chunk `[chunk_placeholder:<id>]` references;
//   2. LPT (Longest Processing Time first) parallel chunk emit scheduler pool
//      (`ChunkEmitPool`) with `max_workers` concurrency throttling, strict state
//      machine (`Pending -> Emitting -> Hashed -> Written | Failed | Cancelled`),
//      and fail-fast cancellation;
//   3. Two-phase content-addressed 64-bit hashing & multi-round fixed-point
//      placeholder resolution (`resolveChunkContentHashes`) with duplicate
//      output path detection;
//   4. Deterministic lexicographically-sorted Asset Manifest JSON generator
//      (`formatAssetManifestJson`) and emit telemetry reporter
//      (`formatChunkEmitPoolReport`).

import { Simd } from "cap:simd";

/// Default parallel worker slot limit for chunk emission.
pub const CHUNK_EMIT_DEFAULT_WORKERS: u32 = 4;

/// Sentinel ID indicating an unassigned chunk or worker slot.
pub const CHUNK_EMIT_INVALID_ID: u32 = 0xFFFFFFFF;

/// Default truncated hex hash length appended to chunk filenames (e.g. `vendor-a1b2c3d4.js`).
pub const CHUNK_EMIT_HASH_HEX_LEN: usize = 8;

/// Default maximum fixed-point convergence rounds for cross-chunk hash resolution.
pub const CHUNK_EMIT_DEFAULT_MAX_ROUNDS: u32 = 8;

/// Canonical placeholder prefix emitted by bundler codegen prior to final hashing.
pub const CHUNK_EMIT_PLACEHOLDER_PREFIX: []const u8 = "[chunk_placeholder:";

/// Error set for chunk emission scheduling, placeholder scanning, hashing, and manifest output.
pub err ChunkEmitPoolError {
    InvalidChunkId,
    DuplicateChunkId,
    InvalidStateTransition,
    SliceCapacityExceeded,
    BufferTooSmall,
    DuplicateOutputPath,
    HashNotConverged,
    UnknownPlaceholderTarget,
    InvalidHashHexLength,
}

// ============================================================================
// Internal Zero-Alloc Hashing, Byte, and JSON Formatting Helpers
// ============================================================================

const CE_FNV1A_OFFSET: u64 = 14695981039346656037;
const CE_FNV1A_PRIME: u64 = 1099511628211;
const CE_HEX_LOWER: []const u8 = "0123456789abcdef";

fn ceBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ceBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn ceHashUpdateBytes(h: u64, data: []const u8): u64 {
    let acc = h;
    let i: usize = 0;
    while (i < data.len) {
        acc = (acc ^ @as(u64, data[i])) *% CE_FNV1A_PRIME;
        i += 1;
    }
    return acc;
}

fn ceHashUpdateU64(h: u64, val: u64): u64 {
    let acc = h;
    let shift: u6 = 0;
    let i: u32 = 0;
    while (i < 8) {
        const b: u8 = @as(u8, @truncate(val >> shift));
        acc = (acc ^ @as(u64, b)) *% CE_FNV1A_PRIME;
        shift = shift +% 8;
        i += 1;
    }
    return acc;
}

fn ceHashFinalize(h: u64): u64 {
    // SplitMix64 avalanche finalizer for uniform 64-bit distribution.
    let z = h;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    z = z ^ (z >> 31);
    return z;
}

/// Computes a deterministic 64-bit content hash over `data` (FNV-1a + SplitMix64 avalanche).
pub fn computeChunkContentHash64(data: []const u8): u64 {
    const h0 = ceHashUpdateBytes(CE_FNV1A_OFFSET, data);
    const h1 = ceHashUpdateU64(h0, @as(u64, data.len));
    return ceHashFinalize(h1);
}

fn ceAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ceAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ceAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ceAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        ceAppendByte(out, pos, tmp[n])?;
    }
}

fn ceAppendHex64(out: []u8, pos: *usize, hash: u64, hex_len: usize)! {
    if (hex_len == 0 || hex_len > 16) return err.InvalidHashHexLength;
    if (hex_len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < hex_len) {
        const shift: u6 = @as(u6, @intCast((15 - i) * 4));
        const nibble: usize = @as(usize, @intCast((hash >> shift) & 0x0F));
        out[pos.* + i] = CE_HEX_LOWER[nibble];
        i += 1;
    }
    pos.* += hex_len;
}

fn ceAppendJsonEscapedString(out: []u8, pos: *usize, s: []const u8)! {
    ceAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            ceAppendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            ceAppendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            ceAppendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            ceAppendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            ceAppendBytes(out, pos, "\\t")?;
        } else if (c < 0x20) {
            ceAppendBytes(out, pos, "\\u00")?;
            const hi: usize = @as(usize, (c >> 4) & 0x0F);
            const lo: usize = @as(usize, c & 0x0F);
            ceAppendByte(out, pos, CE_HEX_LOWER[hi])?;
            ceAppendByte(out, pos, CE_HEX_LOWER[lo])?;
        } else {
            ceAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    ceAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: SIMD-Accelerated Cross-Chunk Placeholder Scanner & Substituter
// ============================================================================

/// A verified `[chunk_placeholder:<id>]` byte span within a chunk source buffer.
pub struct ChunkPlaceholderSpan {
    start: usize = 0,
    end: usize = 0,
    target_chunk_id: u32 = 0,

    static fn empty(): ChunkPlaceholderSpan {
        return ChunkPlaceholderSpan{};
    }
}

/// Aggregate telemetry returned by `scanChunkPlaceholdersFast`.
pub struct PlaceholderScanSummary {
    valid_count: usize = 0,
    malformed_count: usize = 0,
    simd_jumps: usize = 0,
}

/// Outcome of rewriting cross-chunk placeholders into an output buffer.
pub struct PlaceholderRewriteResult {
    bytes_written: usize = 0,
    replaced_count: usize = 0,
    malformed_count: usize = 0,
    unresolved_count: usize = 0,
}

/// Scans `src` using `cap:simd` (`Simd.indexOfByte`) to locate every well-formed
/// `[chunk_placeholder:<u32>]` marker. Malformed or truncated markers are safely
/// skipped and counted in `malformed_count` with zero panics and zero out-of-bounds
/// accesses.
pub fn scanChunkPlaceholdersFast(
    src: []const u8,
    out_spans: []ChunkPlaceholderSpan,
): ChunkEmitPoolError!PlaceholderScanSummary {
    let summary = PlaceholderScanSummary{};
    const prefix = CHUNK_EMIT_PLACEHOLDER_PREFIX;
    let pos: usize = 0;

    while (pos < src.len) {
        const rel_opt = Simd.indexOfByte(src[pos..], '[');
        summary.simd_jumps += 1;
        let rel: usize = 0;
        if (v := rel_opt) {
            rel = v;
        } else {
            break;
        }
        const start = pos + rel;

        // Check if remaining bytes can match "[chunk_placeholder:"
        const rem = src.len - start;
        if (rem < prefix.len) {
            // Check if it is a truncated prefix of "[chunk_placeholder:"
            if (rem >= 2 && ceBytesEql(src[start..src.len], prefix[0..rem])) {
                summary.malformed_count += 1;
            }
            break;
        }

        if (!ceBytesEql(src[start..start + prefix.len], prefix)) {
            pos = start + 1;
            continue;
        }

        // Prefix matched: parse decimal u32 chunk ID up to ']'
        let cur = start + prefix.len;
        if (cur >= src.len) {
            summary.malformed_count += 1;
            break;
        }

        let id_val: u64 = 0;
        let digits: usize = 0;
        let overflowed = false;

        while (cur < src.len) {
            const c = src[cur];
            if (c >= '0' && c <= '9') {
                digits += 1;
                if (digits <= 10) {
                    id_val = id_val * 10 + @as(u64, c - '0');
                    if (id_val > 0xFFFFFFFE) {
                        overflowed = true;
                    }
                } else {
                    overflowed = true;
                }
                cur += 1;
            } else {
                break;
            }
        }

        if (digits == 0 || overflowed || cur >= src.len || src[cur] != ']') {
            summary.malformed_count += 1;
            // Advance past '[' so nested valid placeholders inside malformed text can still be found.
            pos = start + 1;
            continue;
        }

        const end = cur + 1;
        if (summary.valid_count >= out_spans.len) {
            return err.SliceCapacityExceeded;
        }

        out_spans[summary.valid_count] = ChunkPlaceholderSpan{
            start: start,
            end: end,
            target_chunk_id: @as(u32, @intCast(id_val)),
        };
        summary.valid_count += 1;
        pos = end;
    }

    return summary;
}

fn findChunkIdIndex(chunk_ids: []const u32, target: u32): usize? {
    let i: usize = 0;
    while (i < chunk_ids.len) {
        if (chunk_ids[i] == target) return i;
        i += 1;
    }
    return null;
}

/// Rewrites all valid `[chunk_placeholder:<id>]` tokens in `src` using `chunk_ids`
/// and parallel `resolved_names` into caller-owned `out_buf`.
/// When `strict_targets` is true, referencing an unknown chunk ID returns
/// `err.UnknownPlaceholderTarget`; otherwise the original marker is preserved.
pub fn substituteChunkPlaceholders(
    src: []const u8,
    chunk_ids: []const u32,
    resolved_names: []const []const u8,
    strict_targets: bool,
    out_buf: []u8,
): ChunkEmitPoolError!PlaceholderRewriteResult {
    let result = PlaceholderRewriteResult{};
    if (chunk_ids.len != resolved_names.len) return err.InvalidChunkId;

    const prefix = CHUNK_EMIT_PLACEHOLDER_PREFIX;
    let out_pos: usize = 0;
    let copy_from: usize = 0;
    let scan_pos: usize = 0;

    while (scan_pos < src.len) {
        const rel_opt = Simd.indexOfByte(src[scan_pos..], '[');
        let rel: usize = 0;
        if (v := rel_opt) {
            rel = v;
        } else {
            break;
        }
        const start = scan_pos + rel;
        const rem = src.len - start;

        if (rem < prefix.len) {
            if (rem >= 2 && ceBytesEql(src[start..src.len], prefix[0..rem])) {
                result.malformed_count += 1;
            }
            break;
        }

        if (!ceBytesEql(src[start..start + prefix.len], prefix)) {
            scan_pos = start + 1;
            continue;
        }

        let cur = start + prefix.len;
        let id_val: u64 = 0;
        let digits: usize = 0;
        let overflowed = false;

        while (cur < src.len) {
            const c = src[cur];
            if (c >= '0' && c <= '9') {
                digits += 1;
                if (digits <= 10) {
                    id_val = id_val * 10 + @as(u64, c - '0');
                    if (id_val > 0xFFFFFFFE) overflowed = true;
                } else {
                    overflowed = true;
                }
                cur += 1;
            } else {
                break;
            }
        }

        if (digits == 0 || overflowed || cur >= src.len || src[cur] != ']') {
            result.malformed_count += 1;
            scan_pos = start + 1;
            continue;
        }

        const end = cur + 1;
        const target_id = @as(u32, @intCast(id_val));

        if (idx := findChunkIdIndex(chunk_ids, target_id)) {
            // Flush literal bytes before this placeholder
            if (start > copy_from) {
                ceAppendBytes(out_buf, &out_pos, src[copy_from..start])?;
            }
            ceAppendBytes(out_buf, &out_pos, resolved_names[idx])?;
            result.replaced_count += 1;
            copy_from = end;
            scan_pos = end;
        } else {
            if (strict_targets) {
                return err.UnknownPlaceholderTarget;
            }
            result.unresolved_count += 1;
            scan_pos = end;
        }
    }

    if (copy_from < src.len) {
        ceAppendBytes(out_buf, &out_pos, src[copy_from..src.len])?;
    }

    result.bytes_written = out_pos;
    return result;
}

// ============================================================================
// Section 2: LPT Parallel Chunk Emit Scheduler Pool (`ChunkEmitPool`)
// ============================================================================

/// Kind of bundle output chunk scheduled in `ChunkEmitPool`.
pub enum ChunkEmitKind: u8 {
    Js = 0,
    Css = 1,
    Html = 2,
    Asset = 3,
}

/// Lifecycle state of an individual chunk emission job.
pub enum ChunkEmitState: u8 {
    Pending = 0,
    Emitting = 1,
    Hashed = 2,
    Written = 3,
    Failed = 4,
    Cancelled = 5,
}

/// Descriptor and mutable execution state for a single chunk emission task.
pub struct ChunkEmitJob {
    chunk_id: u32 = 0,
    kind: ChunkEmitKind = ChunkEmitKind.Js,
    base_name: []const u8 = "",
    extension: []const u8 = ".js",
    entry_key: []const u8 = "",
    is_entry: bool = false,
    estimated_bytes: u64 = 0,
    priority_boost: u32 = 0,
    state: ChunkEmitState = ChunkEmitState.Pending,
    worker_slot: u32 = CHUNK_EMIT_INVALID_ID,
    content_hash: u64 = 0,
    final_size_bytes: usize = 0,
    error_code: u32 = 0,

    static fn empty(): ChunkEmitJob {
        return ChunkEmitJob{};
    }
}

/// Dispatched chunk emission assignment returned by `ChunkEmitPool.dispatchNext`.
pub struct ChunkEmitDispatchItem {
    task_index: usize = 0,
    chunk_id: u32 = 0,
    worker_slot: u32 = 0,
    estimated_bytes: u64 = 0,

    static fn empty(): ChunkEmitDispatchItem {
        return ChunkEmitDispatchItem{};
    }
}

/// Aggregate execution statistics snapshot for `ChunkEmitPool`.
pub struct ChunkEmitPoolStats {
    total_jobs: usize = 0,
    pending: usize = 0,
    emitting: usize = 0,
    hashed: usize = 0,
    written: usize = 0,
    failed: usize = 0,
    cancelled: usize = 0,
    active_workers: u32 = 0,
    max_workers: u32 = 0,
    peak_active_workers: u32 = 0,
    aborted: bool = false,
    total_estimated_bytes: u64 = 0,
    total_written_bytes: usize = 0,
    critical_path_bytes: u64 = 0,
}

fn isBetterLptCandidate(cand: ChunkEmitJob, best: ChunkEmitJob): bool {
    // Longest Processing Time first: larger estimated_bytes first to minimize tail latency.
    if (cand.estimated_bytes != best.estimated_bytes) {
        return cand.estimated_bytes > best.estimated_bytes;
    }
    if (cand.priority_boost != best.priority_boost) {
        return cand.priority_boost > best.priority_boost;
    }
    return cand.chunk_id < best.chunk_id;
}

/// Zero-heap, caller-slice-backed parallel chunk emission scheduler implementing
/// LPT (Longest Processing Time first) dispatch, `max_workers` slot throttling,
/// strict `Pending -> Emitting -> Hashed -> Written / Failed` state transitions,
/// and fail-fast cancellation.
pub struct ChunkEmitPool {
    jobs: []ChunkEmitJob,
    count: usize = 0,
    max_workers: u32 = CHUNK_EMIT_DEFAULT_WORKERS,
    active_workers: u32 = 0,
    peak_active_workers: u32 = 0,
    fail_fast: bool = true,
    aborted: bool = false,

    static fn init(
        jobs_buf: []ChunkEmitJob,
        max_workers: u32,
        fail_fast: bool,
    ): ChunkEmitPool {
        const eff_workers = if (max_workers == 0) CHUNK_EMIT_DEFAULT_WORKERS else max_workers;
        return ChunkEmitPool{
            jobs: jobs_buf,
            count: 0,
            max_workers: eff_workers,
            active_workers: 0,
            peak_active_workers: 0,
            fail_fast: fail_fast,
            aborted: false,
        };
    }

    /// Finds the task index for `chunk_id`, or `null` if not enqueued.
    fn findByChunkId(chunk_id: u32): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.jobs[i].chunk_id == chunk_id) return i;
            i += 1;
        }
        return null;
    }

    /// Enqueues a chunk emission job into the pool. Rejects `CHUNK_EMIT_INVALID_ID`
    /// and duplicate `chunk_id` values.
    fn enqueue(*_, job: ChunkEmitJob): ChunkEmitPoolError!usize {
        if (job.chunk_id == CHUNK_EMIT_INVALID_ID) return err.InvalidChunkId;
        if (_.findByChunkId(job.chunk_id) != null) return err.DuplicateChunkId;
        if (_.count >= _.jobs.len) return err.SliceCapacityExceeded;

        const idx = _.count;
        _.jobs[idx] = ChunkEmitJob{
            chunk_id: job.chunk_id,
            kind: job.kind,
            base_name: job.base_name,
            extension: job.extension,
            entry_key: job.entry_key,
            is_entry: job.is_entry,
            estimated_bytes: job.estimated_bytes,
            priority_boost: job.priority_boost,
            state: if (_.aborted) ChunkEmitState.Cancelled else ChunkEmitState.Pending,
            worker_slot: CHUNK_EMIT_INVALID_ID,
            content_hash: 0,
            final_size_bytes: 0,
            error_code: 0,
        };
        _.count += 1;
        return idx;
    }

    fn findFreeWorkerSlot(): u32 {
        let slot: u32 = 0;
        while (slot < _.max_workers) {
            let in_use = false;
            let i: usize = 0;
            while (i < _.count) {
                const st = _.jobs[i].state;
                if ((st == ChunkEmitState.Emitting || st == ChunkEmitState.Hashed) &&
                    _.jobs[i].worker_slot == slot)
                {
                    in_use = true;
                    break;
                }
                i += 1;
            }
            if (!in_use) return slot;
            slot += 1;
        }
        return CHUNK_EMIT_INVALID_ID;
    }

    /// Dispatches up to `min(out_batch.len, max_workers - active_workers)` pending
    /// jobs in descending `estimated_bytes` order (LPT scheduling).
    fn dispatchNext(*_, out_batch: []ChunkEmitDispatchItem): usize {
        if (_.aborted || out_batch.len == 0) return 0;

        let dispatched: usize = 0;
        while (dispatched < out_batch.len && _.active_workers < _.max_workers) {
            let best_idx: usize = 0;
            let found = false;

            let i: usize = 0;
            while (i < _.count) {
                if (_.jobs[i].state == ChunkEmitState.Pending) {
                    if (!found || isBetterLptCandidate(_.jobs[i], _.jobs[best_idx])) {
                        best_idx = i;
                        found = true;
                    }
                }
                i += 1;
            }

            if (!found) break;

            const slot = _.findFreeWorkerSlot();
            if (slot == CHUNK_EMIT_INVALID_ID) break;

            _.jobs[best_idx].state = ChunkEmitState.Emitting;
            _.jobs[best_idx].worker_slot = slot;
            _.active_workers += 1;
            if (_.active_workers > _.peak_active_workers) {
                _.peak_active_workers = _.active_workers;
            }

            out_batch[dispatched] = ChunkEmitDispatchItem{
                task_index: best_idx,
                chunk_id: _.jobs[best_idx].chunk_id,
                worker_slot: slot,
                estimated_bytes: _.jobs[best_idx].estimated_bytes,
            };
            dispatched += 1;
        }

        return dispatched;
    }

    /// Transitions a job from `Emitting` to `Hashed` and records its 64-bit content hash.
    /// Worker slot remains held until `markWritten` or `markFailed`.
    fn markHashed(*_, task_index: usize, content_hash: u64)! {
        if (task_index >= _.count) return err.InvalidChunkId;
        if (_.jobs[task_index].state != ChunkEmitState.Emitting) {
            return err.InvalidStateTransition;
        }
        _.jobs[task_index].state = ChunkEmitState.Hashed;
        _.jobs[task_index].content_hash = content_hash;
    }

    /// Transitions a job from `Hashed` to `Written`, recording final byte size and
    /// releasing its concurrency worker slot.
    fn markWritten(*_, task_index: usize, final_size_bytes: usize)! {
        if (task_index >= _.count) return err.InvalidChunkId;
        if (_.jobs[task_index].state != ChunkEmitState.Hashed) {
            return err.InvalidStateTransition;
        }
        _.jobs[task_index].state = ChunkEmitState.Written;
        _.jobs[task_index].final_size_bytes = final_size_bytes;
        _.jobs[task_index].worker_slot = CHUNK_EMIT_INVALID_ID;
        if (_.active_workers > 0) {
            _.active_workers -= 1;
        }
    }

    /// Transitions an active (`Emitting` or `Hashed`) job to `Failed`, releases its
    /// worker slot, and cancels all remaining `Pending` jobs when `fail_fast` is enabled.
    fn markFailed(*_, task_index: usize, error_code: u32)! {
        if (task_index >= _.count) return err.InvalidChunkId;
        const st = _.jobs[task_index].state;
        if (st != ChunkEmitState.Emitting && st != ChunkEmitState.Hashed) {
            return err.InvalidStateTransition;
        }
        _.jobs[task_index].state = ChunkEmitState.Failed;
        _.jobs[task_index].error_code = error_code;
        _.jobs[task_index].worker_slot = CHUNK_EMIT_INVALID_ID;
        if (_.active_workers > 0) {
            _.active_workers -= 1;
        }
        if (_.fail_fast) {
            _.aborted = true;
            * = _.cancelPending();
        }
    }

    /// Cancels all currently `Pending` jobs and returns the number cancelled.
    fn cancelPending(*_): usize {
        let cancelled: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.jobs[i].state == ChunkEmitState.Pending) {
                _.jobs[i].state = ChunkEmitState.Cancelled;
                cancelled += 1;
            }
            i += 1;
        }
        return cancelled;
    }

    /// Returns true when no jobs remain in `Pending`, `Emitting`, or `Hashed`.
    fn isDrained(): bool {
        let i: usize = 0;
        while (i < _.count) {
            const st = _.jobs[i].state;
            if (st == ChunkEmitState.Pending || st == ChunkEmitState.Emitting || st == ChunkEmitState.Hashed) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Computes an aggregate snapshot of pool progress and LPT critical-path metrics.
    fn stats(): ChunkEmitPoolStats {
        let s = ChunkEmitPoolStats{
            total_jobs: _.count,
            active_workers: _.active_workers,
            max_workers: _.max_workers,
            peak_active_workers: _.peak_active_workers,
            aborted: _.aborted,
        };
        let i: usize = 0;
        while (i < _.count) {
            const job = _.jobs[i];
            s.total_estimated_bytes += job.estimated_bytes;
            if (job.estimated_bytes > s.critical_path_bytes) {
                s.critical_path_bytes = job.estimated_bytes;
            }
            if (job.state == ChunkEmitState.Pending) {
                s.pending += 1;
            } else if (job.state == ChunkEmitState.Emitting) {
                s.emitting += 1;
            } else if (job.state == ChunkEmitState.Hashed) {
                s.hashed += 1;
            } else if (job.state == ChunkEmitState.Written) {
                s.written += 1;
                s.total_written_bytes += job.final_size_bytes;
            } else if (job.state == ChunkEmitState.Failed) {
                s.failed += 1;
            } else if (job.state == ChunkEmitState.Cancelled) {
                s.cancelled += 1;
            }
            i += 1;
        }
        return s;
    }
}

// ============================================================================
// Section 3: Two-Phase Content-Addressed Hashing & Placeholder Backfill
// ============================================================================

/// Input descriptor for a single chunk entering content-addressed hash resolution.
pub struct ChunkHashInput {
    chunk_id: u32 = 0,
    base_name: []const u8 = "",
    extension: []const u8 = ".js",
    public_prefix: []const u8 = "",
    raw_source: []const u8 = "",
    include_hash_in_name: bool = true,

    static fn empty(): ChunkHashInput {
        return ChunkHashInput{};
    }
}

/// Resolved output metadata referencing caller-provided `filename_arena` and `code_arena`.
pub struct ResolvedChunkOutput {
    chunk_id: u32 = 0,
    content_hash: u64 = 0,
    filename_start: usize = 0,
    filename_len: usize = 0,
    code_start: usize = 0,
    code_len: usize = 0,
    placeholders_replaced: usize = 0,
    malformed_placeholders: usize = 0,

    static fn empty(): ResolvedChunkOutput {
        return ResolvedChunkOutput{};
    }
}

/// Configuration for `resolveChunkContentHashes`.
pub struct ChunkHashResolveOptions {
    max_rounds: u32 = CHUNK_EMIT_DEFAULT_MAX_ROUNDS,
    hash_hex_len: usize = CHUNK_EMIT_HASH_HEX_LEN,
    strict_placeholders: bool = false,
    detect_duplicate_paths: bool = true,
    require_convergence: bool = true,
}

/// Summary returned by `resolveChunkContentHashes`.
pub struct ChunkHashResolveSummary {
    chunk_count: usize = 0,
    rounds_executed: u32 = 0,
    converged: bool = false,
    total_placeholders_replaced: usize = 0,
    total_malformed_placeholders: usize = 0,
    filename_bytes_used: usize = 0,
    code_bytes_used: usize = 0,
}

/// Formats a content-addressed chunk filename `<public_prefix><base_name>-<hex><extension>`
/// (or `<public_prefix><base_name><extension>` when `include_hash` is false) into `out_buf`.
pub fn formatChunkHashedFilename(
    public_prefix: []const u8,
    base_name: []const u8,
    extension: []const u8,
    content_hash: u64,
    hex_len: usize,
    include_hash: bool,
    out_buf: []u8,
): ChunkEmitPoolError!usize {
    let pos: usize = 0;
    ceAppendBytes(out_buf, &pos, public_prefix)?;
    ceAppendBytes(out_buf, &pos, base_name)?;
    if (include_hash) {
        ceAppendByte(out_buf, &pos, '-')?;
        ceAppendHex64(out_buf, &pos, content_hash, hex_len)?;
    }
    ceAppendBytes(out_buf, &pos, extension)?;
    return pos;
}

/// Extracts the resolved filename slice from `filename_arena`.
pub fn getResolvedFilename(res: ResolvedChunkOutput, filename_arena: []const u8): []const u8 {
    if (res.filename_start + res.filename_len > filename_arena.len) return "";
    return filename_arena[res.filename_start..res.filename_start + res.filename_len];
}

/// Extracts the placeholder-substituted source code slice from `code_arena`.
pub fn getResolvedCode(res: ResolvedChunkOutput, code_arena: []const u8): []const u8 {
    if (res.code_start + res.code_len > code_arena.len) return "";
    return code_arena[res.code_start..res.code_start + res.code_len];
}

/// Performs two-phase content-addressed 64-bit hashing and multi-round fixed-point
/// cross-chunk `[chunk_placeholder:<id>]` backfill with zero heap allocation:
///   - Phase 1: Computes initial 64-bit hashes from each chunk's `raw_source` and
///     materializes initial hashed filenames into `filename_arena`.
///   - Phase 2: Iteratively substitutes all `[chunk_placeholder:<id>]` tokens in every
///     chunk using the latest resolved filenames into `code_arena`, recomputes 64-bit
///     content hashes from the rewritten output, and regenerates filenames until all
///     chunk hashes remain identical across a full round (fixed-point convergence).
///   - Validates output path uniqueness (`err.DuplicateOutputPath`) when enabled.
///
/// Caller provides scratch slices `chunk_ids_scratch` (`len >= inputs.len`) and
/// `names_scratch` (`len >= inputs.len`), plus `out_results` (`len >= inputs.len`),
/// `filename_arena`, and `code_arena`.
pub fn resolveChunkContentHashes(
    inputs: []const ChunkHashInput,
    options: ChunkHashResolveOptions,
    out_results: []ResolvedChunkOutput,
    chunk_ids_scratch: []u32,
    names_scratch: [][]const u8,
    filename_arena: []u8,
    code_arena: []u8,
): ChunkEmitPoolError!ChunkHashResolveSummary {
    const n = inputs.len;
    if (out_results.len < n || chunk_ids_scratch.len < n || names_scratch.len < n) {
        return err.SliceCapacityExceeded;
    }
    if (options.hash_hex_len == 0 || options.hash_hex_len > 16) {
        return err.InvalidHashHexLength;
    }

    let summary = ChunkHashResolveSummary{
        chunk_count: n,
        rounds_executed: 0,
        converged: true,
    };
    if (n == 0) return summary;

    // Validate unique chunk IDs and populate Phase 1 initial hashes & filenames.
    let fn_pos: usize = 0;
    let i: usize = 0;
    while (i < n) {
        const inp = inputs[i];
        if (inp.chunk_id == CHUNK_EMIT_INVALID_ID) return err.InvalidChunkId;
        let j: usize = 0;
        while (j < i) {
            if (chunk_ids_scratch[j] == inp.chunk_id) return err.DuplicateChunkId;
            j += 1;
        }
        chunk_ids_scratch[i] = inp.chunk_id;

        const init_hash = computeChunkContentHash64(inp.raw_source);
        const fn_start = fn_pos;
        const fn_len = formatChunkHashedFilename(
            inp.public_prefix,
            inp.base_name,
            inp.extension,
            init_hash,
            options.hash_hex_len,
            inp.include_hash_in_name,
            filename_arena[fn_start..],
        )?;
        fn_pos += fn_len;

        out_results[i] = ResolvedChunkOutput{
            chunk_id: inp.chunk_id,
            content_hash: init_hash,
            filename_start: fn_start,
            filename_len: fn_len,
            code_start: 0,
            code_len: 0,
            placeholders_replaced: 0,
            malformed_placeholders: 0,
        };
        names_scratch[i] = filename_arena[fn_start..fn_start + fn_len];
        i += 1;
    }

    // Phase 2: Iterative backfill + hash recomputation until fixed-point convergence.
    const max_rounds = if (options.max_rounds == 0) 1 else options.max_rounds;
    let round: u32 = 0;
    let converged = false;
    let final_code_pos: usize = 0;
    let final_fn_pos: usize = fn_pos;
    let final_replaced: usize = 0;
    let final_malformed: usize = 0;

    while (round < max_rounds) {
        round += 1;
        let code_pos: usize = 0;
        let round_changed = false;
        let round_replaced: usize = 0;
        let round_malformed: usize = 0;

        // Pass 2a: Substitute all placeholders using current `names_scratch`
        let k: usize = 0;
        while (k < n) {
            const inp = inputs[k];
            const c_start = code_pos;
            const rw = substituteChunkPlaceholders(
                inp.raw_source,
                chunk_ids_scratch[0..n],
                names_scratch[0..n],
                options.strict_placeholders,
                code_arena[c_start..],
            )?;
            code_pos += rw.bytes_written;
            round_replaced += rw.replaced_count;
            round_malformed += rw.malformed_count;

            const new_hash = computeChunkContentHash64(code_arena[c_start..c_start + rw.bytes_written]);
            const prev = out_results[k];
            if (new_hash != prev.content_hash) {
                round_changed = true;
            }
            out_results[k] = ResolvedChunkOutput{
                chunk_id: prev.chunk_id,
                content_hash: new_hash,
                filename_start: prev.filename_start,
                filename_len: prev.filename_len,
                code_start: c_start,
                code_len: rw.bytes_written,
                placeholders_replaced: rw.replaced_count,
                malformed_placeholders: rw.malformed_count,
            };
            k += 1;
        }

        // Pass 2b: Rebuild filenames in `filename_arena` from updated hashes
        let next_fn_pos: usize = 0;
        let m: usize = 0;
        while (m < n) {
            const inp = inputs[m];
            const prev = out_results[m];
            const f_start = next_fn_pos;
            const f_len = formatChunkHashedFilename(
                inp.public_prefix,
                inp.base_name,
                inp.extension,
                prev.content_hash,
                options.hash_hex_len,
                inp.include_hash_in_name,
                filename_arena[f_start..],
            )?;
            next_fn_pos += f_len;
            out_results[m] = ResolvedChunkOutput{
                chunk_id: prev.chunk_id,
                content_hash: prev.content_hash,
                filename_start: f_start,
                filename_len: f_len,
                code_start: prev.code_start,
                code_len: prev.code_len,
                placeholders_replaced: prev.placeholders_replaced,
                malformed_placeholders: prev.malformed_placeholders,
            };
            names_scratch[m] = filename_arena[f_start..f_start + f_len];
            m += 1;
        }

        final_code_pos = code_pos;
        final_fn_pos = next_fn_pos;
        final_replaced = round_replaced;
        final_malformed = round_malformed;

        if (!round_changed) {
            converged = true;
            break;
        }
    }

    if (!converged && options.require_convergence) {
        return err.HashNotConverged;
    }

    // Duplicate output path check (mirrors Bun's DuplicateOutputPath guard).
    if (options.detect_duplicate_paths) {
        let a: usize = 0;
        while (a < n) {
            let b: usize = a + 1;
            while (b < n) {
                if (ceBytesEql(names_scratch[a], names_scratch[b])) {
                    return err.DuplicateOutputPath;
                }
                b += 1;
            }
            a += 1;
        }
    }

    summary.rounds_executed = round;
    summary.converged = converged;
    summary.total_placeholders_replaced = final_replaced;
    summary.total_malformed_placeholders = final_malformed;
    summary.filename_bytes_used = final_fn_pos;
    summary.code_bytes_used = final_code_pos;
    return summary;
}

// ============================================================================
// Section 4: Deterministic Asset Manifest JSON Generator
// ============================================================================

/// Manifest descriptor for one emitted entry or chunk asset.
/// `imports_start..imports_start + imports_len` indexes into `all_imports`,
/// and `css_start..css_start + css_len` indexes into `all_css`.
pub struct ManifestChunkEntry {
    entry_key: []const u8 = "",
    output_file: []const u8 = "",
    is_entry: bool = false,
    size_bytes: usize = 0,
    integrity_hash: u64 = 0,
    imports_start: usize = 0,
    imports_len: usize = 0,
    css_start: usize = 0,
    css_len: usize = 0,

    static fn empty(): ManifestChunkEntry {
        return ManifestChunkEntry{};
    }
}

/// Formats a deterministic, lexicographically-sorted Asset Manifest JSON object
/// mapping `entry_key -> { file, isEntry, size, integrity, imports, css }` into
/// caller-owned `out_buf` (zero heap allocation).
/// `order_scratch` must have length `>= entries.len`.
pub fn formatAssetManifestJson(
    entries: []const ManifestChunkEntry,
    all_imports: []const []const u8,
    all_css: []const []const u8,
    order_scratch: []usize,
    out_buf: []u8,
): ChunkEmitPoolError!usize {
    const n = entries.len;
    if (order_scratch.len < n) return err.SliceCapacityExceeded;

    // Initialize index permutation and stable insertion-sort by `entry_key` ascending.
    let i: usize = 0;
    while (i < n) {
        order_scratch[i] = i;
        i += 1;
    }

    let s: usize = 1;
    while (s < n) {
        const key_idx = order_scratch[s];
        let j = s;
        while (j > 0 && ceBytesLessThan(entries[key_idx].entry_key, entries[order_scratch[j - 1]].entry_key)) {
            order_scratch[j] = order_scratch[j - 1];
            j -= 1;
        }
        order_scratch[j] = key_idx;
        s += 1;
    }

    let pos: usize = 0;
    if (n == 0) {
        ceAppendBytes(out_buf, &pos, "{}\n")?;
        return pos;
    }

    ceAppendBytes(out_buf, &pos, "{\n")?;

    let k: usize = 0;
    while (k < n) {
        const e = entries[order_scratch[k]];
        ceAppendBytes(out_buf, &pos, "  ")?;
        ceAppendJsonEscapedString(out_buf, &pos, e.entry_key)?;
        ceAppendBytes(out_buf, &pos, ": {\n")?;

        // "file"
        ceAppendBytes(out_buf, &pos, "    \"file\": ")?;
        ceAppendJsonEscapedString(out_buf, &pos, e.output_file)?;
        ceAppendBytes(out_buf, &pos, ",\n")?;

        // "isEntry"
        ceAppendBytes(out_buf, &pos, "    \"isEntry\": ")?;
        if (e.is_entry) {
            ceAppendBytes(out_buf, &pos, "true,\n")?;
        } else {
            ceAppendBytes(out_buf, &pos, "false,\n")?;
        }

        // "size"
        ceAppendBytes(out_buf, &pos, "    \"size\": ")?;
        ceAppendU64(out_buf, &pos, @as(u64, e.size_bytes))?;
        ceAppendBytes(out_buf, &pos, ",\n")?;

        // "integrity"
        ceAppendBytes(out_buf, &pos, "    \"integrity\": \"h64-")?;
        ceAppendHex64(out_buf, &pos, e.integrity_hash, 16)?;
        ceAppendBytes(out_buf, &pos, "\",\n")?;

        // "imports"
        ceAppendBytes(out_buf, &pos, "    \"imports\": [")?;
        const imp_end = if (e.imports_start <= all_imports.len && e.imports_len <= all_imports.len - e.imports_start)
            e.imports_start + e.imports_len
        else
            e.imports_start;
        let im: usize = e.imports_start;
        let first_imp = true;
        while (im < imp_end && im < all_imports.len) {
            if (!first_imp) ceAppendBytes(out_buf, &pos, ", ")?;
            ceAppendJsonEscapedString(out_buf, &pos, all_imports[im])?;
            first_imp = false;
            im += 1;
        }
        ceAppendBytes(out_buf, &pos, "],\n")?;

        // "css"
        ceAppendBytes(out_buf, &pos, "    \"css\": [")?;
        const css_end = if (e.css_start <= all_css.len && e.css_len <= all_css.len - e.css_start)
            e.css_start + e.css_len
        else
            e.css_start;
        let ci: usize = e.css_start;
        let first_css = true;
        while (ci < css_end && ci < all_css.len) {
            if (!first_css) ceAppendBytes(out_buf, &pos, ", ")?;
            ceAppendJsonEscapedString(out_buf, &pos, all_css[ci])?;
            first_css = false;
            ci += 1;
        }
        ceAppendBytes(out_buf, &pos, "]\n")?;

        ceAppendBytes(out_buf, &pos, "  }")?;
        if (k + 1 < n) {
            ceAppendByte(out_buf, &pos, ',')?;
        }
        ceAppendByte(out_buf, &pos, '\n')?;
        k += 1;
    }

    ceAppendBytes(out_buf, &pos, "}\n")?;
    return pos;
}

// ============================================================================
// Section 5: Emit Pool Telemetry & Diagnostics Reporter
// ============================================================================

/// Formats a human-readable summary report for `ChunkEmitPoolStats` into `out_buf`.
pub fn formatChunkEmitPoolReport(
    stats: ChunkEmitPoolStats,
    out_buf: []u8,
): ChunkEmitPoolError!usize {
    let pos: usize = 0;
    ceAppendBytes(out_buf, &pos, "ChunkEmitPool Summary\n")?;
    ceAppendBytes(out_buf, &pos, "  jobs: total=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.total_jobs))?;
    ceAppendBytes(out_buf, &pos, " written=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.written))?;
    ceAppendBytes(out_buf, &pos, " failed=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.failed))?;
    ceAppendBytes(out_buf, &pos, " cancelled=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.cancelled))?;
    ceAppendByte(out_buf, &pos, '\n')?;

    ceAppendBytes(out_buf, &pos, "  workers: max=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.max_workers))?;
    ceAppendBytes(out_buf, &pos, " peak=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.peak_active_workers))?;
    ceAppendBytes(out_buf, &pos, " active=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.active_workers))?;
    ceAppendBytes(out_buf, &pos, " aborted=")?;
    if (stats.aborted) {
        ceAppendBytes(out_buf, &pos, "true\n")?;
    } else {
        ceAppendBytes(out_buf, &pos, "false\n")?;
    }

    ceAppendBytes(out_buf, &pos, "  bytes: estimated=")?;
    ceAppendU64(out_buf, &pos, stats.total_estimated_bytes)?;
    ceAppendBytes(out_buf, &pos, " written=")?;
    ceAppendU64(out_buf, &pos, @as(u64, stats.total_written_bytes))?;
    ceAppendBytes(out_buf, &pos, " critical_path=")?;
    ceAppendU64(out_buf, &pos, stats.critical_path_bytes)?;
    ceAppendByte(out_buf, &pos, '\n')?;
    return pos;
}
