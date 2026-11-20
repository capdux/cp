// Source: cp/lib/bpe/vocab.cp (Pure CP BPE Vocabulary, Merges & Tiktoken Loader)
const std = @import("std");
import { BpeError, SpecialToken, BpeMerge, TokenBytes } from "./types.cp";

pub fn makePairKey(left_id: u32, right_id: u32): u64 {
    return (@as(u64, left_id) << 32) | @as(u64, right_id);
}

pub struct BpeVocab {
    pub alloc: std.mem.Allocator,
    pub token_to_id: std.StringHashMap(u32),
    pub id_to_token: std.AutoHashMap(u32, TokenBytes),
    pub pair_ranks: std.AutoHashMap(u64, BpeMerge),
    pub special_tokens: std.ArrayList(SpecialToken),
    pub special_to_id: std.StringHashMap(u32),
    pub id_to_special: std.AutoHashMap(u32, TokenBytes),
    pub owned_slices: std.ArrayList([]u8),
    pub next_id: u32,

    pub fn init(alloc: std.mem.Allocator): BpeVocab {
        return BpeVocab{
            .alloc = alloc,
            .token_to_id = std.StringHashMap(u32).init(alloc),
            .id_to_token = std.AutoHashMap(u32, TokenBytes).init(alloc),
            .pair_ranks = std.AutoHashMap(u64, BpeMerge).init(alloc),
            .special_tokens = std.ArrayList(SpecialToken).empty,
            .special_to_id = std.StringHashMap(u32).init(alloc),
            .id_to_special = std.AutoHashMap(u32, TokenBytes).init(alloc),
            .owned_slices = std.ArrayList([]u8).empty,
            .next_id = 0,
        };
    }

    pub fn deinit(* _) {
        _.token_to_id.deinit();
        _.id_to_token.deinit();
        _.pair_ranks.deinit();
        _.special_tokens.deinit(_.alloc);
        _.special_to_id.deinit();
        _.id_to_special.deinit();
        for (slice of _.owned_slices.items) {
            _.alloc.free(slice);
        }
        _.owned_slices.deinit(_.alloc);
    }

    pub fn allocOwnedSlice(* _, bytes: []const u8): ![]const u8 {
        const copy = try _.alloc.dupe(u8, bytes);
        errdefer _.alloc.free(copy);
        try _.owned_slices.append(_.alloc, copy);
        return copy;
    }

    pub fn addToken(* _, bytes: []const u8, rank_id: u32): !void {
        if (_.token_to_id.contains(bytes)) {
            return;
        }
        const owned = try _.allocOwnedSlice(bytes);
        try _.token_to_id.put(owned, rank_id);
        try _.id_to_token.put(rank_id, TokenBytes{ .data = owned });
        if (rank_id >= _.next_id) {
            _.next_id = rank_id + 1;
        }
    }

    pub fn addByteTokens(* _): !void {
        let b: u32 = 0;
        while (b < 256) {
            let buf: [1]u8 = .{@intCast(b)};
            try _.addToken(&buf, b);
            b += 1;
        }
    }

    pub fn ensureToken(* _, bytes: []const u8): !u32 {
        if (existing_id := _.token_to_id.get(bytes)) {
            return existing_id;
        }
        const assigned_id = _.next_id;
        try _.addToken(bytes, assigned_id);
        return assigned_id;
    }

    pub fn addMerge(* _, left_bytes: []const u8, right_bytes: []const u8): !u32 {
        const left_id = try _.ensureToken(left_bytes);
        const right_id = try _.ensureToken(right_bytes);

        const merged_buf = try _.alloc.alloc(u8, left_bytes.len + right_bytes.len);
        defer _.alloc.free(merged_buf);
        @memcpy(merged_buf[0..left_bytes.len], left_bytes);
        @memcpy(merged_buf[left_bytes.len..], right_bytes);

        const merged_id = try _.ensureToken(merged_buf);
        const key = makePairKey(left_id, right_id);
        if (!_.pair_ranks.contains(key)) {
            const rank: u32 = @intCast(_.pair_ranks.count());
            try _.pair_ranks.put(key, BpeMerge{
                .left_id = left_id,
                .right_id = right_id,
                .merged_id = merged_id,
                .rank = rank,
            });
        }
        return merged_id;
    }

    pub fn addSpecialToken(* _, text: []const u8, id: u32): !void {
        if (_.special_to_id.contains(text)) {
            return;
        }
        const owned = try _.allocOwnedSlice(text);
        try _.special_tokens.append(_.alloc, SpecialToken{
            .text = owned,
            .id = id,
        });
        try _.special_to_id.put(owned, id);
        try _.id_to_special.put(id, TokenBytes{ .data = owned });
        if (id >= _.next_id) {
            _.next_id = id + 1;
        }
    }

    pub fn getTokenId(bytes: []const u8): u32? {
        return _.token_to_id.get(bytes);
    }

    pub fn getTokenBytes(id: u32): ([]const u8)? {
        if (bytes := _.id_to_token.get(id)) {
            return bytes.data;
        }
        if (st := _.id_to_special.get(id)) {
            return st.data;
        }
        return null;
    }

    pub fn getPairMerge(left_id: u32, right_id: u32): BpeMerge? {
        const key = makePairKey(left_id, right_id);
        return _.pair_ranks.get(key);
    }

    pub fn vocabSize(): usize {
        return _.token_to_id.count() + _.special_tokens.items.len;
    }
}

pub fn loadTiktokenText(alloc: std.mem.Allocator, content: []const u8): !BpeVocab {
    let vocab = BpeVocab.init(alloc);
    errdefer vocab.deinit();

    let lines = std.mem.splitScalar(u8, content, '\n');
    while (raw_line := lines.next()) {
        const line = std.mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0) {
            continue;
        }

        const space_idx = std.mem.indexOfScalar(u8, line, ' ') orelse {
            return BpeError.InvalidFormat;
        };
        const b64_part = line[0..space_idx];
        const rank_part = std.mem.trim(u8, line[space_idx + 1 ..], " \t");
        if (b64_part.len == 0 or rank_part.len == 0) {
            return BpeError.InvalidFormat;
        }

        const rank_id = std.fmt.parseInt(u32, rank_part, 10) catch {
            return BpeError.InvalidFormat;
        };

        const decoded_len = std.base64.standard.Decoder.calcSizeForSlice(b64_part) catch {
            return BpeError.InvalidBase64;
        };
        const decoded_buf = try alloc.alloc(u8, decoded_len);
        defer alloc.free(decoded_buf);

        std.base64.standard.Decoder.decode(decoded_buf, b64_part) catch {
            return BpeError.InvalidBase64;
        };

        try vocab.addToken(decoded_buf, rank_id);
    }

    return vocab;
}
