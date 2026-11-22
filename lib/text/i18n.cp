// Source: cp/lib/i18n/catalog.cp (Locale Negotiator, CLDR Plural Rules & I18n Message Catalog)
// Pure CP internationalization engine providing BCP 47 locale normalization & fallback chains,
// RFC 4647 Accept-Language negotiation, CLDR cardinal plural rules, and safe message catalog interpolation.

const std = @import("std");
import { JsonParser, JsonValue, JsonTag } from "cap:json";

/// Error set returned by `cap:i18n` operations.
pub err I18nError {
    LocaleNotFound,
    KeyNotFound,
    InvalidLocale,
    InvalidJson,
    OutOfMemory,
}

/// CLDR plural categories.
pub enum PluralCategory {
    Zero,
    One,
    Two,
    Few,
    Many,
    Other,
}

/// Key-value parameter for message template interpolation.
pub struct I18nParam {
    key: string = "";
    value: string = "";
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlnum(c: u8): bool {
    return isAsciiAlpha(c) || isAsciiDigit(c);
}

fn i18nToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn i18nToUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

fn trimAscii(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (i18nToLowerAscii(a[i]) != i18nToLowerAscii(b[i])) return false;
    }
    return true;
}

fn isAllAlpha(s: string): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isAsciiAlpha(s[i])) return false;
    }
    return true;
}

fn isAllAlnum(s: string): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isAsciiAlnum(s[i])) return false;
    }
    return true;
}

fn writeI64ToBuf(buf: []u8, val: i64): string {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    const is_neg = val < 0;
    let mag: u64 = 0;
    if (is_neg) {
        const raw: u64 = @bitCast(val);
        mag = (0 as u64) -% raw;
    } else {
        mag = val as u64;
    }

    let tmp: [32]u8 = undefined;
    let tlen: usize = 0;
    let v = mag;
    while (v > 0) {
        tmp[tlen] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        tlen += 1;
    }
    let pos: usize = 0;
    if (is_neg) {
        buf[0] = '-';
        pos = 1;
    }
    let i: usize = 0;
    while (i < tlen) : (i += 1) {
        buf[pos + i] = tmp[tlen - 1 - i];
    }
    return buf[0..(pos + tlen)];
}

struct NegotiateCandidate {
    tag: string;
    q_milli: u16;
    order: usize;
}

fn parseQValue(raw_q: string): u16? {
    const s = trimAscii(raw_q);
    if (s.len == 0) return null;
    if (s.len == 1) {
        if (s[0] == '0') return 0;
        if (s[0] == '1') return 1000;
        return null;
    }
    if (s[1] != '.') return null;
    if (s[0] == '1') {
        let i: usize = 2;
        while (i < s.len) : (i += 1) {
            if (s[i] != '0') return null;
        }
        if (s.len > 5) return null;
        return 1000;
    }
    if (s[0] != '0') return null;
    if (s.len == 2 || s.len > 5) return null;
    let val: u16 = 0;
    let mult: u16 = 100;
    let i: usize = 2;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (!isAsciiDigit(c)) return null;
        val += ((c - '0') as u16) * mult;
        mult /= 10;
    }
    return val;
}

fn startsWithLocalePrefix(full: string, prefix: string): bool {
    if (full.len <= prefix.len) return false;
    if (!eqlIgnoreCase(full[0..prefix.len], prefix)) return false;
    return full[prefix.len] == '-';
}

/// BCP 47 locale tag normalizer, hierarchical fallback generator, and RFC 4647 Accept-Language negotiator.
pub static struct LocaleNegotiator {
    /// Normalizes a locale tag (e.g. `"zh_cn"` -> `"zh-CN"`, `"zh_hans_cn"` -> `"zh-Hans-CN"`).
    static fn normalizeLocale(alloc: std.mem.Allocator, tag: string): ([]u8) ! I18nError {
        const trimmed = trimAscii(tag);
        if (trimmed.len == 0) return I18nError.InvalidLocale;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let subtag_idx: usize = 0;
        let in_extension = false;
        let i: usize = 0;

        while (i <= trimmed.len) {
            const start = i;
            while (i < trimmed.len && trimmed[i] != '-' && trimmed[i] != '_') {
                i += 1;
            }
            const part = trimmed[start..i];
            if (!isAllAlnum(part)) {
                return I18nError.InvalidLocale;
            }

            if (subtag_idx > 0) {
                out.append(alloc, '-') catch return I18nError.OutOfMemory;
            }

            if (subtag_idx == 0) {
                if (!isAllAlpha(part) || part.len < 2 || part.len > 8) {
                    return I18nError.InvalidLocale;
                }
                for (ch of part) {
                    out.append(alloc, i18nToLowerAscii(ch)) catch return I18nError.OutOfMemory;
                }
            } else if (!in_extension && part.len == 1) {
                in_extension = true;
                out.append(alloc, i18nToLowerAscii(part[0])) catch return I18nError.OutOfMemory;
            } else if (!in_extension && part.len == 4 && isAllAlpha(part)) {
                // Script subtag: TitleCase (e.g. Hans, Latn)
                out.append(alloc, i18nToUpperAscii(part[0])) catch return I18nError.OutOfMemory;
                let k: usize = 1;
                while (k < part.len) : (k += 1) {
                    out.append(alloc, i18nToLowerAscii(part[k])) catch return I18nError.OutOfMemory;
                }
            } else if (!in_extension && part.len == 2 && isAllAlpha(part)) {
                // Region subtag: UPPERCASE (e.g. CN, US, GB)
                for (ch of part) {
                    out.append(alloc, i18nToUpperAscii(ch)) catch return I18nError.OutOfMemory;
                }
            } else {
                for (ch of part) {
                    out.append(alloc, i18nToLowerAscii(ch)) catch return I18nError.OutOfMemory;
                }
            }

            subtag_idx += 1;
            if (i == trimmed.len) break;
            i += 1;
            if (i == trimmed.len) {
                // Trailing '-' or '_'
                return I18nError.InvalidLocale;
            }
        }

        return out.toOwnedSlice(alloc) catch return I18nError.OutOfMemory;
    }

    /// Generates hierarchical fallback chain (e.g. `"zh-Hans-CN"` -> `["zh-Hans-CN", "zh-Hans", "zh"]`).
    /// Caller owns returned slice and each element (use `freeFallbackChain`).
    static fn fallbackChain(alloc: std.mem.Allocator, locale: string): ([][]u8) ! I18nError {
        const norm = LocaleNegotiator.normalizeLocale(alloc, locale)?;
        let list: std.ArrayList([]u8) = .empty;
        errdefer {
            for (item of list.items) alloc.free(item);
            list.deinit(alloc);
        }

        list.append(alloc, norm) catch {
            alloc.free(norm);
            return I18nError.OutOfMemory;
        };

        let cur_len: usize = norm.len;
        while (cur_len > 0) {
            let dash_idx: usize? = null;
            let j = cur_len;
            while (j > 0) {
                j -= 1;
                if (norm[j] == '-') {
                    dash_idx = j;
                    break;
                }
            }
            if (idx := dash_idx) {
                if (idx == 0) break;
                const parent = alloc.dupe(u8, norm[0..idx]) catch return I18nError.OutOfMemory;
                list.append(alloc, parent) catch {
                    alloc.free(parent);
                    return I18nError.OutOfMemory;
                };
                cur_len = idx;
            } else {
                break;
            }
        }

        return list.toOwnedSlice(alloc) catch return I18nError.OutOfMemory;
    }

    /// Frees a fallback chain returned by `fallbackChain`.
    static fn freeFallbackChain(alloc: std.mem.Allocator, chain: [][]u8) {
        for (item of chain) {
            alloc.free(item);
        }
        alloc.free(chain);
    }

    /// Negotiates the best supported locale from an HTTP `Accept-Language` header per RFC 4647.
    static fn negotiate(
        alloc: std.mem.Allocator,
        accept_language_header: string,
        supported_locales: []const string,
        default_locale: string,
    ): ([]u8) ! I18nError {
        let arena_state = std.heap.ArenaAllocator.init(alloc);
        defer arena_state.deinit();
        const tmp = arena_state.allocator();

        const norm_default = LocaleNegotiator.normalizeLocale(tmp, default_locale)?;

        let norm_supported: std.ArrayList([]u8) = .empty;
        for (sup of supported_locales) {
            const ns = LocaleNegotiator.normalizeLocale(tmp, sup)?;
            norm_supported.append(tmp, ns) catch return I18nError.OutOfMemory;
        }

        let candidates: std.ArrayList(NegotiateCandidate) = .empty;
        let order: usize = 0;
        let pos: usize = 0;
        while (pos < accept_language_header.len) {
            let comma = pos;
            while (comma < accept_language_header.len && accept_language_header[comma] != ',') {
                comma += 1;
            }
            const entry = trimAscii(accept_language_header[pos..comma]);
            pos = if (comma < accept_language_header.len) comma + 1 else comma;

            if (entry.len == 0) continue;

            let semi: usize? = null;
            let k: usize = 0;
            while (k < entry.len) : (k += 1) {
                if (entry[k] == ';') {
                    semi = k;
                    break;
                }
            }

            let raw_tag = entry;
            let q_milli: u16 = 1000;
            let valid_q = true;
            if (sidx := semi) {
                raw_tag = trimAscii(entry[0..sidx]);
                const params_str = trimAscii(entry[(sidx + 1)..]);
                // Parse q=...
                if (params_str.len >= 2 && (params_str[0] == 'q' || params_str[0] == 'Q')) {
                    const after_q = trimAscii(params_str[1..]);
                    if (after_q.len >= 2 && after_q[0] == '=') {
                        if (qv := parseQValue(after_q[1..])) {
                            q_milli = qv;
                        } else {
                            valid_q = false;
                        }
                    } else {
                        valid_q = false;
                    }
                }
            }

            if (!valid_q || q_milli == 0 || raw_tag.len == 0) continue;

            candidates.append(tmp, NegotiateCandidate{
                tag: raw_tag,
                q_milli: q_milli,
                order: order,
            }) catch return I18nError.OutOfMemory;
            order += 1;
        }

        // Stable insertion sort by q_milli descending, then original order ascending
        let idx: usize = 1;
        while (idx < candidates.items.len) : (idx += 1) {
            const key_item = candidates.items[idx];
            let j = idx;
            while (j > 0) {
                const prev = candidates.items[j - 1];
                if (prev.q_milli < key_item.q_milli || (prev.q_milli == key_item.q_milli && prev.order > key_item.order)) {
                    candidates.items[j] = prev;
                    j -= 1;
                } else {
                    break;
                }
            }
            candidates.items[j] = key_item;
        }

        for (cand of candidates.items) {
            if (std.mem.eql(u8, cand.tag, "*")) {
                return alloc.dupe(u8, norm_default) catch return I18nError.OutOfMemory;
            }

            const norm_req = LocaleNegotiator.normalizeLocale(tmp, cand.tag) catch continue;

            // 1. Exact match
            for (sup of norm_supported.items) {
                if (eqlIgnoreCase(norm_req, sup)) {
                    return alloc.dupe(u8, sup) catch return I18nError.OutOfMemory;
                }
            }

            // 2. Prefix fallback on requested tag (e.g. fr-CH -> fr)
            const chain = LocaleNegotiator.fallbackChain(tmp, norm_req) catch continue;
            let ci: usize = 1;
            while (ci < chain.len) : (ci += 1) {
                for (sup of norm_supported.items) {
                    if (eqlIgnoreCase(chain[ci], sup)) {
                        return alloc.dupe(u8, sup) catch return I18nError.OutOfMemory;
                    }
                }
            }

            // 3. Supported locale starts with requested tag (e.g. en -> en-US)
            for (sup of norm_supported.items) {
                if (startsWithLocalePrefix(sup, norm_req)) {
                    return alloc.dupe(u8, sup) catch return I18nError.OutOfMemory;
                }
            }
        }

        return alloc.dupe(u8, norm_default) catch return I18nError.OutOfMemory;
    }
}

/// CLDR cardinal plural category selector.
pub static struct PluralRules {
    /// Selects the CLDR cardinal `PluralCategory` for `locale` and integer `count`.
    static fn select(locale: string, count: i64): PluralCategory {
        const trimmed = trimAscii(locale);
        let sep_idx: usize = trimmed.len;
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            if (trimmed[i] == '-' || trimmed[i] == '_') {
                sep_idx = i;
                break;
            }
        }
        const lang = trimmed[0..sep_idx];

        let n: u64 = 0;
        if (count < 0) {
            const raw: u64 = @bitCast(count);
            n = (0 as u64) -% raw;
        } else {
            n = count as u64;
        }

        // East Asian & Southeast Asian languages with no grammatical plural distinction
        if (eqlIgnoreCase(lang, "zh") ||
            eqlIgnoreCase(lang, "ja") ||
            eqlIgnoreCase(lang, "ko") ||
            eqlIgnoreCase(lang, "vi") ||
            eqlIgnoreCase(lang, "th") ||
            eqlIgnoreCase(lang, "id") ||
            eqlIgnoreCase(lang, "ms"))
        {
            return PluralCategory.Other;
        }

        // French & Portuguese (0 and 1 are One)
        if (eqlIgnoreCase(lang, "fr") || eqlIgnoreCase(lang, "pt")) {
            if (n == 0 || n == 1) return PluralCategory.One;
            return PluralCategory.Other;
        }

        // East/South Slavic (Russian, Ukrainian, Belarusian, Serbian, Croatian, Bosnian)
        if (eqlIgnoreCase(lang, "ru") ||
            eqlIgnoreCase(lang, "uk") ||
            eqlIgnoreCase(lang, "be") ||
            eqlIgnoreCase(lang, "sr") ||
            eqlIgnoreCase(lang, "hr") ||
            eqlIgnoreCase(lang, "bs"))
        {
            const mod10 = n % 10;
            const mod100 = n % 100;
            if (mod10 == 1 && mod100 != 11) return PluralCategory.One;
            if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return PluralCategory.Few;
            if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) || (mod100 >= 11 && mod100 <= 14)) return PluralCategory.Many;
            return PluralCategory.Other;
        }

        // Polish
        if (eqlIgnoreCase(lang, "pl")) {
            if (n == 1) return PluralCategory.One;
            const mod10 = n % 10;
            const mod100 = n % 100;
            if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return PluralCategory.Few;
            return PluralCategory.Many;
        }

        // Czech & Slovak
        if (eqlIgnoreCase(lang, "cs") || eqlIgnoreCase(lang, "sk")) {
            if (n == 1) return PluralCategory.One;
            if (n >= 2 && n <= 4) return PluralCategory.Few;
            return PluralCategory.Other;
        }

        // Arabic
        if (eqlIgnoreCase(lang, "ar")) {
            if (n == 0) return PluralCategory.Zero;
            if (n == 1) return PluralCategory.One;
            if (n == 2) return PluralCategory.Two;
            const mod100 = n % 100;
            if (mod100 >= 3 && mod100 <= 10) return PluralCategory.Few;
            if (mod100 >= 11 && mod100 <= 99) return PluralCategory.Many;
            return PluralCategory.Other;
        }

        // Default Germanic/Romance/etc. (en, de, es, it, nl, ...)
        if (n == 1) return PluralCategory.One;
        return PluralCategory.Other;
    }
}

struct PluralForms {
    forms: [6](([]u8)?) = [null, null, null, null, null, null];
}

struct LocaleBundle {
    messages: std.StringHashMapUnmanaged([]u8) = .empty;
    plurals: std.StringHashMapUnmanaged(PluralForms) = .empty;
}

fn pluralIndex(cat: PluralCategory): usize {
    return switch (cat) {
        .Zero => 0,
        .One => 1,
        .Two => 2,
        .Few => 3,
        .Many => 4,
        .Other => 5,
    };
}

fn findParam(params: []const I18nParam, name: string): string? {
    for (p of params) {
        if (std.mem.eql(u8, p.key, name)) return p.value;
    }
    return null;
}

fn interpolateTemplate(
    alloc: std.mem.Allocator,
    tmpl: string,
    params: []const I18nParam,
    implicit_count: string?,
): ([]u8) ! I18nError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < tmpl.len) {
        const ch = tmpl[i];
        if (ch == '{') {
            if (i + 1 < tmpl.len && tmpl[i + 1] == '{') {
                out.append(alloc, '{') catch return I18nError.OutOfMemory;
                i += 2;
                continue;
            }
            let close_idx: usize? = null;
            let j = i + 1;
            while (j < tmpl.len) : (j += 1) {
                if (tmpl[j] == '}') {
                    close_idx = j;
                    break;
                }
                if (tmpl[j] == '{') {
                    break;
                }
            }
            if (cidx := close_idx) {
                const pname = tmpl[(i + 1)..cidx];
                if (pval := findParam(params, pname)) {
                    // Single-pass append: never re-scans pval for placeholders
                    out.appendSlice(alloc, pval) catch return I18nError.OutOfMemory;
                } else if (std.mem.eql(u8, pname, "count")) {
                    if (cstr_val := implicit_count) {
                        out.appendSlice(alloc, cstr_val) catch return I18nError.OutOfMemory;
                    } else {
                        out.appendSlice(alloc, tmpl[i..(cidx + 1)]) catch return I18nError.OutOfMemory;
                    }
                } else {
                    // Unknown placeholder: preserve literally
                    out.appendSlice(alloc, tmpl[i..(cidx + 1)]) catch return I18nError.OutOfMemory;
                }
                i = cidx + 1;
                continue;
            }
            out.append(alloc, '{') catch return I18nError.OutOfMemory;
            i += 1;
        } else if (ch == '}') {
            if (i + 1 < tmpl.len && tmpl[i + 1] == '}') {
                out.append(alloc, '}') catch return I18nError.OutOfMemory;
                i += 2;
                continue;
            }
            out.append(alloc, '}') catch return I18nError.OutOfMemory;
            i += 1;
        } else {
            out.append(alloc, ch) catch return I18nError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(alloc) catch return I18nError.OutOfMemory;
}

/// Multi-locale message catalog with hierarchical fallback, JSON dictionary loading,
/// single-pass safe placeholder interpolation, and CLDR pluralization.
pub struct I18nCatalog {
    alloc: std.mem.Allocator;
    default_locale: []u8;
    locales: std.StringHashMapUnmanaged(LocaleBundle) = .empty;

    /// Initializes a new `I18nCatalog` with the given `default_locale`.
    pub static fn init(alloc: std.mem.Allocator, default_locale: string): I18nCatalog ! I18nError {
        const norm_def = LocaleNegotiator.normalizeLocale(alloc, default_locale)?;
        return I18nCatalog{
            alloc: alloc,
            default_locale: norm_def,
            locales: .empty,
        };
    }

    /// Releases all memory owned by the catalog.
    pub fn deinit(*_) {
        let loc_it = _.locales.iterator();
        while (entry := loc_it.next()) {
            let msg_it = entry.value_ptr.messages.iterator();
            while (me := msg_it.next()) {
                _.alloc.free(me.key_ptr.*);
                _.alloc.free(me.value_ptr.*);
            }
            entry.value_ptr.messages.deinit(_.alloc);

            let pl_it = entry.value_ptr.plurals.iterator();
            while (pe := pl_it.next()) {
                _.alloc.free(pe.key_ptr.*);
                let fi: usize = 0;
                while (fi < 6) : (fi += 1) {
                    if (f := pe.value_ptr.forms[fi]) {
                        _.alloc.free(f);
                    }
                }
            }
            entry.value_ptr.plurals.deinit(_.alloc);

            _.alloc.free(entry.key_ptr.*);
        }
        _.locales.deinit(_.alloc);
        _.alloc.free(_.default_locale);
    }

    fn getOrCreateBundle(*_, norm_locale: string): (*LocaleBundle) ! I18nError {
        if (existing := _.locales.getPtr(norm_locale)) {
            return existing;
        }
        const key_copy = _.alloc.dupe(u8, norm_locale) catch return I18nError.OutOfMemory;
        errdefer _.alloc.free(key_copy);
        const gop = _.locales.getOrPut(_.alloc, key_copy) catch return I18nError.OutOfMemory;
        if (!gop.found_existing) {
            gop.value_ptr.* = LocaleBundle{};
        }
        return gop.value_ptr;
    }

    fn addNormalized(*_, norm_locale: string, key: string, template: string): ! {
        const bundle = _.getOrCreateBundle(norm_locale)?;
        const val_copy = _.alloc.dupe(u8, template) catch return I18nError.OutOfMemory;
        errdefer _.alloc.free(val_copy);

        if (existing := bundle.messages.getPtr(key)) {
            _.alloc.free(existing.*);
            existing.* = val_copy;
            return;
        }
        const key_copy = _.alloc.dupe(u8, key) catch return I18nError.OutOfMemory;
        errdefer _.alloc.free(key_copy);
        bundle.messages.put(_.alloc, key_copy, val_copy) catch return I18nError.OutOfMemory;
    }

    /// Registers or replaces a message template for `(locale, key)`.
    pub fn add(*_, locale: string, key: string, template: string): ! {
        const norm = LocaleNegotiator.normalizeLocale(_.alloc, locale)?;
        defer _.alloc.free(norm);
        return _.addNormalized(norm, key, template);
    }

    /// Registers or replaces a plural template for `(locale, key, category)`.
    pub fn addPlural(*_, locale: string, key: string, category: PluralCategory, template: string): ! {
        const norm = LocaleNegotiator.normalizeLocale(_.alloc, locale)?;
        defer _.alloc.free(norm);

        const bundle = _.getOrCreateBundle(norm)?;
        const val_copy = _.alloc.dupe(u8, template) catch return I18nError.OutOfMemory;
        errdefer _.alloc.free(val_copy);
        const idx = pluralIndex(category);

        if (existing := bundle.plurals.getPtr(key)) {
            if (old := existing.forms[idx]) {
                _.alloc.free(old);
            }
            existing.forms[idx] = val_copy;
            return;
        }

        const key_copy = _.alloc.dupe(u8, key) catch return I18nError.OutOfMemory;
        errdefer _.alloc.free(key_copy);
        let forms = PluralForms{};
        forms.forms[idx] = val_copy;
        bundle.plurals.put(_.alloc, key_copy, forms) catch return I18nError.OutOfMemory;
    }

    fn loadJsonObject(*_, norm_locale: string, prefix: string, obj: JsonValue, tmp_alloc: std.mem.Allocator): ! {
        const fields = obj.asObject() orelse return I18nError.InvalidJson;
        for (field of fields) {
            let full_key: string = field.key;
            if (prefix.len > 0) {
                let buf: std.ArrayList(u8) = .empty;
                buf.appendSlice(tmp_alloc, prefix) catch return I18nError.OutOfMemory;
                buf.append(tmp_alloc, '.') catch return I18nError.OutOfMemory;
                buf.appendSlice(tmp_alloc, field.key) catch return I18nError.OutOfMemory;
                full_key = buf.items;
            }
            switch (field.value.tag) {
                .String => {
                    _.addNormalized(norm_locale, full_key, field.value.str_val)?;
                },
                .Object => {
                    _.loadJsonObject(norm_locale, full_key, field.value, tmp_alloc)?;
                },
                else => return I18nError.InvalidJson,
            }
        }
    }

    /// Parses a JSON object (flat or nested) and loads all `"dot.separated.key": "template"` entries for `locale`.
    pub fn loadJson(*_, locale: string, json_str: string): ! {
        const norm = LocaleNegotiator.normalizeLocale(_.alloc, locale)?;
        defer _.alloc.free(norm);

        let arena_state = std.heap.ArenaAllocator.init(_.alloc);
        defer arena_state.deinit();
        const tmp = arena_state.allocator();

        const root = JsonParser.parse(tmp, json_str) catch return I18nError.InvalidJson;
        if (!root.isObject()) return I18nError.InvalidJson;
        return _.loadJsonObject(norm, "", root, tmp);
    }

    fn lookupTemplate(locale: string, key: string): string? {
        const chain = LocaleNegotiator.fallbackChain(_.alloc, locale) catch [];
        if (chain.len > 0) {
            defer LocaleNegotiator.freeFallbackChain(_.alloc, chain);
            for (cand of chain) {
                if (bundle := _.locales.getPtr(cand)) {
                    if (tmpl := bundle.messages.get(key)) {
                        return tmpl;
                    }
                }
            }
        }

        if (def_bundle := _.locales.getPtr(_.default_locale)) {
            if (tmpl := def_bundle.messages.get(key)) {
                return tmpl;
            }
        }
        return null;
    }

    fn lookupPluralTemplate(locale: string, key: string, cat: PluralCategory): string? {
        const cat_idx = pluralIndex(cat);
        const other_idx = pluralIndex(PluralCategory.Other);

        const chain = LocaleNegotiator.fallbackChain(_.alloc, locale) catch [];
        if (chain.len > 0) {
            defer LocaleNegotiator.freeFallbackChain(_.alloc, chain);
            for (cand of chain) {
                if (bundle := _.locales.getPtr(cand)) {
                    if (pforms := bundle.plurals.get(key)) {
                        if (tmpl := pforms.forms[cat_idx]) return tmpl;
                        if (tmpl := pforms.forms[other_idx]) return tmpl;
                    }
                }
            }
        }

        if (def_bundle := _.locales.getPtr(_.default_locale)) {
            if (pforms := def_bundle.plurals.get(key)) {
                if (tmpl := pforms.forms[cat_idx]) return tmpl;
                if (tmpl := pforms.forms[other_idx]) return tmpl;
            }
        }
        return null;
    }

    /// Translates `key` for `locale` with hierarchical & default-locale fallback.
    pub fn t(alloc: std.mem.Allocator, locale: string, key: string): ([]u8) ! I18nError {
        return _.format(alloc, locale, key, []);
    }

    /// Translates `key` for `locale` and performs single-pass `{param}` & `{{escaped}}` interpolation.
    pub fn format(
        alloc: std.mem.Allocator,
        locale: string,
        key: string,
        params: []const I18nParam,
    ): ([]u8) ! I18nError {
        if (tmpl := _.lookupTemplate(locale, key)) {
            return interpolateTemplate(alloc, tmpl, params, null);
        }
        return I18nError.KeyNotFound;
    }

    /// Translates `key` for `locale` with CLDR plural selection for `count`, injecting `{count}` and `params`.
    pub fn tp(
        alloc: std.mem.Allocator,
        locale: string,
        key: string,
        count: i64,
        params: []const I18nParam,
    ): ([]u8) ! I18nError {
        const cat = PluralRules.select(locale, count);
        if (tmpl := _.lookupPluralTemplate(locale, key, cat)) {
            let cbuf: [32]u8 = undefined;
            const count_str = writeI64ToBuf(&cbuf, count);
            return interpolateTemplate(alloc, tmpl, params, count_str);
        }
        return I18nError.KeyNotFound;
    }
}
