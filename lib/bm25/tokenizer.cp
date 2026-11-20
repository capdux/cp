// Source: cp/lib/bm25/tokenizer.cp (BM25 Tokenizer & Stopwords)
const std = @import("std");

pub struct TokenizerConfig {
    lowercase: bool = true,
    filter_stopwords: bool = true,
    min_len: usize = 1,
    max_len: usize = 64,
    custom_stopwords: []const []const u8 = &.{},
}

const DEFAULT_STOPWORDS: [25][]const u8 = [
    "a",    "an",   "and",  "are",  "as",   "at",   "be",   "by",
    "for",  "from", "has",  "he",   "in",   "is",   "it",   "its",
    "of",   "on",   "that", "the",  "to",   "was",  "were", "will",
    "with",
];

pub fn defaultStopwords(): []const []const u8 {
    return &DEFAULT_STOPWORDS;
}

fn isTokenByte(b: u8): bool {
    if (b >= 0x80) {
        return true;
    }
    if ((b >= 'a' and b <= 'z') or (b >= 'A' and b <= 'Z')) {
        return true;
    }
    if ((b >= '0' and b <= '9') or b == '_') {
        return true;
    }
    return false;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' and b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn isStopword(word: []const u8, config: TokenizerConfig): bool {
    if (!config.filter_stopwords) {
        return false;
    }
    for (sw of DEFAULT_STOPWORDS) {
        if (std.mem.eql(u8, word, sw)) {
            return true;
        }
    }
    for (sw of config.custom_stopwords) {
        if (std.mem.eql(u8, word, sw)) {
            return true;
        }
    }
    return false;
}

pub fn lowercaseTerm(alloc: std.mem.Allocator, term: []const u8, lowercase: bool): ![]u8 {
    let buf = try alloc.alloc(u8, term.len);
    let i: usize = 0;
    for (b of term) {
        buf[i] = if (lowercase) toLowerAscii(b) else b;
        i += 1;
    }
    return buf;
}

pub fn tokenizeTerms(alloc: std.mem.Allocator, text: []const u8, config: TokenizerConfig): ![][]u8 {
    let tokens = std.ArrayList([]u8).empty;
    errdefer {
        for (tok of tokens.items) {
            alloc.free(tok);
        }
        tokens.deinit(alloc);
    }

    let i: usize = 0;
    while (i < text.len) {
        while (i < text.len and !isTokenByte(text[i])) {
            i += 1;
        }
        if (i >= text.len) {
            break;
        }
        const start = i;
        while (i < text.len and isTokenByte(text[i])) {
            i += 1;
        }
        const raw = text[start..i];
        if (raw.len < config.min_len) {
            continue;
        }
        if (config.max_len > 0 and raw.len > config.max_len) {
            continue;
        }

        const tok = try lowercaseTerm(alloc, raw, config.lowercase);
        if (isStopword(tok, config)) {
            alloc.free(tok);
            continue;
        }
        tokens.append(alloc, tok) catch |e| {
            alloc.free(tok);
            return e;
        };
    }

    return tokens.toOwnedSlice(alloc);
}

pub fn freeTerms(alloc: std.mem.Allocator, terms: [][]u8) {
    for (tok of terms) {
        alloc.free(tok);
    }
    alloc.free(terms);
}
