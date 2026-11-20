// Source: cp/lib/bpe/tokenizer.cp (Pure CP Byte-Level BPE Tokenizer & Budget Engine)
const std = @import("std");
import { BpeError, BpeMerge } from "./types.cp";
import { BpeVocab, makePairKey, loadTiktokenText } from "./vocab.cp";

pub struct BpeTokenizer {
    pub vocab: BpeVocab,

    pub fn init(alloc: std.mem.Allocator): BpeTokenizer {
        return BpeTokenizer{
            .vocab = BpeVocab.init(alloc),
        };
    }

    pub fn fromVocab(vocab: BpeVocab): BpeTokenizer {
        return BpeTokenizer{
            .vocab = vocab,
        };
    }

    pub fn deinit(* _) {
        _.vocab.deinit();
    }

    pub fn loadTiktoken(alloc: std.mem.Allocator, content: []const u8): !BpeTokenizer {
        const vocab = try loadTiktokenText(alloc, content);
        return BpeTokenizer{
            .vocab = vocab,
        };
    }

    pub fn trainFromCorpus(alloc: std.mem.Allocator, text: []const u8, target_vocab_size: usize): !BpeTokenizer {
        if (target_vocab_size < 256) {
            return BpeError.InvalidTargetVocabSize;
        }
        let vocab = BpeVocab.init(alloc);
        errdefer vocab.deinit();
        try vocab.addByteTokens();

        let ids = std.ArrayList(u32).empty;
        defer ids.deinit(alloc);
        for (byte of text) {
            try ids.append(alloc, @as(u32, byte));
        }

        while (vocab.vocabSize() < target_vocab_size and ids.items.len >= 2) {
            let counts = std.AutoHashMap(u64, usize).init(alloc);
            defer counts.deinit();

            let best_key: u64 = 0;
            let best_count: usize = 0;
            let best_left: u32 = 0;
            let best_right: u32 = 0;

            let i: usize = 0;
            while (i + 1 < ids.items.len) {
                const l = ids.items[i];
                const r = ids.items[i + 1];
                const key = makePairKey(l, r);
                const prev = counts.get(key) orelse @as(usize, 0);
                const next_count = prev + @as(usize, 1);
                try counts.put(key, next_count);
                if (next_count > best_count) {
                    best_count = next_count;
                    best_key = key;
                    best_left = l;
                    best_right = r;
                }
                i += 1;
            }

            if (best_count < @as(usize, 2)) {
                break;
            }

            const left_bytes = vocab.getTokenBytes(best_left) orelse break;
            const right_bytes = vocab.getTokenBytes(best_right) orelse break;
            const merged_id = try vocab.addMerge(left_bytes, right_bytes);

            let next_ids = std.ArrayList(u32).empty;
            errdefer next_ids.deinit(alloc);

            let idx: usize = 0;
            while (idx < ids.items.len) {
                if (idx + 1 < ids.items.len and ids.items[idx] == best_left and ids.items[idx + 1] == best_right) {
                    try next_ids.append(alloc, merged_id);
                    idx += 2;
                } else {
                    try next_ids.append(alloc, ids.items[idx]);
                    idx += 1;
                }
            }

            ids.deinit(alloc);
            ids = next_ids;
        }

        return BpeTokenizer{
            .vocab = vocab,
        };
    }

    fn findBestPair(
        alloc: std.mem.Allocator,
        ids: []const u32,
        out_idx: *usize,
        out_merged_id: *u32,
    ): !bool {
        if (ids.len < 2) {
            return false;
        }

        let found = false;
        let best_rank: u32 = 0;
        let best_idx: usize = 0;
        let best_merged: u32 = 0;

        let i: usize = 0;
        while (i + 1 < ids.len) {
            const left_id = ids[i];
            const right_id = ids[i + 1];

            if (m := _.vocab.getPairMerge(left_id, right_id)) {
                if (!found or m.rank < best_rank) {
                    found = true;
                    best_rank = m.rank;
                    best_idx = i;
                    best_merged = m.merged_id;
                }
                i += 1;
                continue;
            }

            const left_bytes = _.vocab.getTokenBytes(left_id);
            const right_bytes = _.vocab.getTokenBytes(right_id);
            if (left_bytes != null and right_bytes != null) {
                const lb = left_bytes.?;
                const rb = right_bytes.?;
                const combined = try alloc.alloc(u8, lb.len + rb.len);
                defer alloc.free(combined);
                @memcpy(combined[0..lb.len], lb);
                @memcpy(combined[lb.len..], rb);

                if (merged_id := _.vocab.getTokenId(combined)) {
                    if (!found or merged_id < best_rank) {
                        found = true;
                        best_rank = merged_id;
                        best_idx = i;
                        best_merged = merged_id;
                    }
                }
            }
            i += 1;
        }

        if (found) {
            out_idx.* = best_idx;
            out_merged_id.* = best_merged;
        }
        return found;
    }

    pub fn encodeOrdinary(alloc: std.mem.Allocator, text: []const u8): ![]u32 {
        let ids = std.ArrayList(u32).empty;
        errdefer ids.deinit(alloc);

        for (byte of text) {
            let buf: [1]u8 = .{byte};
            const tid = _.vocab.getTokenId(&buf) orelse {
                return BpeError.UnknownTokenId;
            };
            try ids.append(alloc, tid);
        }

        while (ids.items.len >= 2) {
            let merge_idx: usize = 0;
            let merged_id: u32 = 0;
            const has_pair = try _.findBestPair(alloc, ids.items, &merge_idx, &merged_id);
            if (!has_pair) {
                break;
            }

            const target_left = ids.items[merge_idx];
            const target_right = ids.items[merge_idx + 1];

            let next_ids = std.ArrayList(u32).empty;
            errdefer next_ids.deinit(alloc);

            let i: usize = 0;
            while (i < ids.items.len) {
                if (i + 1 < ids.items.len and ids.items[i] == target_left and ids.items[i + 1] == target_right) {
                    try next_ids.append(alloc, merged_id);
                    i += 2;
                } else {
                    try next_ids.append(alloc, ids.items[i]);
                    i += 1;
                }
            }

            ids.deinit(alloc);
            ids = next_ids;
        }

        return ids.toOwnedSlice(alloc);
    }

    pub fn encode(alloc: std.mem.Allocator, text: []const u8): ![]u32 {
        if (_.vocab.special_tokens.items.len == 0) {
            return _.encodeOrdinary(alloc, text);
        }

        let result = std.ArrayList(u32).empty;
        errdefer result.deinit(alloc);

        let pos: usize = 0;
        let chunk_start: usize = 0;

        while (pos < text.len) {
            let matched_len: usize = 0;
            let matched_id: u32 = 0;

            for (st of _.vocab.special_tokens.items) {
                if (st.text.len > matched_len and pos + st.text.len <= text.len) {
                    if (std.mem.eql(u8, text[pos .. pos + st.text.len], st.text)) {
                        matched_len = st.text.len;
                        matched_id = st.id;
                    }
                }
            }

            if (matched_len > 0) {
                if (pos > chunk_start) {
                    const sub_ids = try _.encodeOrdinary(alloc, text[chunk_start..pos]);
                    defer alloc.free(sub_ids);
                    for (id of sub_ids) {
                        try result.append(alloc, id);
                    }
                }
                try result.append(alloc, matched_id);
                pos += matched_len;
                chunk_start = pos;
            } else {
                pos += 1;
            }
        }

        if (chunk_start < text.len) {
            const tail_ids = try _.encodeOrdinary(alloc, text[chunk_start..]);
            defer alloc.free(tail_ids);
            for (id of tail_ids) {
                try result.append(alloc, id);
            }
        }

        return result.toOwnedSlice(alloc);
    }

    pub fn decode(alloc: std.mem.Allocator, token_ids: []const u32): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        for (id of token_ids) {
            const bytes = _.vocab.getTokenBytes(id) orelse {
                return BpeError.UnknownTokenId;
            };
            try out.appendSlice(alloc, bytes);
        }

        return out.toOwnedSlice(alloc);
    }

    pub fn countTokens(alloc: std.mem.Allocator, text: []const u8): !usize {
        const ids = try _.encode(alloc, text);
        defer alloc.free(ids);
        return ids.len;
    }

    pub fn truncateToMaxTokens(alloc: std.mem.Allocator, text: []const u8, max_tokens: usize): ![]u8 {
        if (max_tokens == 0) {
            return alloc.dupe(u8, "");
        }

        const ids = try _.encode(alloc, text);
        defer alloc.free(ids);

        if (ids.len <= max_tokens) {
            return alloc.dupe(u8, text);
        }

        const raw = try _.decode(alloc, ids[0..max_tokens]);
        errdefer alloc.free(raw);

        const safe_len = validUtf8PrefixLen(raw);
        if (safe_len == raw.len) {
            return raw;
        }
        const safe_slice = try alloc.dupe(u8, raw[0..safe_len]);
        alloc.free(raw);
        return safe_slice;
    }
}

pub fn validUtf8PrefixLen(bytes: []const u8): usize {
    if (bytes.len == 0) {
        return 0;
    }
    let idx = bytes.len;
    let steps: usize = 0;
    while (idx > 0 and steps < 4) {
        idx -= 1;
        steps += 1;
        const b = bytes[idx];
        if ((b & 0x80) == 0) {
            return idx + 1;
        }
        if ((b & 0xC0) != 0x80) {
            const seq_len = std.unicode.utf8ByteSequenceLength(b) catch {
                return idx;
            };
            if (idx + seq_len <= bytes.len) {
                return idx + seq_len;
            }
            return idx;
        }
    }
    return bytes.len - steps;
}
