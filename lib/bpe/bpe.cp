// Source: cp/lib/bpe/bpe.cp (Byte-Level BPE Tokenizer Engine)
// Pure CP zero-C-dependency Byte Pair Encoding (BPE) tokenizer with 256 lossless
// base byte tokens, deterministic frequency-ranked training, merge-rank encoding,
// lossless UTF-8 decoding, and longest-match special token protection.

const std = @import("std");

const SENTINEL_ID: u32 = std.math.maxInt(u32);

/// Error set for BPE tokenizer initialization, training, encoding, and decoding.
pub err BpeError {
    InvalidVocabSize,
    UnknownTokenId,
    DuplicateSpecialToken,
    InvalidMergeRule,
    OutOfMemory,
}

/// Learned or manually loaded BPE merge rule with deterministic priority rank.
pub struct MergePair {
    left: u32;
    right: u32;
    new_id: u32;
    rank: u32;
}

const MergeVal = struct {
    new_id: u32;
    rank: u32;
};

const SpecialToken = struct {
    text: []const u8;
    id: u32;
};

@inline
fn packPair(left: u32, right: u32): u64 {
    return (u64(left) << 32) | u64(right);
}

@inline
fn unpackLeft(key: u64): u32 {
    return (key >> 32) as u32;
}

@inline
fn unpackRight(key: u64): u32 {
    return (key & 0xFFFF_FFFF) as u32;
}

/// Industrial-grade Byte-Level BPE Tokenizer with 256 lossless base bytes (0..255),
/// special token splitting protection, deterministic BPE training, and rank-ordered encoding.
pub struct BpeTokenizer {
    alloc: std.mem.Allocator;
    owned_bufs: std.ArrayList([]u8) = .empty;
    vocab: std.ArrayList(string) = .empty;
    merges: std.ArrayList(MergePair) = .empty;
    merge_map: std.AutoHashMapUnmanaged(u64, MergeVal) = .empty;
    special_tokens: std.ArrayList(SpecialToken) = .empty;
    special_set: std.StringHashMapUnmanaged(u32) = .empty;
    special_id_set: std.AutoHashMapUnmanaged(u32, void) = .empty;

    /// Initialize a `BpeTokenizer` pre-populated with all 256 single-byte tokens (IDs 0..255).
    pub static fn init(alloc: std.mem.Allocator): BpeTokenizer ! BpeError {
        let tok = BpeTokenizer{
            alloc: alloc,
            owned_bufs: .empty,
            vocab: .empty,
            merges: .empty,
            merge_map: .empty,
            special_tokens: .empty,
            special_set: .empty,
            special_id_set: .empty,
        };
        errdefer tok.deinit();

        tok.vocab.ensureTotalCapacity(alloc, 256) catch return BpeError.OutOfMemory;
        let base = alloc.alloc(u8, 256) catch return BpeError.OutOfMemory;
        tok.owned_bufs.append(alloc, base) catch {
            alloc.free(base);
            return BpeError.OutOfMemory;
        };

        let i: usize = 0;
        while (i < 256) : (i += 1) {
            base[i] = i as u8;
            tok.vocab.appendAssumeCapacity(base[i..i + 1]);
        }

        return tok;
    }

    /// Release all vocabulary buffers, merge tables, and special token indices.
    pub fn deinit(*_) {
        _.special_id_set.deinit(_.alloc);
        _.special_set.deinit(_.alloc);
        _.special_tokens.deinit(_.alloc);
        _.merge_map.deinit(_.alloc);
        _.merges.deinit(_.alloc);
        _.vocab.deinit(_.alloc);
        for (buf of _.owned_bufs.items) {
            if (buf.len > 0) {
                _.alloc.free(buf);
            }
        }
        _.owned_bufs.deinit(_.alloc);
    }

    /// Return the total vocabulary size (base 256 bytes + merges + special tokens).
    pub fn vocabSize(): usize {
        return _.vocab.items.len;
    }

    /// Return the raw byte slice for `id`, or `null` if `id` is out of range.
    pub fn tokenBytes(id: u32): ([]const u8)? {
        const idx = usize(id);
        if (idx >= _.vocab.items.len) {
            return null;
        }
        return _.vocab.items[idx];
    }

    /// Register a protected special token (e.g. `<|endoftext|>`, `<|im_start|>`).
    /// Returns `DuplicateSpecialToken` if already registered.
    pub fn addSpecialToken(*_, token_str: string): u32 ! BpeError {
        if (token_str.len == 0) {
            return BpeError.InvalidMergeRule;
        }
        if (_.special_set.contains(token_str)) {
            return BpeError.DuplicateSpecialToken;
        }
        if (_.vocab.items.len >= usize(SENTINEL_ID)) {
            return BpeError.OutOfMemory;
        }

        const copy = _.alloc.alloc(u8, token_str.len) catch return BpeError.OutOfMemory;
        @memcpy(copy, token_str);

        _.owned_bufs.append(_.alloc, copy) catch {
            _.alloc.free(copy);
            return BpeError.OutOfMemory;
        };

        const new_id: u32 = _.vocab.items.len as u32;
        _.vocab.append(_.alloc, copy) catch return BpeError.OutOfMemory;
        _.special_set.put(_.alloc, copy, new_id) catch {
            * = _.vocab.pop();
            return BpeError.OutOfMemory;
        };
        _.special_id_set.put(_.alloc, new_id, {}) catch {
            * = _.special_set.remove(copy);
            * = _.vocab.pop();
            return BpeError.OutOfMemory;
        };
        _.special_tokens.append(_.alloc, SpecialToken{ text: copy, id: new_id }) catch {
            * = _.special_id_set.remove(new_id);
            * = _.special_set.remove(copy);
            * = _.vocab.pop();
            return BpeError.OutOfMemory;
        };

        return new_id;
    }

    /// Manually register a BPE merge rule `(left_id, right_id) -> new_id` with next sequential rank.
    pub fn addMerge(*_, left_id: u32, right_id: u32): u32 ! BpeError {
        if (usize(left_id) >= _.vocab.items.len || usize(right_id) >= _.vocab.items.len) {
            return BpeError.InvalidMergeRule;
        }
        if (_.special_id_set.contains(left_id) || _.special_id_set.contains(right_id)) {
            return BpeError.InvalidMergeRule;
        }
        const key = packPair(left_id, right_id);
        if (_.merge_map.contains(key)) {
            return BpeError.InvalidMergeRule;
        }
        if (_.vocab.items.len >= usize(SENTINEL_ID)) {
            return BpeError.OutOfMemory;
        }

        const left_bytes = _.vocab.items[usize(left_id)];
        const right_bytes = _.vocab.items[usize(right_id)];
        const merged_len = left_bytes.len + right_bytes.len;
        const merged = _.alloc.alloc(u8, merged_len) catch return BpeError.OutOfMemory;
        @memcpy(merged[0..left_bytes.len], left_bytes);
        @memcpy(merged[left_bytes.len..merged_len], right_bytes);

        _.owned_bufs.append(_.alloc, merged) catch {
            _.alloc.free(merged);
            return BpeError.OutOfMemory;
        };

        const new_id: u32 = _.vocab.items.len as u32;
        const rank: u32 = _.merges.items.len as u32;

        _.vocab.append(_.alloc, merged) catch return BpeError.OutOfMemory;
        _.merges.append(_.alloc, MergePair{
            left: left_id,
            right: right_id,
            new_id: new_id,
            rank: rank,
        }) catch {
            * = _.vocab.pop();
            return BpeError.OutOfMemory;
        };
        _.merge_map.put(_.alloc, key, MergeVal{ new_id: new_id, rank: rank }) catch {
            * = _.merges.pop();
            * = _.vocab.pop();
            return BpeError.OutOfMemory;
        };

        return new_id;
    }

    /// Train BPE merge rules on `corpus` until `vocabSize() >= target_vocab_size`
    /// or no adjacent pair appears at least twice.
    pub fn train(*_, corpus: string, target_vocab_size: usize): void ! BpeError {
        if (target_vocab_size < 256) {
            return BpeError.InvalidVocabSize;
        }
        if (_.vocab.items.len >= target_vocab_size || corpus.len < 2) {
            return;
        }

        let tokens: std.ArrayList(u32) = .empty;
        defer tokens.deinit(_.alloc);

        // Seed training token stream, isolating any registered special tokens with SENTINEL_ID.
        if (_.special_tokens.items.len == 0) {
            tokens.ensureTotalCapacity(_.alloc, corpus.len) catch return BpeError.OutOfMemory;
            for (b of corpus) {
                tokens.appendAssumeCapacity(u32(b));
            }
        } else {
            let pos: usize = 0;
            while (pos < corpus.len) {
                if (sp := _.matchSpecialAt(corpus, pos)) {
                    if (tokens.items.len > 0 && tokens.items[tokens.items.len - 1] != SENTINEL_ID) {
                        tokens.append(_.alloc, SENTINEL_ID) catch return BpeError.OutOfMemory;
                    }
                    pos += sp.text.len;
                } else {
                    tokens.append(_.alloc, u32(corpus[pos])) catch return BpeError.OutOfMemory;
                    pos += 1;
                }
            }
        }

        let counts: std.AutoHashMapUnmanaged(u64, usize) = .empty;
        defer counts.deinit(_.alloc);

        while (_.vocab.items.len < target_vocab_size && tokens.items.len >= 2) {
            counts.clearRetainingCapacity();

            let i: usize = 0;
            while (i + 1 < tokens.items.len) : (i += 1) {
                const a = tokens.items[i];
                const b = tokens.items[i + 1];
                if (a == SENTINEL_ID || b == SENTINEL_ID) {
                    continue;
                }
                const key = packPair(a, b);
                if (_.merge_map.contains(key)) {
                    continue;
                }
                const prev = counts.get(key) ?? 0;
                counts.put(_.alloc, key, prev + 1) catch return BpeError.OutOfMemory;
            }

            let best_count: usize = 0;
            let best_left: u32 = 0;
            let best_right: u32 = 0;
            let found = false;

            let iter = counts.iterator();
            while (entry := iter.next()) {
                const cnt = entry.value_ptr.*;
                if (cnt < 2) {
                    continue;
                }
                const k = entry.key_ptr.*;
                const l = unpackLeft(k);
                const r = unpackRight(k);
                if (!found || cnt > best_count || (cnt == best_count && (l < best_left || (l == best_left && r < best_right)))) {
                    best_count = cnt;
                    best_left = l;
                    best_right = r;
                    found = true;
                }
            }

            if (!found) {
                break;
            }

            const new_id = _.addMerge(best_left, best_right)?;

            // Replace all non-overlapping occurrences of (best_left, best_right) in-place.
            let read_idx: usize = 0;
            let write_idx: usize = 0;
            while (read_idx < tokens.items.len) {
                if (read_idx + 1 < tokens.items.len && tokens.items[read_idx] == best_left && tokens.items[read_idx + 1] == best_right) {
                    tokens.items[write_idx] = new_id;
                    write_idx += 1;
                    read_idx += 2;
                } else {
                    tokens.items[write_idx] = tokens.items[read_idx];
                    write_idx += 1;
                    read_idx += 1;
                }
            }
            tokens.items.len = write_idx;
        }
    }

    fn matchSpecialAt(text: string, pos: usize): SpecialToken? {
        let best: SpecialToken? = null;
        let best_len: usize = 0;
        for (sp of _.special_tokens.items) {
            const slen = sp.text.len;
            if (slen > best_len && pos + slen <= text.len) {
                if (std.mem.eql(u8, text[pos..pos + slen], sp.text)) {
                    best = sp;
                    best_len = slen;
                }
            }
        }
        return best;
    }

    fn encodePlainSpan(alloc: std.mem.Allocator, span: []const u8, out: *std.ArrayList(u32)): void ! BpeError {
        if (span.len == 0) {
            return;
        }
        if (span.len == 1 || _.merges.items.len == 0) {
            out.ensureUnusedCapacity(alloc, span.len) catch return BpeError.OutOfMemory;
            for (b of span) {
                out.appendAssumeCapacity(u32(b));
            }
            return;
        }

        let work: std.ArrayList(u32) = .empty;
        defer work.deinit(alloc);
        work.ensureTotalCapacity(alloc, span.len) catch return BpeError.OutOfMemory;
        for (b of span) {
            work.appendAssumeCapacity(u32(b));
        }

        while (work.items.len >= 2) {
            let best_rank: u32 = SENTINEL_ID;
            let best_left: u32 = 0;
            let best_right: u32 = 0;
            let best_new_id: u32 = 0;
            let found = false;

            let i: usize = 0;
            while (i + 1 < work.items.len) : (i += 1) {
                const a = work.items[i];
                const b = work.items[i + 1];
                const key = packPair(a, b);
                if (mv := _.merge_map.get(key)) {
                    if (!found || mv.rank < best_rank) {
                        best_rank = mv.rank;
                        best_left = a;
                        best_right = b;
                        best_new_id = mv.new_id;
                        found = true;
                    }
                }
            }

            if (!found) {
                break;
            }

            let read_idx: usize = 0;
            let write_idx: usize = 0;
            while (read_idx < work.items.len) {
                if (read_idx + 1 < work.items.len && work.items[read_idx] == best_left && work.items[read_idx + 1] == best_right) {
                    work.items[write_idx] = best_new_id;
                    write_idx += 1;
                    read_idx += 2;
                } else {
                    work.items[write_idx] = work.items[read_idx];
                    write_idx += 1;
                    read_idx += 1;
                }
            }
            work.items.len = write_idx;
        }

        out.appendSlice(alloc, work.items) catch return BpeError.OutOfMemory;
    }

    /// Encode `text` into a newly allocated `[]u32` token ID slice using `alloc`.
    /// Registered special tokens are matched greedily (longest-first) and never split.
    /// Caller owns the returned slice and must free it with `alloc.free(ids)`.
    pub fn encode(alloc: std.mem.Allocator, text: string): []u32 ! BpeError {
        let out: std.ArrayList(u32) = .empty;
        errdefer out.deinit(alloc);

        if (text.len == 0) {
            return out.toOwnedSlice(alloc) catch return BpeError.OutOfMemory;
        }

        if (_.special_tokens.items.len == 0) {
            _.encodePlainSpan(alloc, text, *out)?;
            return out.toOwnedSlice(alloc) catch return BpeError.OutOfMemory;
        }

        let pos: usize = 0;
        let plain_start: usize = 0;
        while (pos < text.len) {
            if (sp := _.matchSpecialAt(text, pos)) {
                if (pos > plain_start) {
                    _.encodePlainSpan(alloc, text[plain_start..pos], *out)?;
                }
                out.append(alloc, sp.id) catch return BpeError.OutOfMemory;
                pos += sp.text.len;
                plain_start = pos;
            } else {
                pos += 1;
            }
        }
        if (text.len > plain_start) {
            _.encodePlainSpan(alloc, text[plain_start..text.len], *out)?;
        }

        return out.toOwnedSlice(alloc) catch return BpeError.OutOfMemory;
    }

    /// Decode `ids` back into a newly allocated lossless `[]u8` byte slice using `alloc`.
    /// Returns `UnknownTokenId` if any ID is outside `[0, vocabSize())`.
    /// Caller owns the returned slice and must free it with `alloc.free(bytes)`.
    pub fn decode(alloc: std.mem.Allocator, ids: []const u32): []u8 ! BpeError {
        let total_len: usize = 0;
        for (id of ids) {
            const idx = usize(id);
            if (idx >= _.vocab.items.len) {
                return BpeError.UnknownTokenId;
            }
            total_len += _.vocab.items[idx].len;
        }

        const out = alloc.alloc(u8, total_len) catch return BpeError.OutOfMemory;
        let offset: usize = 0;
        for (id of ids) {
            const tok_bytes = _.vocab.items[usize(id)];
            if (tok_bytes.len > 0) {
                @memcpy(out[offset..offset + tok_bytes.len], tok_bytes);
                offset += tok_bytes.len;
            }
        }
        return out;
    }

    /// Convenience helper returning the number of tokens produced by `encode(alloc, text)`.
    pub fn countTokens(alloc: std.mem.Allocator, text: string): usize ! BpeError {
        const ids = _.encode(alloc, text)?;
        defer alloc.free(ids);
        return ids.len;
    }
}
