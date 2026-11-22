// Source: cp/lib/dedup/dedup.cp (SimHash, SimHashIndex, MinHash & LshIndex)
// Pure CP zero-C-dependency near-duplicate document fingerprinting and LSH engine.

const std = @import("std");

/// Error set for deduplication and LSH indexing operations.
pub err DedupError {
    InvalidThreshold,
    InvalidParams,
    OutOfMemory,
}

/// Match result returned by `SimHashIndex` queries.
pub struct SimHashHit {
    doc_id: string;
    fingerprint: u64;
    hamming_dist: u8;
}

/// Candidate match result returned by `LshIndex` queries.
pub struct LshHit {
    doc_id: string;
    estimated_jaccard: f64;
}

@inline fn splitmix64(x: u64): u64 {
    let z = x +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

@inline fn hashBytes64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    for (b of data) {
        h ^= b as u64;
        h = h *% 0x100000001b3;
    }
    return splitmix64(h ^ ((data.len as u64) *% 0x9e3779b97f4a7c15));
}

@inline fn hashTokenLower64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    for (b of data) {
        let lb = b;
        if (lb >= 'A' && lb <= 'Z') {
            lb = lb + 32;
        }
        h ^= lb as u64;
        h = h *% 0x100000001b3;
    }
    return splitmix64(h ^ ((data.len as u64) *% 0x9e3779b97f4a7c15));
}

@inline fn hashU64Slice(vals: []const u64, band_salt: u64): u64 {
    let h: u64 = @as(u64, 0xcbf29ce484222325) ^ splitmix64(band_salt);
    for (v of vals) {
        h = splitmix64(h ^ v);
    }
    return h;
}

@inline fn isWordByte(b: u8): bool {
    return (b >= 'a' && b <= 'z') ||
        (b >= 'A' && b <= 'Z') ||
        (b >= '0' && b <= '9') ||
        b == '_' ||
        b >= 0x80;
}

fn accumulateToken(v: *[64]i64, tok: []const u8) {
    const h = hashTokenLower64(tok);
    let b: usize = 0;
    while (b < 64) : (b += 1) {
        if (((h >> u6(b)) & 1) == 1) {
            v[b] += 1;
        } else {
            v[b] -= 1;
        }
    }
}

fn finalizeWeights(v: &[64]i64): u64 {
    let fp: u64 = 0;
    let b: usize = 0;
    while (b < 64) : (b += 1) {
        if (v[b] > 0) {
            fp |= (@as(u64, 1) << u6(b));
        }
    }
    return fp;
}

fn sortSimHashHits(arr: []SimHashHit) {
    if (arr.len < 2) {
        return;
    }
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key = arr[i];
        let j = i;
        while (j > 0 && arr[j - 1].hamming_dist > key.hamming_dist) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

fn sortLshHits(arr: []LshHit) {
    if (arr.len < 2) {
        return;
    }
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key = arr[i];
        let j = i;
        while (j > 0 && arr[j - 1].estimated_jaccard < key.estimated_jaccard) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

/// 64-bit Charikar SimHash fingerprinting and hardware-accelerated Hamming distance.
pub struct SimHash {
    /// Compute a 64-bit Charikar SimHash fingerprint from raw text by word tokenization.
    pub static fn computeText(text: string): u64 {
        let v: [64]i64 = [0; 64];
        let token_count: usize = 0;
        let i: usize = 0;
        while (i < text.len) {
            while (i < text.len && !isWordByte(text[i])) : (i += 1) {}
            if (i >= text.len) {
                break;
            }
            const start = i;
            while (i < text.len && isWordByte(text[i])) : (i += 1) {}
            accumulateToken(*v, text[start..i]);
            token_count += 1;
        }
        if (token_count == 0) {
            return 0;
        }
        return finalizeWeights(&v);
    }

    /// Compute a 64-bit SimHash fingerprint from a pre-tokenized slice of words.
    pub static fn computeTokens(tokens: []const string): u64 {
        if (tokens.len == 0) {
            return 0;
        }
        let v: [64]i64 = [0; 64];
        for (tok of tokens) {
            if (tok.len > 0) {
                accumulateToken(*v, tok);
            }
        }
        return finalizeWeights(&v);
    }

    /// Compute Hamming distance between two 64-bit fingerprints using `@popCount`.
    pub static fn hammingDistance(a: u64, b: u64): u8 {
        return @popCount(a ^ b) as u8;
    }
}

struct SimHashDoc {
    doc_id: string;
    fp: u64;
}

/// Sublinear 4x16-bit pigeonhole-partitioned SimHash near-duplicate index.
/// Guarantees 100% exact recall for Hamming distance `<= 3` without full-table scans.
pub struct SimHashIndex {
    alloc: std.mem.Allocator;
    docs: std.ArrayList(SimHashDoc) = .empty;
    bands: [4]std.AutoHashMapUnmanaged(u16, std.ArrayList(usize)) = [.empty, .empty, .empty, .empty];

    /// Initialize an empty `SimHashIndex`.
    pub static fn init(alloc: std.mem.Allocator): SimHashIndex {
        return SimHashIndex{
            alloc: alloc,
            docs: .empty,
            bands: [.empty, .empty, .empty, .empty],
        };
    }

    /// Release all owned document IDs, document records, and band bucket tables.
    pub fn deinit(*_) {
        for (doc of _.docs.items) {
            if (doc.doc_id.len > 0) {
                _.alloc.free(doc.doc_id);
            }
        }
        _.docs.deinit(_.alloc);

        let b: usize = 0;
        while (b < 4) : (b += 1) {
            let it = _.bands[b].valueIterator();
            while (list_ptr := it.next()) {
                list_ptr.deinit(_.alloc);
            }
            _.bands[b].deinit(_.alloc);
        }
    }

    /// Return the number of indexed documents.
    pub fn count(): usize {
        return _.docs.items.len;
    }

    /// Index a precomputed 64-bit SimHash `fp` under `doc_id`.
    pub fn addFingerprint(*_, doc_id: string, fp: u64): void ! DedupError {
        const owned_id = _.alloc.dupe(u8, doc_id) catch return DedupError.OutOfMemory;
        const doc_idx = _.docs.items.len;
        _.docs.append(_.alloc, SimHashDoc{
            doc_id: owned_id,
            fp: fp,
        }) catch {
            _.alloc.free(owned_id);
            return DedupError.OutOfMemory;
        };

        let b: usize = 0;
        while (b < 4) : (b += 1) {
            const key = u16((fp >> u6(b * 16)) & 0xFFFF);
            const gop = _.bands[b].getOrPut(_.alloc, key) catch return DedupError.OutOfMemory;
            if (!gop.found_existing) {
                gop.value_ptr.* = .empty;
            }
            gop.value_ptr.append(_.alloc, doc_idx) catch return DedupError.OutOfMemory;
        }
    }

    /// Compute SimHash for `text`, index it under `doc_id`, and return the fingerprint.
    pub fn add(*_, doc_id: string, text: string): u64 ! DedupError {
        const fp = SimHash.computeText(text);
        _.addFingerprint(doc_id, fp)?;
        return fp;
    }

    /// Query near-duplicates of a 64-bit fingerprint within `max_hamming_dist`, sorted ascending.
    pub fn queryFingerprint(*_, alloc: std.mem.Allocator, fp: u64, max_hamming_dist: u8): []SimHashHit ! DedupError {
        if (max_hamming_dist > 64) {
            return DedupError.InvalidThreshold;
        }
        if (_.docs.items.len == 0) {
            return alloc.alloc(SimHashHit, 0) catch return DedupError.OutOfMemory;
        }

        let hits: std.ArrayList(SimHashHit) = .empty;
        errdefer hits.deinit(alloc);

        if (max_hamming_dist <= 3) {
            let seen = alloc.alloc(bool, _.docs.items.len) catch return DedupError.OutOfMemory;
            defer alloc.free(seen);
            @memset(seen, false);

            let b: usize = 0;
            while (b < 4) : (b += 1) {
                const key = u16((fp >> u6(b * 16)) & 0xFFFF);
                if (bucket := _.bands[b].get(key)) {
                    for (doc_idx of bucket.items) {
                        if (!seen[doc_idx]) {
                            seen[doc_idx] = true;
                            const doc = _.docs.items[doc_idx];
                            const dist = SimHash.hammingDistance(fp, doc.fp);
                            if (dist <= max_hamming_dist) {
                                hits.append(alloc, SimHashHit{
                                    doc_id: doc.doc_id,
                                    fingerprint: doc.fp,
                                    hamming_dist: dist,
                                }) catch return DedupError.OutOfMemory;
                            }
                        }
                    }
                }
            }
        } else {
            for (doc of _.docs.items) {
                const dist = SimHash.hammingDistance(fp, doc.fp);
                if (dist <= max_hamming_dist) {
                    hits.append(alloc, SimHashHit{
                        doc_id: doc.doc_id,
                        fingerprint: doc.fp,
                        hamming_dist: dist,
                    }) catch return DedupError.OutOfMemory;
                }
            }
        }

        sortSimHashHits(hits.items);
        return hits.toOwnedSlice(alloc) catch return DedupError.OutOfMemory;
    }

    /// Query near-duplicates of `text` within `max_hamming_dist`, sorted by distance ascending.
    pub fn queryNearDuplicates(*_, alloc: std.mem.Allocator, text: string, max_hamming_dist: u8): []SimHashHit ! DedupError {
        const fp = SimHash.computeText(text);
        return _.queryFingerprint(alloc, fp, max_hamming_dist);
    }
}

/// MinHash k-shingle signature generator and Jaccard similarity estimator.
pub struct MinHash {
    /// Compute a `num_hashes`-element MinHash signature over character `k_shingle`s of `text`.
    pub static fn computeShingles(text: string, k_shingle: usize, num_hashes: usize, out_sig: []u64): void ! DedupError {
        if (k_shingle == 0 || num_hashes == 0 || out_sig.len < num_hashes) {
            return DedupError.InvalidParams;
        }
        const MAX_U64: u64 = 0xFFFFFFFFFFFFFFFF;
        @memset(out_sig[0..num_hashes], MAX_U64);

        if (text.len == 0) {
            return;
        }

        if (text.len <= k_shingle) {
            const base_h = hashBytes64(text);
            let j: usize = 0;
            while (j < num_hashes) : (j += 1) {
                const seed = (@as(u64, j) +% 1) *% 0x9e3779b97f4a7c15;
                const h = splitmix64(base_h ^ seed);
                if (h < out_sig[j]) {
                    out_sig[j] = h;
                }
            }
            return;
        }

        const limit = text.len - k_shingle + 1;
        let i: usize = 0;
        while (i < limit) : (i += 1) {
            const base_h = hashBytes64(text[i .. i + k_shingle]);
            let j: usize = 0;
            while (j < num_hashes) : (j += 1) {
                const seed = (@as(u64, j) +% 1) *% 0x9e3779b97f4a7c15;
                const h = splitmix64(base_h ^ seed);
                if (h < out_sig[j]) {
                    out_sig[j] = h;
                }
            }
        }
    }

    /// Estimate Jaccard similarity in `[0.0, 1.0]` from two equal-length MinHash signatures.
    pub static fn estimateJaccard(sig_a: []const u64, sig_b: []const u64): f64 ! DedupError {
        if (sig_a.len == 0 || sig_a.len != sig_b.len) {
            return DedupError.InvalidParams;
        }
        let matches: usize = 0;
        let i: usize = 0;
        while (i < sig_a.len) : (i += 1) {
            if (sig_a[i] == sig_b[i]) {
                matches += 1;
            }
        }
        return (matches as f64) / (sig_a.len as f64);
    }
}

struct LshDoc {
    doc_id: string;
    sig: []u64;
}

/// Locality-Sensitive Hashing (LSH) banding index over MinHash signatures.
pub struct LshIndex {
    alloc: std.mem.Allocator;
    bands: usize;
    rows_per_band: usize;
    num_hashes: usize;
    k_shingle: usize;
    docs: std.ArrayList(LshDoc) = .empty;
    band_buckets: []std.AutoHashMapUnmanaged(u64, std.ArrayList(usize));

    /// Initialize an `LshIndex` with `bands` bands and `rows_per_band` rows per band (default `k_shingle = 4`).
    pub static fn init(alloc: std.mem.Allocator, bands: usize, rows_per_band: usize): LshIndex ! DedupError {
        return LshIndex.initWithShingle(alloc, bands, rows_per_band, 4);
    }

    /// Initialize an `LshIndex` with an explicit `k_shingle` length.
    pub static fn initWithShingle(alloc: std.mem.Allocator, bands: usize, rows_per_band: usize, k_shingle: usize): LshIndex ! DedupError {
        if (bands == 0 || rows_per_band == 0 || k_shingle == 0) {
            return DedupError.InvalidParams;
        }
        const num_hashes = std.math.mul(usize, bands, rows_per_band) catch return DedupError.InvalidParams;
        let buckets = alloc.alloc(std.AutoHashMapUnmanaged(u64, std.ArrayList(usize)), bands) catch return DedupError.OutOfMemory;
        for (*bucket of buckets) {
            bucket.* = .empty;
        }
        return LshIndex{
            alloc: alloc,
            bands: bands,
            rows_per_band: rows_per_band,
            num_hashes: num_hashes,
            k_shingle: k_shingle,
            docs: .empty,
            band_buckets: buckets,
        };
    }

    /// Release all owned signatures, document IDs, and LSH band buckets.
    pub fn deinit(*_) {
        for (doc of _.docs.items) {
            if (doc.doc_id.len > 0) {
                _.alloc.free(doc.doc_id);
            }
            if (doc.sig.len > 0) {
                _.alloc.free(doc.sig);
            }
        }
        _.docs.deinit(_.alloc);

        let b: usize = 0;
        while (b < _.bands) : (b += 1) {
            let it = _.band_buckets[b].valueIterator();
            while (list_ptr := it.next()) {
                list_ptr.deinit(_.alloc);
            }
            _.band_buckets[b].deinit(_.alloc);
        }
        _.alloc.free(_.band_buckets);
    }

    /// Return the number of indexed documents.
    pub fn count(): usize {
        return _.docs.items.len;
    }

    /// Compute MinHash signature for `text` and insert it into all LSH band buckets under `doc_id`.
    pub fn insert(*_, doc_id: string, text: string): void ! DedupError {
        const owned_id = _.alloc.dupe(u8, doc_id) catch return DedupError.OutOfMemory;
        let sig = _.alloc.alloc(u64, _.num_hashes) catch {
            _.alloc.free(owned_id);
            return DedupError.OutOfMemory;
        };
        MinHash.computeShingles(text, _.k_shingle, _.num_hashes, sig) catch e => {
            _.alloc.free(owned_id);
            _.alloc.free(sig);
            return e;
        };

        const doc_idx = _.docs.items.len;
        _.docs.append(_.alloc, LshDoc{
            doc_id: owned_id,
            sig: sig,
        }) catch {
            _.alloc.free(owned_id);
            _.alloc.free(sig);
            return DedupError.OutOfMemory;
        };

        let b: usize = 0;
        while (b < _.bands) : (b += 1) {
            const start = b * _.rows_per_band;
            const band_hash = hashU64Slice(sig[start .. start + _.rows_per_band], @as(u64, b));
            const gop = _.band_buckets[b].getOrPut(_.alloc, band_hash) catch return DedupError.OutOfMemory;
            if (!gop.found_existing) {
                gop.value_ptr.* = .empty;
            }
            gop.value_ptr.append(_.alloc, doc_idx) catch return DedupError.OutOfMemory;
        }
    }

    /// Query LSH band buckets for candidate documents with `estimated_jaccard >= min_jaccard`, sorted descending.
    pub fn queryCandidates(*_, alloc: std.mem.Allocator, text: string, min_jaccard: f64): []LshHit ! DedupError {
        if (std.math.isNan(min_jaccard) || min_jaccard < 0.0 || min_jaccard > 1.0) {
            return DedupError.InvalidThreshold;
        }
        if (_.docs.items.len == 0) {
            return alloc.alloc(LshHit, 0) catch return DedupError.OutOfMemory;
        }

        let q_sig = alloc.alloc(u64, _.num_hashes) catch return DedupError.OutOfMemory;
        defer alloc.free(q_sig);
        MinHash.computeShingles(text, _.k_shingle, _.num_hashes, q_sig)?;

        let seen = alloc.alloc(bool, _.docs.items.len) catch return DedupError.OutOfMemory;
        defer alloc.free(seen);
        @memset(seen, false);

        let hits: std.ArrayList(LshHit) = .empty;
        errdefer hits.deinit(alloc);

        let b: usize = 0;
        while (b < _.bands) : (b += 1) {
            const start = b * _.rows_per_band;
            const band_hash = hashU64Slice(q_sig[start .. start + _.rows_per_band], @as(u64, b));
            if (bucket := _.band_buckets[b].get(band_hash)) {
                for (doc_idx of bucket.items) {
                    if (!seen[doc_idx]) {
                        seen[doc_idx] = true;
                        const doc = _.docs.items[doc_idx];
                        const est = MinHash.estimateJaccard(q_sig, doc.sig)?;
                        if (est >= min_jaccard) {
                            hits.append(alloc, LshHit{
                                doc_id: doc.doc_id,
                                estimated_jaccard: est,
                            }) catch return DedupError.OutOfMemory;
                        }
                    }
                }
            }
        }

        sortLshHits(hits.items);
        return hits.toOwnedSlice(alloc) catch return DedupError.OutOfMemory;
    }
}
