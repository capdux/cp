// packages/@cap/icu-locid/src/index.cp
// Zero-allocation BCP-47 / UTS #35 Unicode Language Identifier parser and canonicalizer in pure CP.
// Reuses @cap/tinystr (TinyAsciiStr4, TinyAsciiStr8) for stack-inline ASCII subtags & case normalization.
// Clean-room MIT implementation (ICU4X icu_locid / icu_locale_core parity).

import { TinyAsciiStr4, TinyAsciiStr8 } from "../../tinystr/src/index.cp";

pub const LOCALE_OK: i64 = 0;
pub const LOCALE_ERR_EMPTY: i64 = -1;
pub const LOCALE_ERR_EMPTY_SUBTAG: i64 = -2;
pub const LOCALE_ERR_INVALID_LANGUAGE: i64 = -3;
pub const LOCALE_ERR_INVALID_SUBTAG: i64 = -4;
pub const LOCALE_ERR_TOO_MANY_VARIANTS: i64 = -5;
pub const LOCALE_ERR_DUPLICATE_VARIANT: i64 = -6;

pub const LOCALE_MAX_VARIANTS: usize = 4;

fn locIsSep(ch: u8): bool {
    return ch == ('-' as u8) || ch == ('_' as u8);
}

fn locIsDigitByte(ch: u8): bool {
    return ch >= ('0' as u8) && ch <= ('9' as u8);
}

fn locEmptyVariants(): [4]TinyAsciiStr8 {
    let arr: [4]TinyAsciiStr8 = undefined;
    let i: usize = 0;
    while (i < LOCALE_MAX_VARIANTS) {
        arr[i] = TinyAsciiStr8.empty();
        i = i + 1;
    }
    return arr;
}

fn locLangEqStr(lang: TinyAsciiStr8, lit: string): bool {
    const r = TinyAsciiStr8.fromStr(lit);
    if (!r.ok) {
        return false;
    }
    return lang.eq(r.value);
}

fn locRegionEqStr(reg: TinyAsciiStr4, lit: string): bool {
    const r = TinyAsciiStr4.fromStr(lit);
    if (!r.ok) {
        return false;
    }
    return reg.eq(r.value);
}

fn locValidLanguageSubtag(sub: TinyAsciiStr8): bool {
    const n = sub.len();
    if (!sub.isAsciiAlphabetic()) {
        return false;
    }
    // UTS #35: unicode_language_subtag = alpha{2,3} | alpha{5,8}
    return (n >= 2 && n <= 3) || (n >= 5 && n <= 8);
}

fn locValidVariantSubtag(sub: TinyAsciiStr8): bool {
    const n = sub.len();
    if (!sub.isAsciiAlphanumeric()) {
        return false;
    }
    // UTS #35: unicode_variant_subtag = alphanum{5,8} | digit alphanum{3}
    if (n >= 5 && n <= 8) {
        return true;
    }
    if (n == 4 && locIsDigitByte(sub.byteAt(0))) {
        return true;
    }
    return false;
}

pub struct LanguageIdentifier {
    pub language: TinyAsciiStr8,
    pub has_script: bool,
    pub script: TinyAsciiStr4,
    pub has_region: bool,
    pub region: TinyAsciiStr4,
    pub variant_count: usize,
    pub variants: [4]TinyAsciiStr8,

    pub static fn und(): LanguageIdentifier {
        return LanguageIdentifier {
            language: TinyAsciiStr8.fromStr("und").value,
            has_script: false,
            script: TinyAsciiStr4.empty(),
            has_region: false,
            region: TinyAsciiStr4.empty(),
            variant_count: 0,
            variants: locEmptyVariants(),
        };
    }

    /// Parses and canonicalizes a BCP-47 / UTS #35 Unicode Language Identifier.
    /// Supports both `-` and `_` separators, normalizes case (lower language, Title script,
    /// UPPER region, sorted lower variants), and strictly validates every subtag.
    pub static fn parse(input: string): LocaleParseResult {
        if (input.len == 0) {
            return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_EMPTY };
        }

        let loc = LanguageIdentifier.und();
        // stage: 0 = language, 1 = script, 2 = region, 3 = variants
        let stage: u8 = 0 as u8;
        let pos: usize = 0;

        while (true) {
            const start = pos;
            while (pos < input.len && !locIsSep(input[pos])) {
                pos = pos + 1;
            }
            const seg_len = pos - start;
            if (seg_len == 0) {
                return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_EMPTY_SUBTAG };
            }
            const seg = input[start..pos];

            if (stage == (0 as u8)) {
                const s8 = TinyAsciiStr8.fromBytes(seg);
                if (!s8.ok || !locValidLanguageSubtag(s8.value)) {
                    return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_INVALID_LANGUAGE };
                }
                loc.language = s8.value.toAsciiLowercase();
                stage = 1 as u8;
            } else {
                let consumed = false;

                if (stage == (1 as u8) && seg_len == 4) {
                    const s4 = TinyAsciiStr4.fromBytes(seg);
                    if (s4.ok && s4.value.isAsciiAlphabetic()) {
                        loc.has_script = true;
                        loc.script = s4.value.toAsciiTitlecase();
                        stage = 2 as u8;
                        consumed = true;
                    }
                }

                if (!consumed && stage <= (2 as u8) && (seg_len == 2 || seg_len == 3)) {
                    const r4 = TinyAsciiStr4.fromBytes(seg);
                    if (r4.ok) {
                        const is_alpha_reg = seg_len == 2 && r4.value.isAsciiAlphabetic();
                        const is_num_reg = seg_len == 3 && r4.value.isAsciiNumeric();
                        if (is_alpha_reg || is_num_reg) {
                            loc.has_region = true;
                            loc.region = r4.value.toAsciiUppercase();
                            stage = 3 as u8;
                            consumed = true;
                        }
                    }
                }

                if (!consumed) {
                    const v8 = TinyAsciiStr8.fromBytes(seg);
                    if (!v8.ok || !locValidVariantSubtag(v8.value)) {
                        return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_INVALID_SUBTAG };
                    }
                    const norm_v = v8.value.toAsciiLowercase();
                    // Check duplicates
                    let vi: usize = 0;
                    while (vi < loc.variant_count) {
                        if (loc.variants[vi].eq(norm_v)) {
                            return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_DUPLICATE_VARIANT };
                        }
                        vi = vi + 1;
                    }
                    if (loc.variant_count >= LOCALE_MAX_VARIANTS) {
                        return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_TOO_MANY_VARIANTS };
                    }
                    // Insert in sorted lexicographical order (UTS #35 canonical variant ordering)
                    let ins: usize = loc.variant_count;
                    while (ins > 0 && norm_v.cmp(loc.variants[ins - 1]) < 0) {
                        loc.variants[ins] = loc.variants[ins - 1];
                        ins = ins - 1;
                    }
                    loc.variants[ins] = norm_v;
                    loc.variant_count = loc.variant_count + 1;
                    stage = 3 as u8;
                }
            }

            if (pos >= input.len) {
                break;
            }
            // Skip separator; trailing separator is an empty subtag error
            pos = pos + 1;
            if (pos >= input.len) {
                return LocaleParseResult { ok: false, locale: LanguageIdentifier.und(), err: LOCALE_ERR_EMPTY_SUBTAG };
            }
        }

        return LocaleParseResult { ok: true, locale: loc, err: LOCALE_OK };
    }

    /// Formats the canonical BCP-47 hyphen-separated tag into `out` (e.g. `"zh-Hans-CN"`).
    /// Returns the number of bytes written (0 if buffer is too small).
    pub fn toStr(out: []u8): usize {
        const lang_len = _.language.len();
        let total = lang_len;
        if (_.has_script) {
            total = total + 1 + _.script.len();
        }
        if (_.has_region) {
            total = total + 1 + _.region.len();
        }
        let vi: usize = 0;
        while (vi < _.variant_count) {
            total = total + 1 + _.variants[vi].len();
            vi = vi + 1;
        }
        if (out.len < total) {
            return 0;
        }

        let pos: usize = _.language.asBytes(out[0..out.len]);
        if (_.has_script) {
            out[pos] = '-' as u8;
            pos = pos + 1;
            pos = pos + _.script.asBytes(out[pos..out.len]);
        }
        if (_.has_region) {
            out[pos] = '-' as u8;
            pos = pos + 1;
            pos = pos + _.region.asBytes(out[pos..out.len]);
        }
        vi = 0;
        while (vi < _.variant_count) {
            out[pos] = '-' as u8;
            pos = pos + 1;
            pos = pos + _.variants[vi].asBytes(out[pos..out.len]);
            vi = vi + 1;
        }
        return pos;
    }

    /// Returns true if the language subtag is `"und"` (undetermined).
    pub fn isUnd(): bool {
        return locLangEqStr(_.language, "und");
    }

    /// Exact structural equality across language, script, region, and canonical variants.
    pub fn eq(other: LanguageIdentifier): bool {
        if (!_.language.eq(other.language)) {
            return false;
        }
        if (_.has_script != other.has_script) {
            return false;
        }
        if (_.has_script && !_.script.eq(other.script)) {
            return false;
        }
        if (_.has_region != other.has_region) {
            return false;
        }
        if (_.has_region && !_.region.eq(other.region)) {
            return false;
        }
        if (_.variant_count != other.variant_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.variant_count) {
            if (!_.variants[i].eq(other.variants[i])) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    /// Partial subtag compatibility match (`und` acts as language wildcard; absent script/region
    /// on either side matches any value on the other side).
    pub fn matches(other: LanguageIdentifier): bool {
        if (!_.isUnd() && !other.isUnd() && !_.language.eq(other.language)) {
            return false;
        }
        if (_.has_script && other.has_script && !_.script.eq(other.script)) {
            return false;
        }
        if (_.has_region && other.has_region && !_.region.eq(other.region)) {
            return false;
        }
        return true;
    }

    /// Applies CLDR Add Likely Subtags maximization for common world languages
    /// (e.g. `zh -> zh-Hans-CN`, `zh-TW -> zh-Hant-TW`, `en -> en-Latn-US`,
    /// `ja -> ja-Jpan-JP`, `ko -> ko-Kore-KR`, `ar -> ar-Arab-EG`, `ru -> ru-Cyrl-RU`).
    pub fn maximizeLikelySubtags(): LanguageIdentifier {
        let res = LanguageIdentifier {
            language: _.language,
            has_script: _.has_script,
            script: _.script,
            has_region: _.has_region,
            region: _.region,
            variant_count: _.variant_count,
            variants: _.variants,
        };

        if (locLangEqStr(res.language, "und")) {
            res.language = TinyAsciiStr8.fromStr("en").value;
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Latn").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("US").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "zh")) {
            if (!res.has_script) {
                res.has_script = true;
                if (res.has_region && (locRegionEqStr(res.region, "TW") || locRegionEqStr(res.region, "HK") || locRegionEqStr(res.region, "MO"))) {
                    res.script = TinyAsciiStr4.fromStr("Hant").value;
                } else {
                    res.script = TinyAsciiStr4.fromStr("Hans").value;
                }
            }
            if (!res.has_region) {
                res.has_region = true;
                if (locRegionEqStr(res.script, "Hant")) {
                    res.region = TinyAsciiStr4.fromStr("TW").value;
                } else {
                    res.region = TinyAsciiStr4.fromStr("CN").value;
                }
            }
            return res;
        }

        if (locLangEqStr(res.language, "en")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Latn").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("US").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "ja")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Jpan").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("JP").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "ko")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Kore").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("KR").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "ar")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Arab").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("EG").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "ru")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Cyrl").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("RU").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "fr")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Latn").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("FR").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "de")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Latn").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("DE").value;
            }
            return res;
        }

        if (locLangEqStr(res.language, "es")) {
            if (!res.has_script) {
                res.has_script = true;
                res.script = TinyAsciiStr4.fromStr("Latn").value;
            }
            if (!res.has_region) {
                res.has_region = true;
                res.region = TinyAsciiStr4.fromStr("ES").value;
            }
            return res;
        }

        return res;
    }
}

pub struct LocaleParseResult {
    pub ok: bool,
    pub locale: LanguageIdentifier,
    pub err: i64,
}
