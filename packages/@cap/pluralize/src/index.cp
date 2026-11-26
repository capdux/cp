// @cap/pluralize — Zero-allocation English Pluralization, Singularization & Ordinal Engine
// Clean-room pure-CP implementation modeled after npm `pluralize`, `inflection`, and Rust `pluralizer` (MIT License).

const std = @import("std");

const MAX_WORD_LEN: usize = 256;

const IRREGULAR_SINGULAR: [56][]const u8 = [
    "child", "person", "man", "woman", "tooth", "foot", "goose", "mouse",
    "louse", "ox", "leaf", "knife", "wife", "wolf", "life", "shelf",
    "half", "loaf", "calf", "elf", "thief", "self", "sheaf", "scarf",
    "cactus", "focus", "fungus", "nucleus", "radius", "stimulus", "alumnus", "syllabus",
    "analysis", "basis", "crisis", "diagnosis", "thesis", "hypothesis", "parenthesis", "axis",
    "criterion", "phenomenon", "datum", "medium", "bacterium", "curriculum", "index", "matrix",
    "vertex", "appendix", "quiz", "hero", "potato", "tomato", "echo", "die",
];

const IRREGULAR_PLURAL: [56][]const u8 = [
    "children", "people", "men", "women", "teeth", "feet", "geese", "mice",
    "lice", "oxen", "leaves", "knives", "wives", "wolves", "lives", "shelves",
    "halves", "loaves", "calves", "elves", "thieves", "selves", "sheaves", "scarves",
    "cacti", "foci", "fungi", "nuclei", "radii", "stimuli", "alumni", "syllabi",
    "analyses", "bases", "crises", "diagnoses", "theses", "hypotheses", "parentheses", "axes",
    "criteria", "phenomena", "data", "media", "bacteria", "curricula", "indices", "matrices",
    "vertices", "appendices", "quizzes", "heroes", "potatoes", "tomatoes", "echoes", "dice",
];

const UNCOUNTABLES: [36][]const u8 = [
    "information", "equipment", "rice", "money", "species", "series",
    "fish", "sheep", "deer", "aircraft", "software", "hardware",
    "metadata", "feedback", "news", "advice", "luggage", "furniture",
    "traffic", "music", "audio", "police", "moose", "bison",
    "salmon", "trout", "water", "air", "sugar", "salt",
    "knowledge", "research", "evidence", "progress", "weather", "spacecraft",
];

// Singular nouns ending in a single 's' whose plural adds 'es' (e.g. bus -> buses).
const SINGULAR_S_ES: [20][]const u8 = [
    "bus", "gas", "lens", "alias", "status", "campus", "virus", "bonus",
    "chorus", "circus", "atlas", "canvas", "bias", "iris", "plus", "surplus",
    "walrus", "octopus", "genius", "census",
];

// Plural nouns ending in 'ies' whose singular ends in 'ie' (strip 's', not 'ies'->'y').
const PLURAL_IE_S: [10][]const u8 = [
    "movies", "cookies", "zombies", "brownies", "calories",
    "genies", "rookies", "selfies", "smoothies", "ties",
];

const CASE_LOWER: u8 = 0;
const CASE_TITLE: u8 = 1;
const CASE_UPPER: u8 = 2;

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

fn isVowelLower(c: u8): bool {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

fn detectCaseStyle(word: []const u8): u8 {
    if (word.len == 0) return CASE_LOWER;
    let has_alpha = false;
    let all_upper = true;
    let i: usize = 0;
    while (i < word.len) : (i += 1) {
        const c = word[i];
        if (c >= 'a' && c <= 'z') {
            has_alpha = true;
            all_upper = false;
        } else if (c >= 'A' && c <= 'Z') {
            has_alpha = true;
        }
    }
    if (has_alpha && all_upper) return CASE_UPPER;
    if (word[0] >= 'A' && word[0] <= 'Z') return CASE_TITLE;
    return CASE_LOWER;
}

fn copySlice(out: []u8, src: []const u8): []const u8 {
    const n = if (src.len < out.len) src.len else out.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = src[i];
    }
    return out[0..n];
}

fn writeWithCase(out: []u8, src_lower: []const u8, style: u8): []const u8 {
    const n = if (src_lower.len < out.len) src_lower.len else out.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const c = src_lower[i];
        if (style == CASE_UPPER) {
            out[i] = toUpperAscii(c);
        } else if (style == CASE_TITLE && i == 0) {
            out[i] = toUpperAscii(c);
        } else {
            out[i] = c;
        }
    }
    return out[0..n];
}

fn writeStemAndSuffix(out: []u8, stem: []const u8, suffix_lower: []const u8, all_upper: bool): []const u8 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < stem.len && pos < out.len) : (i += 1) {
        out[pos] = stem[i];
        pos += 1;
    }
    let j: usize = 0;
    while (j < suffix_lower.len && pos < out.len) : (j += 1) {
        out[pos] = if (all_upper) toUpperAscii(suffix_lower[j]) else suffix_lower[j];
        pos += 1;
    }
    return out[0..pos];
}

fn toLowerInto(word: []const u8, buf: []u8): []const u8 {
    const n = if (word.len < buf.len) word.len else buf.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        buf[i] = toLowerAscii(word[i]);
    }
    return buf[0..n];
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    return std.mem.eql(u8, s[(s.len - suffix.len)..], suffix);
}

fn inList(lower: []const u8, list: []const []const u8): bool {
    let i: usize = 0;
    while (i < list.len) : (i += 1) {
        if (std.mem.eql(u8, lower, list[i])) return true;
    }
    return false;
}

fn formatI64Into(n: i64, out: []u8, pos: *usize) {
    if (n == 0) {
        if (pos.* < out.len) {
            out[pos.*] = '0';
            pos.* += 1;
        }
        return;
    }
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    const neg = n < 0;
    let mag: u64 = if (neg) (@as(u64, 0) -% @as(u64, @bitCast(n))) else @as(u64, @intCast(n));
    while (mag > 0) {
        tmp[len] = ('0' as u8) + ((mag % 10) as u8);
        len += 1;
        mag /= 10;
    }
    if (neg && pos.* < out.len) {
        out[pos.*] = '-';
        pos.* += 1;
    }
    let i: usize = 0;
    while (i < len && pos.* < out.len) : (i += 1) {
        out[pos.*] = tmp[len - 1 - i];
        pos.* += 1;
    }
}

/// Returns `true` if `word` is an English uncountable noun (case-insensitive).
pub fn isUncountable(word: []const u8): bool {
    if (word.len == 0 || word.len > MAX_WORD_LEN) return false;
    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const lower = toLowerInto(word, &lower_buf);
    return inList(lower, &UNCOUNTABLES);
}

/// Converts an English noun to its plural form into `out`, preserving original case style.
pub fn plural(word: []const u8, out: []u8): []const u8 {
    if (word.len == 0) return out[0..0];
    if (word.len > MAX_WORD_LEN) return copySlice(out, word);

    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const lower = toLowerInto(word, &lower_buf);
    const style = detectCaseStyle(word);
    const all_upper = style == CASE_UPPER;

    // 1. Uncountable nouns stay unchanged
    if (inList(lower, &UNCOUNTABLES)) {
        return copySlice(out, word);
    }

    // 2. Irregular dictionary lookup (singular -> plural, or already irregular plural)
    let i: usize = 0;
    while (i < IRREGULAR_SINGULAR.len) : (i += 1) {
        if (std.mem.eql(u8, lower, IRREGULAR_SINGULAR[i])) {
            return writeWithCase(out, IRREGULAR_PLURAL[i], style);
        }
        if (std.mem.eql(u8, lower, IRREGULAR_PLURAL[i])) {
            return copySlice(out, word);
        }
    }

    // 3. Known singular '-s' words that take '+es' (bus -> buses, status -> statuses)
    if (inList(lower, &SINGULAR_S_ES)) {
        return writeStemAndSuffix(out, word, "es", all_upper);
    }

    // 4. Sibilant endings: -ss, -ch, -sh, -x, -z -> +es
    if (endsWith(lower, "ss")
        || endsWith(lower, "ch")
        || endsWith(lower, "sh")
        || endsWith(lower, "x")
        || endsWith(lower, "z"))
    {
        return writeStemAndSuffix(out, word, "es", all_upper);
    }

    // 5. Already ends with 's' (not -ss and not in SINGULAR_S_ES): assume already plural
    if (lower[lower.len - 1] == 's') {
        return copySlice(out, word);
    }

    // 6. Consonant + 'y' -> replace 'y' with 'ies'; vowel + 'y' -> +s
    if (lower.len >= 2 && lower[lower.len - 1] == 'y') {
        if (!isVowelLower(lower[lower.len - 2])) {
            return writeStemAndSuffix(out, word[0..(word.len - 1)], "ies", all_upper);
        }
        return writeStemAndSuffix(out, word, "s", all_upper);
    }

    // 7. Default regular plural: +s
    return writeStemAndSuffix(out, word, "s", all_upper);
}

/// Converts an English noun to its singular form into `out`, preserving original case style.
pub fn singular(word: []const u8, out: []u8): []const u8 {
    if (word.len == 0) return out[0..0];
    if (word.len > MAX_WORD_LEN) return copySlice(out, word);

    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const lower = toLowerInto(word, &lower_buf);
    const style = detectCaseStyle(word);
    const all_upper = style == CASE_UPPER;

    // 1. Uncountable nouns stay unchanged
    if (inList(lower, &UNCOUNTABLES)) {
        return copySlice(out, word);
    }

    // 2. Irregular dictionary lookup (plural -> singular, or already irregular singular)
    let i: usize = 0;
    while (i < IRREGULAR_PLURAL.len) : (i += 1) {
        if (std.mem.eql(u8, lower, IRREGULAR_PLURAL[i])) {
            return writeWithCase(out, IRREGULAR_SINGULAR[i], style);
        }
        if (std.mem.eql(u8, lower, IRREGULAR_SINGULAR[i])) {
            return copySlice(out, word);
        }
    }

    // 3. Known singular '-s' words or '-ss' / '-us' / '-is' stay singular
    if (inList(lower, &SINGULAR_S_ES)
        || endsWith(lower, "ss")
        || endsWith(lower, "us")
        || endsWith(lower, "is"))
    {
        return copySlice(out, word);
    }

    // 4. '-ies' endings
    if (lower.len > 3 && endsWith(lower, "ies")) {
        if (inList(lower, &PLURAL_IE_S)) {
            return copySlice(out, word[0..(word.len - 1)]);
        }
        return writeStemAndSuffix(out, word[0..(word.len - 3)], "y", all_upper);
    }

    // 5. Sibilant '+es' endings: -sses, -ches, -shes, -xes, -zes -> strip 'es'
    if (endsWith(lower, "sses")
        || endsWith(lower, "ches")
        || endsWith(lower, "shes")
        || endsWith(lower, "xes")
        || endsWith(lower, "zes"))
    {
        return copySlice(out, word[0..(word.len - 2)]);
    }

    // 6. Known singular '-s' + 'es' (buses -> bus, statuses -> status, aliases -> alias)
    if (lower.len > 2 && endsWith(lower, "es")) {
        const stem = lower[0..(lower.len - 2)];
        if (inList(stem, &SINGULAR_S_ES)) {
            return copySlice(out, word[0..(word.len - 2)]);
        }
    }

    // 7. Regular trailing 's' -> strip 's'
    if (lower.len > 1 && lower[lower.len - 1] == 's') {
        return copySlice(out, word[0..(word.len - 1)]);
    }

    return copySlice(out, word);
}

/// Returns `true` if `word` is in plural form (uncountable nouns return `true`).
pub fn isPlural(word: []const u8): bool {
    if (word.len == 0) return false;
    if (isUncountable(word)) return true;
    let buf: [MAX_WORD_LEN + 8]u8 = undefined;
    const p = plural(word, &buf);
    return eqlIgnoreCase(word, p);
}

/// Returns `true` if `word` is in singular form (uncountable nouns return `true`).
pub fn isSingular(word: []const u8): bool {
    if (word.len == 0) return false;
    if (isUncountable(word)) return true;
    let buf: [MAX_WORD_LEN + 8]u8 = undefined;
    const s = singular(word, &buf);
    return eqlIgnoreCase(word, s);
}

/// Formats `count` together with the properly inflected `word` into `out`
/// (e.g. `pluralizeCount("item", 1, &buf)` -> `"1 item"`, `count = 3` -> `"3 items"`).
pub fn pluralizeCount(word: []const u8, count: i64, out: []u8): []const u8 {
    let pos: usize = 0;
    formatI64Into(count, out, &pos);
    if (pos < out.len) {
        out[pos] = ' ';
        pos += 1;
    }
    const rest = out[pos..];
    const inflected = if (count == 1 || count == -1)
        singular(word, rest)
    else
        plural(word, rest);
    return out[0..(pos + inflected.len)];
}

/// Formats an integer with its English ordinal suffix into `out`
/// (e.g. `1` -> `"1st"`, `2` -> `"2nd"`, `3` -> `"3rd"`, `11` -> `"11th"`, `21` -> `"21st"`).
pub fn ordinal(n: i64, out: []u8): []const u8 {
    let pos: usize = 0;
    formatI64Into(n, out, &pos);
    const abs_n: u64 = if (n < 0)
        (@as(u64, 0) -% @as(u64, @bitCast(n)))
    else
        @as(u64, @intCast(n));
    const mod100 = abs_n % 100;
    const mod10 = abs_n % 10;
    const suffix: []const u8 = if (mod100 >= 11 && mod100 <= 13)
        "th"
    else if (mod10 == 1)
        "st"
    else if (mod10 == 2)
        "nd"
    else if (mod10 == 3)
        "rd"
    else
        "th";

    let i: usize = 0;
    while (i < suffix.len && pos < out.len) : (i += 1) {
        out[pos] = suffix[i];
        pos += 1;
    }
    return out[0..pos];
}
