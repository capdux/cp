// @cap/is-potential-custom-element-name — Zero-allocation WHATWG Custom Element Name validator in pure CP
// Clean-room MIT implementation of npm `is-potential-custom-element-name` (#407, 132M/wk).

fn pcenStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

/// Returns `true` if `cp` is a valid WHATWG `PCENChar` (HTML Standard §4.13.2).
pub fn isPcenChar(cp: u32): bool {
    if (cp == 0x2D || cp == 0x2E || cp == 0x5F) { return true; } // '-', '.', '_'
    if (cp >= 0x30 && cp <= 0x39) { return true; } // '0'-'9'
    if (cp >= 0x61 && cp <= 0x7A) { return true; } // 'a'-'z'
    if (cp == 0xB7) { return true; }
    if (cp >= 0xC0 && cp <= 0xD6) { return true; }
    if (cp >= 0xD8 && cp <= 0xF6) { return true; }
    if (cp >= 0xF8 && cp <= 0x37D) { return true; }
    if (cp >= 0x37F && cp <= 0x1FFF) { return true; }
    if (cp == 0x200C || cp == 0x200D || cp == 0x203F || cp == 0x2040) { return true; }
    if (cp >= 0x2070 && cp <= 0x218F) { return true; }
    if (cp >= 0x2C00 && cp <= 0x2FEF) { return true; }
    if (cp >= 0x3001 && cp <= 0xD7FF) { return true; }
    if (cp >= 0xF900 && cp <= 0xFDCF) { return true; }
    if (cp >= 0xFDF0 && cp <= 0xFFFD) { return true; }
    if (cp >= 0x10000 && cp <= 0xEFFFF) { return true; }
    return false;
}

/// Returns `true` if `name` is one of the 8 reserved hyphenated SVG/MathML element names.
pub fn isReservedCustomElementName(name: string): bool {
    return pcenStrEq(name, "annotation-xml")
        || pcenStrEq(name, "color-profile")
        || pcenStrEq(name, "font-face")
        || pcenStrEq(name, "font-face-src")
        || pcenStrEq(name, "font-face-uri")
        || pcenStrEq(name, "font-face-format")
        || pcenStrEq(name, "font-face-name")
        || pcenStrEq(name, "missing-glyph");
}

/// Validates whether `name` is a legal WHATWG `PotentialCustomElementName` (HTML §4.13.2).
pub fn isPotentialCustomElementName(name: string): bool {
    if (name.len < 2) {
        return false;
    }
    // Must start with ASCII lowercase [a-z]
    if (name[0] < 0x61 || name[0] > 0x7A) {
        return false;
    }
    if (isReservedCustomElementName(name)) {
        return false;
    }
    let has_hyphen = false;
    let i: usize = 1;
    while (i < name.len) {
        const b0 = name[i];
        let cp: u32 = 0;
        let step: usize = 1;
        if (b0 < 0x80) {
            cp = b0 as u32;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < name.len && (name[i + 1] & 0xC0) == 0x80) {
            cp = (((b0 & 0x1F) as u32) << 6) | ((name[i + 1] & 0x3F) as u32);
            step = 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < name.len && (name[i + 1] & 0xC0) == 0x80 && (name[i + 2] & 0xC0) == 0x80) {
            cp = (((b0 & 0x0F) as u32) << 12) | (((name[i + 1] & 0x3F) as u32) << 6) | ((name[i + 2] & 0x3F) as u32);
            step = 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < name.len && (name[i + 1] & 0xC0) == 0x80 && (name[i + 2] & 0xC0) == 0x80 && (name[i + 3] & 0xC0) == 0x80) {
            cp = (((b0 & 0x07) as u32) << 18) | (((name[i + 1] & 0x3F) as u32) << 12) | (((name[i + 2] & 0x3F) as u32) << 6) | ((name[i + 3] & 0x3F) as u32);
            step = 4;
        } else {
            return false;
        }
        if (cp == 0x2D) {
            has_hyphen = true;
        }
        if (!isPcenChar(cp)) {
            return false;
        }
        i += step;
    }
    return has_hyphen;
}

export {
    isPcenChar,
    isReservedCustomElementName,
    isPotentialCustomElementName,
};
