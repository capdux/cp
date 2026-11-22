// Origin: Pure-CP Stdlib (Zero-Alloc FastCDC Gear Rolling Hash Chunker & Dedup Index) | Created: 2026-11-22
// High-throughput zero-heap-allocation FastCDC content-defined chunking (Gear Rolling Hash with
// dual-mask normalization) and open-addressing fingerprint deduplication index for backup engines,
// AI dataset incremental sync, and content-addressed artifact caches.

const FNV1A_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME: u64 = 0x00000100000001B3;

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV1A_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = (h ^ (data[i] as u64)) *% FNV1A_PRIME;
    }
    return h;
}

fn splitMix64(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn floorLog2(v: usize): u6 {
    let x = v;
    let bits: u6 = 0;
    while (x > 1 && bits < 63) {
        x = x >> 1;
        bits += 1;
    }
    return bits;
}

/// Builds a high-bit Gear mask with `n_bits` ones placed in the most significant bits of a `u64`.
/// High-bit masks exploit full 64-bit sliding history under left-shifting Gear rolling hashes.
fn makeHighBitMask(n_bits: u6): u64 {
    if (n_bits == 0) return 0;
    if (n_bits >= 63) return 0xFFFFFFFFFFFFFFFF;
    const ones: u64 = ((1 as u64) << n_bits) - 1;
    const shift: u6 = (64 - (n_bits as u8)) as u6;
    return ones << shift;
}

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) return 0xFFFFFFFF;
    return a + b;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) return 0xFFFFFFFFFFFFFFFF;
    return a + b;
}

/// Configuration for `FastCdcChunker`.
/// Enforces `min_size >= 64` and `min_size < avg_size < max_size`.
pub struct FastCdcConfig {
    min_size: usize = 2048;
    avg_size: usize = 8192;
    max_size: usize = 32768;

    /// Validates and constructs a `FastCdcConfig`, or returns `null` if invalid.
    static fn init(min_size: usize, avg_size: usize, max_size: usize): FastCdcConfig? {
        if (min_size < 64 || min_size >= avg_size || avg_size >= max_size) {
            return null;
        }
        return FastCdcConfig{
            min_size: min_size,
            avg_size: avg_size,
            max_size: max_size,
        };
    }

    /// Returns `true` if the configuration satisfies FastCDC invariants.
    fn isValid(): bool {
        return _.min_size >= 64 && _.min_size < _.avg_size && _.avg_size < _.max_size;
    }
}

/// Zero-copy span describing a single content-defined chunk and its 64-bit FNV-1a fingerprint.
pub struct CdcChunkSpan {
    offset: usize = 0;
    len: usize = 0;
    fingerprint: u64 = 0;
}

/// Zero-heap-allocation FastCDC content-defined chunker using a 256-entry Gear rolling hash
/// table and dual-mask normalization (`mask_s` for `< avg_size`, `mask_l` for `>= avg_size`).
pub struct FastCdcChunker {
    cfg: FastCdcConfig = FastCdcConfig{},
    mask_s: u64 = 0,
    mask_l: u64 = 0,
    cursor: usize = 0,
    gear: [256]u64 = [0; 256],

    /// Initializes a `FastCdcChunker` from `cfg`. Returns `null` if `cfg` is invalid.
    static fn init(cfg: FastCdcConfig): FastCdcChunker? {
        if (!cfg.isValid()) {
            return null;
        }
        const avg_bits = floorLog2(cfg.avg_size);
        const s_bits: u6 = if (avg_bits < 62) (avg_bits + 1) else 62;
        const l_bits: u6 = if (avg_bits > 1) (avg_bits - 1) else 1;

        let chunker = FastCdcChunker{
            cfg: cfg,
            mask_s: makeHighBitMask(s_bits),
            mask_l: makeHighBitMask(l_bits),
            cursor: 0,
            gear: [0; 256],
        };

        // Deterministic 256-entry Gear random lookup table (SplitMix64 seeded with FastCDC tag)
        let seed: u64 = 0x4641535443444331; // "FASTCDC1"
        let i: usize = 0;
        while (i < 256) : (i += 1) {
            chunker.gear[i] = splitMix64(*seed);
        }
        return chunker;
    }

    /// Resets the streaming cursor to offset `0`.
    fn reset(*_) {
        _.cursor = 0;
    }

    /// Returns the current byte offset of the streaming cursor.
    fn position(): usize {
        return _.cursor;
    }

    fn findCutLength(data: []const u8, start: usize): usize {
        const rem = data.len - start;
        if (rem <= _.cfg.min_size) {
            return rem;
        }
        const limit = if (rem < _.cfg.max_size) rem else _.cfg.max_size;
        const normal = if (_.cfg.avg_size < limit) _.cfg.avg_size else limit;

        let fp: u64 = 0;
        let i: usize = _.cfg.min_size;

        // Phase 1 (< avg_size): stricter mask_s suppresses overly small chunks
        while (i < normal) : (i += 1) {
            fp = (fp << 1) +% _.gear[data[start + i]];
            if ((fp & _.mask_s) == 0) {
                return i + 1;
            }
        }

        // Phase 2 (>= avg_size): looser mask_l encourages cutpoints before max_size
        while (i < limit) : (i += 1) {
            fp = (fp << 1) +% _.gear[data[start + i]];
            if ((fp & _.mask_l) == 0) {
                return i + 1;
            }
        }

        return limit;
    }

    /// Advances the streaming cursor by one content-defined chunk over `data`.
    /// Returns `null` when `cursor >= data.len`.
    fn nextChunk(*_, data: []const u8): CdcChunkSpan? {
        if (_.cursor >= data.len) {
            return null;
        }
        const start = _.cursor;
        const clen = _.findCutLength(data, start);
        const fp = fnv1a64(data[start .. start + clen]);
        _.cursor = start + clen;
        return CdcChunkSpan{
            offset: start,
            len: clen,
            fingerprint: fp,
        };
    }

    /// Chunks the entire `data` slice from offset `0` into `out_spans` without modifying `cursor`.
    /// Stops cleanly when `data` is exhausted or `out_spans` is full, returning the number of spans written.
    fn chunkAll(data: []const u8, out_spans: []CdcChunkSpan): usize {
        let pos: usize = 0;
        let count: usize = 0;
        while (pos < data.len && count < out_spans.len) {
            const clen = _.findCutLength(data, pos);
            const fp = fnv1a64(data[pos .. pos + clen]);
            out_spans[count] = CdcChunkSpan{
                offset: pos,
                len: clen,
                fingerprint: fp,
            };
            pos += clen;
            count += 1;
        }
        return count;
    }
}

/// Single open-addressing slot stored in a caller-provided `CdcDedupTable` buffer.
pub struct CdcDedupSlot {
    fingerprint: u64 = 0;
    chunk_len: usize = 0;
    hit_count: u32 = 0;
    occupied: bool = false;
}

/// Aggregate deduplication metrics produced by `CdcDedupTable`.
pub struct CdcDedupStats {
    total_chunks: u64 = 0;
    unique_chunks: u64 = 0;
    duplicate_chunks: u64 = 0;
    total_bytes: u64 = 0;
    unique_bytes: u64 = 0;
    duplicate_bytes: u64 = 0;
    table_full_drops: u64 = 0;

    /// Returns the byte deduplication savings ratio in `[0.0, 1.0]`:
    /// `(total_bytes - unique_bytes) / total_bytes`, or `0.0` when `total_bytes == 0`.
    fn dedupRatio(): f64 {
        if (_.total_bytes == 0) return 0.0;
        const saved = if (_.total_bytes >= _.unique_bytes) (_.total_bytes - _.unique_bytes) else 0;
        return (saved as f64) / (_.total_bytes as f64);
    }
}

/// Zero-heap-allocation open-addressing chunk fingerprint deduplication index.
pub struct CdcDedupTable {
    slots: []CdcDedupSlot;
    cur_stats: CdcDedupStats = CdcDedupStats{};

    /// Initializes a `CdcDedupTable` backed by caller-provided `slots`.
    /// Returns `null` if `slots.len == 0`.
    static fn init(slots: []CdcDedupSlot): CdcDedupTable? {
        if (slots.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = CdcDedupSlot{};
        }
        return CdcDedupTable{
            slots: slots,
            cur_stats: CdcDedupStats{},
        };
    }

    /// Resets all slots and deduplication statistics for reuse.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = CdcDedupSlot{};
        }
        _.cur_stats = CdcDedupStats{};
    }

    /// Observes a chunk span in the deduplication index.
    /// Returns `true` if the chunk is newly inserted (unique), or `false` if duplicate (or table full).
    fn observe(*_, span: CdcChunkSpan): bool {
        const cap = _.slots.len;
        const start = (span.fingerprint as usize) % cap;
        let probe: usize = 0;
        while (probe < cap) : (probe += 1) {
            const idx = (start + probe) % cap;
            if (!_.slots[idx].occupied) {
                _.slots[idx] = CdcDedupSlot{
                    fingerprint: span.fingerprint,
                    chunk_len: span.len,
                    hit_count: 1,
                    occupied: true,
                };
                _.cur_stats.total_chunks = satAddU64(_.cur_stats.total_chunks, 1);
                _.cur_stats.unique_chunks = satAddU64(_.cur_stats.unique_chunks, 1);
                _.cur_stats.total_bytes = satAddU64(_.cur_stats.total_bytes, span.len as u64);
                _.cur_stats.unique_bytes = satAddU64(_.cur_stats.unique_bytes, span.len as u64);
                return true;
            }
            if (_.slots[idx].fingerprint == span.fingerprint && _.slots[idx].chunk_len == span.len) {
                _.slots[idx].hit_count = satAddU32(_.slots[idx].hit_count, 1);
                _.cur_stats.total_chunks = satAddU64(_.cur_stats.total_chunks, 1);
                _.cur_stats.duplicate_chunks = satAddU64(_.cur_stats.duplicate_chunks, 1);
                _.cur_stats.total_bytes = satAddU64(_.cur_stats.total_bytes, span.len as u64);
                _.cur_stats.duplicate_bytes = satAddU64(_.cur_stats.duplicate_bytes, span.len as u64);
                return false;
            }
        }

        _.cur_stats.table_full_drops = satAddU64(_.cur_stats.table_full_drops, 1);
        return false;
    }

    /// Returns `true` if `fingerprint` is currently present in the index.
    fn contains(fingerprint: u64): bool {
        const cap = _.slots.len;
        const start = (fingerprint as usize) % cap;
        let probe: usize = 0;
        while (probe < cap) : (probe += 1) {
            const idx = (start + probe) % cap;
            if (!_.slots[idx].occupied) return false;
            if (_.slots[idx].fingerprint == fingerprint) return true;
        }
        return false;
    }

    /// Returns a snapshot of accumulated deduplication metrics.
    fn stats(): CdcDedupStats {
        return _.cur_stats;
    }

    /// Returns the byte deduplication savings ratio in `[0.0, 1.0]`.
    fn dedupRatio(): f64 {
        return _.cur_stats.dedupRatio();
    }
}
