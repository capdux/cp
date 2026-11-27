// @cap/pluralize — Zero-allocation English Pluralization, Singularization & Ordinal Engine
// Clean-room pure-CP implementation modeled after npm `pluralize`, `inflection`, and Rust `pluralizer` (MIT License).

import { toUpperByte, toLower, eqIgnoreCase, endsWith, strEq } from "cap:ascii";
import { writeStr, writeI64 } from "cap:fmt";

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
    const n = writeStr(src, out, 0);
    return out[0..n];
}

fn writeWithCase(out: []u8, src_lower: []const u8, style: u8): []const u8 {
    const n = if (src_lower.len < out.len) src_lower.len else out.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const c = src_lower[i];
        if (style == CASE_UPPER || (style == CASE_TITLE && i == 0)) {
            out[i] = toUpperByte(c);
        } else {
            out[i] = c;
        }
    }
    return out[0..n];
}

fn writeStemAndSuffix(out: []u8, stem: []const u8, suffix_lower: []const u8, all_upper: bool): []const u8 {
    let pos = writeStr(stem, out, 0);
    let j: usize = 0;
    while (j < suffix_lower.len && pos < out.len) : (j += 1) {
        out[pos] = if (all_upper) toUpperByte(suffix_lower[j]) else suffix_lower[j];
        pos += 1;
    }
    return out[0..pos];
}

fn inList(lower: []const u8, list: []const []const u8): bool {
    let i: usize = 0;
    while (i < list.len) : (i += 1) {
        if (strEq(lower, list[i])) return true;
    }
    return false;
}

/// Returns `true` if `word` is an English uncountable noun (case-insensitive).
pub fn isUncountable(word: []const u8): bool {
    if (word.len == 0 || word.len > MAX_WORD_LEN) return false;
    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const n = toLower(word, &lower_buf);
    return inList(lower_buf[0..n], &UNCOUNTABLES);
}

/// Converts an English noun to its plural form into `out`, preserving original case style.
pub fn plural(word: []const u8, out: []u8): []const u8 {
    if (word.len == 0) return out[0..0];
    if (word.len > MAX_WORD_LEN) return copySlice(out, word);

    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const lower = lower_buf[0..toLower(word, &lower_buf)];
    const style = detectCaseStyle(word);
    const all_upper = style == CASE_UPPER;

    if (inList(lower, &UNCOUNTABLES)) {
        return copySlice(out, word);
    }

    let i: usize = 0;
    while (i < IRREGULAR_SINGULAR.len) : (i += 1) {
        if (strEq(lower, IRREGULAR_SINGULAR[i])) {
            return writeWithCase(out, IRREGULAR_PLURAL[i], style);
        }
        if (strEq(lower, IRREGULAR_PLURAL[i])) {
            return copySlice(out, word);
        }
    }

    if (inList(lower, &SINGULAR_S_ES)) {
        return writeStemAndSuffix(out, word, "es", all_upper);
    }

    if (endsWith(lower, "ss")
        || endsWith(lower, "ch")
        || endsWith(lower, "sh")
        || endsWith(lower, "x")
        || endsWith(lower, "z"))
    {
        return writeStemAndSuffix(out, word, "es", all_upper);
    }

    if (lower[lower.len - 1] == 's') {
        return copySlice(out, word);
    }

    if (lower.len >= 2 && lower[lower.len - 1] == 'y') {
        if (!isVowelLower(lower[lower.len - 2])) {
            return writeStemAndSuffix(out, word[0..(word.len - 1)], "ies", all_upper);
        }
        return writeStemAndSuffix(out, word, "s", all_upper);
    }

    return writeStemAndSuffix(out, word, "s", all_upper);
}

/// Converts an English noun to its singular form into `out`, preserving original case style.
pub fn singular(word: []const u8, out: []u8): []const u8 {
    if (word.len == 0) return out[0..0];
    if (word.len > MAX_WORD_LEN) return copySlice(out, word);

    let lower_buf: [MAX_WORD_LEN]u8 = undefined;
    const lower = lower_buf[0..toLower(word, &lower_buf)];
    const style = detectCaseStyle(word);
    const all_upper = style == CASE_UPPER;

    if (inList(lower, &UNCOUNTABLES)) {
        return copySlice(out, word);
    }

    let i: usize = 0;
    while (i < IRREGULAR_PLURAL.len) : (i += 1) {
        if (strEq(lower, IRREGULAR_PLURAL[i])) {
            return writeWithCase(out, IRREGULAR_SINGULAR[i], style);
        }
        if (strEq(lower, IRREGULAR_SINGULAR[i])) {
            return copySlice(out, word);
        }
    }

    if (inList(lower, &SINGULAR_S_ES)
        || endsWith(lower, "ss")
        || endsWith(lower, "us")
        || endsWith(lower, "is"))
    {
        return copySlice(out, word);
    }

    if (lower.len > 3 && endsWith(lower, "ies")) {
        if (inList(lower, &PLURAL_IE_S)) {
            return copySlice(out, word[0..(word.len - 1)]);
        }
        return writeStemAndSuffix(out, word[0..(word.len - 3)], "y", all_upper);
    }

    if (endsWith(lower, "sses")
        || endsWith(lower, "ches")
        || endsWith(lower, "shes")
        || endsWith(lower, "xes")
        || endsWith(lower, "zes"))
    {
        return copySlice(out, word[0..(word.len - 2)]);
    }

    if (lower.len > 2 && endsWith(lower, "es")) {
        const stem = lower[0..(lower.len - 2)];
        if (inList(stem, &SINGULAR_S_ES)) {
            return copySlice(out, word[0..(word.len - 2)]);
        }
    }

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
    return eqIgnoreCase(word, p);
}

/// Returns `true` if `word` is in singular form (uncountable nouns return `true`).
pub fn isSingular(word: []const u8): bool {
    if (word.len == 0) return false;
    if (isUncountable(word)) return true;
    let buf: [MAX_WORD_LEN + 8]u8 = undefined;
    const s = singular(word, &buf);
    return eqIgnoreCase(word, s);
}

/// Formats `count` together with the properly inflected `word` into `out`
/// (e.g. `pluralizeCount("item", 1, &buf)` -> `"1 item"`, `count = 3` -> `"3 items"`).
pub fn pluralizeCount(word: []const u8, count: i64, out: []u8): []const u8 {
    let pos = writeI64(count, out, 0);
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
    let pos = writeI64(n, out, 0);
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

    pos = writeStr(suffix, out, pos);
    return out[0..pos];
}
