// @cap/language-tags — Zero-heap BCP 47 / RFC 5646 parser, canonicalizer & RFC 4647 matcher

pub struct Bcp47Tag {
    language: string,
    script: string,
    region: string,
    variant: string,
    valid: bool,
}

fn ltgIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn ltgIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn ltgIsAlnum(c: u8): bool {
    return ltgIsAlpha(c) || ltgIsDigit(c);
}

fn ltgIsSep(c: u8): bool {
    return c == 45 || c == 95;
}

fn ltgToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn ltgToUpper(c: u8): u8 {
    if (c >= 97 && c <= 122) {
        return c - 32;
    }
    return c;
}

fn ltgAllAlpha(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!ltgIsAlpha(s[i])) {
            return false;
        }
    }
    return s.len > 0;
}

fn ltgAllDigit(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!ltgIsDigit(s[i])) {
            return false;
        }
    }
    return s.len > 0;
}

fn ltgAllAlnum(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!ltgIsAlnum(s[i])) {
            return false;
        }
    }
    return s.len > 0;
}

fn ltgInvalidTag(): Bcp47Tag {
    return Bcp47Tag{
        language: "",
        script: "",
        region: "",
        variant: "",
        valid: false,
    };
}

pub fn parseLanguageTag(tag: string): Bcp47Tag {
    const n: usize = tag.len;
    if (n == 0) {
        return ltgInvalidTag();
    }

    let lang: string = "";
    let script: string = "";
    let region: string = "";
    let variant: string = "";

    // stage: 0=expect lang, 1=after lang, 2=after script, 3=after region, 4=after variant
    let stage: u8 = 0;
    let start: usize = 0;
    let i: usize = 0;

    while (i <= n) : (i += 1) {
        if (i == n || ltgIsSep(tag[i])) {
            const sub: string = tag[start..i];
            if (sub.len == 0) {
                return ltgInvalidTag();
            }

            if (stage == 0) {
                // Primary language: 2..=8 ASCII alpha (ISO 639 2-3 or registered 5-8)
                if (sub.len < 2 || sub.len > 8 || !ltgAllAlpha(sub)) {
                    return ltgInvalidTag();
                }
                lang = sub;
                stage = 1;
            } else if (stage < 2 && sub.len == 4 && ltgAllAlpha(sub)) {
                // Script: 4 ASCII alpha (ISO 15924)
                script = sub;
                stage = 2;
            } else if (stage < 3 && ((sub.len == 2 && ltgAllAlpha(sub)) || (sub.len == 3 && ltgAllDigit(sub)))) {
                // Region: 2 alpha (ISO 3166-1) or 3 digit (UN M.49)
                region = sub;
                stage = 3;
            } else if ((sub.len >= 5 && sub.len <= 8 && ltgAllAlnum(sub)) || (sub.len == 4 && ltgIsDigit(sub[0]) && ltgAllAlnum(sub))) {
                // Variant: 5..=8 alnum or digit + 3 alnum
                if (variant.len == 0) {
                    variant = sub;
                }
                stage = 4;
            } else {
                return ltgInvalidTag();
            }

            start = i + 1;
        }
    }

    return Bcp47Tag{
        language: lang,
        script: script,
        region: region,
        variant: variant,
        valid: true,
    };
}

fn ltgPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

pub fn formatCanonicalTag(t: Bcp47Tag, out: []u8): usize {
    if (!t.valid || t.language.len == 0) {
        return 0;
    }
    let pos: usize = 0;

    // language: all lowercase
    let i: usize = 0;
    while (i < t.language.len) : (i += 1) {
        pos = ltgPushByte(out, pos, ltgToLower(t.language[i]));
    }

    // Script: Title Case
    if (t.script.len > 0) {
        pos = ltgPushByte(out, pos, 45);
        let si: usize = 0;
        while (si < t.script.len) : (si += 1) {
            const ch: u8 = if (si == 0) ltgToUpper(t.script[si]) else ltgToLower(t.script[si]);
            pos = ltgPushByte(out, pos, ch);
        }
    }

    // REGION: ALL UPPERCASE
    if (t.region.len > 0) {
        pos = ltgPushByte(out, pos, 45);
        let ri: usize = 0;
        while (ri < t.region.len) : (ri += 1) {
            pos = ltgPushByte(out, pos, ltgToUpper(t.region[ri]));
        }
    }

    // variant: all lowercase
    if (t.variant.len > 0) {
        pos = ltgPushByte(out, pos, 45);
        let vi: usize = 0;
        while (vi < t.variant.len) : (vi += 1) {
            pos = ltgPushByte(out, pos, ltgToLower(t.variant[vi]));
        }
    }

    return pos;
}

pub fn matchesLanguageRange(tag: string, range: string): bool {
    if (tag.len == 0 || range.len == 0) {
        return false;
    }
    if (range.len == 1 && range[0] == 42) {
        return true;
    }
    if (range.len > tag.len) {
        return false;
    }
    let i: usize = 0;
    while (i < range.len) : (i += 1) {
        const tc: u8 = if (ltgIsSep(tag[i])) 45 else ltgToLower(tag[i]);
        const rc: u8 = if (ltgIsSep(range[i])) 45 else ltgToLower(range[i]);
        if (tc != rc) {
            return false;
        }
    }
    if (tag.len == range.len) {
        return true;
    }
    return ltgIsSep(tag[range.len]);
}
