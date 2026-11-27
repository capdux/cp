// @cap/color-name — Zero-allocation W3C CSS Color Level 4 named color table (148 colors)
// Clean-room MIT implementation in pure CP with case-insensitive O(log n) binary search.

pub const CSS_COLOR_COUNT: usize = 148;

pub struct RgbColor {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub found: bool,
}

pub struct ColorEntry {
    pub name: string,
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

// Strictly sorted in ascending ASCII alphabetical order for O(log n) binary search.
const CN_TABLE: [148]ColorEntry = [
    ColorEntry { name: "aliceblue", r: 240, g: 248, b: 255 },
    ColorEntry { name: "antiquewhite", r: 250, g: 235, b: 215 },
    ColorEntry { name: "aqua", r: 0, g: 255, b: 255 },
    ColorEntry { name: "aquamarine", r: 127, g: 255, b: 212 },
    ColorEntry { name: "azure", r: 240, g: 255, b: 255 },
    ColorEntry { name: "beige", r: 245, g: 245, b: 220 },
    ColorEntry { name: "bisque", r: 255, g: 228, b: 196 },
    ColorEntry { name: "black", r: 0, g: 0, b: 0 },
    ColorEntry { name: "blanchedalmond", r: 255, g: 235, b: 205 },
    ColorEntry { name: "blue", r: 0, g: 0, b: 255 },
    ColorEntry { name: "blueviolet", r: 138, g: 43, b: 226 },
    ColorEntry { name: "brown", r: 165, g: 42, b: 42 },
    ColorEntry { name: "burlywood", r: 222, g: 184, b: 135 },
    ColorEntry { name: "cadetblue", r: 95, g: 158, b: 160 },
    ColorEntry { name: "chartreuse", r: 127, g: 255, b: 0 },
    ColorEntry { name: "chocolate", r: 210, g: 105, b: 30 },
    ColorEntry { name: "coral", r: 255, g: 127, b: 80 },
    ColorEntry { name: "cornflowerblue", r: 100, g: 149, b: 237 },
    ColorEntry { name: "cornsilk", r: 255, g: 248, b: 220 },
    ColorEntry { name: "crimson", r: 220, g: 20, b: 60 },
    ColorEntry { name: "cyan", r: 0, g: 255, b: 255 },
    ColorEntry { name: "darkblue", r: 0, g: 0, b: 139 },
    ColorEntry { name: "darkcyan", r: 0, g: 139, b: 139 },
    ColorEntry { name: "darkgoldenrod", r: 184, g: 134, b: 11 },
    ColorEntry { name: "darkgray", r: 169, g: 169, b: 169 },
    ColorEntry { name: "darkgreen", r: 0, g: 100, b: 0 },
    ColorEntry { name: "darkgrey", r: 169, g: 169, b: 169 },
    ColorEntry { name: "darkkhaki", r: 189, g: 183, b: 107 },
    ColorEntry { name: "darkmagenta", r: 139, g: 0, b: 139 },
    ColorEntry { name: "darkolivegreen", r: 85, g: 107, b: 47 },
    ColorEntry { name: "darkorange", r: 255, g: 140, b: 0 },
    ColorEntry { name: "darkorchid", r: 153, g: 50, b: 204 },
    ColorEntry { name: "darkred", r: 139, g: 0, b: 0 },
    ColorEntry { name: "darksalmon", r: 233, g: 150, b: 122 },
    ColorEntry { name: "darkseagreen", r: 143, g: 188, b: 143 },
    ColorEntry { name: "darkslateblue", r: 72, g: 61, b: 139 },
    ColorEntry { name: "darkslategray", r: 47, g: 79, b: 79 },
    ColorEntry { name: "darkslategrey", r: 47, g: 79, b: 79 },
    ColorEntry { name: "darkturquoise", r: 0, g: 206, b: 209 },
    ColorEntry { name: "darkviolet", r: 148, g: 0, b: 211 },
    ColorEntry { name: "deeppink", r: 255, g: 20, b: 147 },
    ColorEntry { name: "deepskyblue", r: 0, g: 191, b: 255 },
    ColorEntry { name: "dimgray", r: 105, g: 105, b: 105 },
    ColorEntry { name: "dimgrey", r: 105, g: 105, b: 105 },
    ColorEntry { name: "dodgerblue", r: 30, g: 144, b: 255 },
    ColorEntry { name: "firebrick", r: 178, g: 34, b: 34 },
    ColorEntry { name: "floralwhite", r: 255, g: 250, b: 240 },
    ColorEntry { name: "forestgreen", r: 34, g: 139, b: 34 },
    ColorEntry { name: "fuchsia", r: 255, g: 0, b: 255 },
    ColorEntry { name: "gainsboro", r: 220, g: 220, b: 220 },
    ColorEntry { name: "ghostwhite", r: 248, g: 248, b: 255 },
    ColorEntry { name: "gold", r: 255, g: 215, b: 0 },
    ColorEntry { name: "goldenrod", r: 218, g: 165, b: 32 },
    ColorEntry { name: "gray", r: 128, g: 128, b: 128 },
    ColorEntry { name: "green", r: 0, g: 128, b: 0 },
    ColorEntry { name: "greenyellow", r: 173, g: 255, b: 47 },
    ColorEntry { name: "grey", r: 128, g: 128, b: 128 },
    ColorEntry { name: "honeydew", r: 240, g: 255, b: 240 },
    ColorEntry { name: "hotpink", r: 255, g: 105, b: 180 },
    ColorEntry { name: "indianred", r: 205, g: 92, b: 92 },
    ColorEntry { name: "indigo", r: 75, g: 0, b: 130 },
    ColorEntry { name: "ivory", r: 255, g: 255, b: 240 },
    ColorEntry { name: "khaki", r: 240, g: 230, b: 140 },
    ColorEntry { name: "lavender", r: 230, g: 230, b: 250 },
    ColorEntry { name: "lavenderblush", r: 255, g: 240, b: 245 },
    ColorEntry { name: "lawngreen", r: 124, g: 252, b: 0 },
    ColorEntry { name: "lemonchiffon", r: 255, g: 250, b: 205 },
    ColorEntry { name: "lightblue", r: 173, g: 216, b: 230 },
    ColorEntry { name: "lightcoral", r: 240, g: 128, b: 128 },
    ColorEntry { name: "lightcyan", r: 224, g: 255, b: 255 },
    ColorEntry { name: "lightgoldenrodyellow", r: 250, g: 250, b: 210 },
    ColorEntry { name: "lightgray", r: 211, g: 211, b: 211 },
    ColorEntry { name: "lightgreen", r: 144, g: 238, b: 144 },
    ColorEntry { name: "lightgrey", r: 211, g: 211, b: 211 },
    ColorEntry { name: "lightpink", r: 255, g: 182, b: 193 },
    ColorEntry { name: "lightsalmon", r: 255, g: 160, b: 122 },
    ColorEntry { name: "lightseagreen", r: 32, g: 178, b: 170 },
    ColorEntry { name: "lightskyblue", r: 135, g: 206, b: 250 },
    ColorEntry { name: "lightslategray", r: 119, g: 136, b: 153 },
    ColorEntry { name: "lightslategrey", r: 119, g: 136, b: 153 },
    ColorEntry { name: "lightsteelblue", r: 176, g: 196, b: 222 },
    ColorEntry { name: "lightyellow", r: 255, g: 255, b: 224 },
    ColorEntry { name: "lime", r: 0, g: 255, b: 0 },
    ColorEntry { name: "limegreen", r: 50, g: 205, b: 50 },
    ColorEntry { name: "linen", r: 250, g: 240, b: 230 },
    ColorEntry { name: "magenta", r: 255, g: 0, b: 255 },
    ColorEntry { name: "maroon", r: 128, g: 0, b: 0 },
    ColorEntry { name: "mediumaquamarine", r: 102, g: 205, b: 170 },
    ColorEntry { name: "mediumblue", r: 0, g: 0, b: 205 },
    ColorEntry { name: "mediumorchid", r: 186, g: 85, b: 211 },
    ColorEntry { name: "mediumpurple", r: 147, g: 112, b: 219 },
    ColorEntry { name: "mediumseagreen", r: 60, g: 179, b: 113 },
    ColorEntry { name: "mediumslateblue", r: 123, g: 104, b: 238 },
    ColorEntry { name: "mediumspringgreen", r: 0, g: 250, b: 154 },
    ColorEntry { name: "mediumturquoise", r: 72, g: 209, b: 204 },
    ColorEntry { name: "mediumvioletred", r: 199, g: 21, b: 133 },
    ColorEntry { name: "midnightblue", r: 25, g: 25, b: 112 },
    ColorEntry { name: "mintcream", r: 245, g: 255, b: 250 },
    ColorEntry { name: "mistyrose", r: 255, g: 228, b: 225 },
    ColorEntry { name: "moccasin", r: 255, g: 228, b: 181 },
    ColorEntry { name: "navajowhite", r: 255, g: 222, b: 173 },
    ColorEntry { name: "navy", r: 0, g: 0, b: 128 },
    ColorEntry { name: "oldlace", r: 253, g: 245, b: 230 },
    ColorEntry { name: "olive", r: 128, g: 128, b: 0 },
    ColorEntry { name: "olivedrab", r: 107, g: 142, b: 35 },
    ColorEntry { name: "orange", r: 255, g: 165, b: 0 },
    ColorEntry { name: "orangered", r: 255, g: 69, b: 0 },
    ColorEntry { name: "orchid", r: 218, g: 112, b: 214 },
    ColorEntry { name: "palegoldenrod", r: 238, g: 232, b: 170 },
    ColorEntry { name: "palegreen", r: 152, g: 251, b: 152 },
    ColorEntry { name: "paleturquoise", r: 175, g: 238, b: 238 },
    ColorEntry { name: "palevioletred", r: 219, g: 112, b: 147 },
    ColorEntry { name: "papayawhip", r: 255, g: 239, b: 213 },
    ColorEntry { name: "peachpuff", r: 255, g: 218, b: 185 },
    ColorEntry { name: "peru", r: 205, g: 133, b: 63 },
    ColorEntry { name: "pink", r: 255, g: 192, b: 203 },
    ColorEntry { name: "plum", r: 221, g: 160, b: 221 },
    ColorEntry { name: "powderblue", r: 176, g: 224, b: 230 },
    ColorEntry { name: "purple", r: 128, g: 0, b: 128 },
    ColorEntry { name: "rebeccapurple", r: 102, g: 51, b: 153 },
    ColorEntry { name: "red", r: 255, g: 0, b: 0 },
    ColorEntry { name: "rosybrown", r: 188, g: 143, b: 143 },
    ColorEntry { name: "royalblue", r: 65, g: 105, b: 225 },
    ColorEntry { name: "saddlebrown", r: 139, g: 69, b: 19 },
    ColorEntry { name: "salmon", r: 250, g: 128, b: 114 },
    ColorEntry { name: "sandybrown", r: 244, g: 164, b: 96 },
    ColorEntry { name: "seagreen", r: 46, g: 139, b: 87 },
    ColorEntry { name: "seashell", r: 255, g: 245, b: 238 },
    ColorEntry { name: "sienna", r: 160, g: 82, b: 45 },
    ColorEntry { name: "silver", r: 192, g: 192, b: 192 },
    ColorEntry { name: "skyblue", r: 135, g: 206, b: 235 },
    ColorEntry { name: "slateblue", r: 106, g: 90, b: 205 },
    ColorEntry { name: "slategray", r: 112, g: 128, b: 144 },
    ColorEntry { name: "slategrey", r: 112, g: 128, b: 144 },
    ColorEntry { name: "snow", r: 255, g: 250, b: 250 },
    ColorEntry { name: "springgreen", r: 0, g: 255, b: 127 },
    ColorEntry { name: "steelblue", r: 70, g: 130, b: 180 },
    ColorEntry { name: "tan", r: 210, g: 180, b: 140 },
    ColorEntry { name: "teal", r: 0, g: 128, b: 128 },
    ColorEntry { name: "thistle", r: 216, g: 191, b: 216 },
    ColorEntry { name: "tomato", r: 255, g: 99, b: 71 },
    ColorEntry { name: "turquoise", r: 64, g: 224, b: 208 },
    ColorEntry { name: "violet", r: 238, g: 130, b: 238 },
    ColorEntry { name: "wheat", r: 245, g: 222, b: 179 },
    ColorEntry { name: "white", r: 255, g: 255, b: 255 },
    ColorEntry { name: "whitesmoke", r: 245, g: 245, b: 245 },
    ColorEntry { name: "yellow", r: 255, g: 255, b: 0 },
    ColorEntry { name: "yellowgreen", r: 154, g: 205, b: 50 },
];

fn cnToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

/// Case-insensitive lexicographical comparison: returns -1 if a < b, 0 if a == b, 1 if a > b.
fn cnCmpIgnoreCase(a: string, b: string): i8 {
    let min_len: usize = a.len;
    if (b.len < min_len) {
        min_len = b.len;
    }
    let i: usize = 0;
    while (i < min_len) {
        const ca: u8 = cnToLower(a[i]);
        const cb: u8 = cnToLower(b[i]);
        if (ca < cb) {
            return -1;
        }
        if (ca > cb) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

/// Return the `ColorEntry` at table index `index` (`0..CSS_COLOR_COUNT`).
pub fn colorAt(index: usize): ColorEntry {
    if (index >= CSS_COLOR_COUNT) {
        return ColorEntry { name: "", r: 0, g: 0, b: 0 };
    }
    return CN_TABLE[index];
}

/// Case-insensitive O(log n) binary lookup of any W3C CSS Color Level 4 named color.
pub fn lookupColor(name: string): RgbColor {
    if (name.len == 0) {
        return RgbColor { r: 0, g: 0, b: 0, found: false };
    }
    let lo: usize = 0;
    let hi: usize = CSS_COLOR_COUNT;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        const entry: ColorEntry = CN_TABLE[mid];
        const cmp: i8 = cnCmpIgnoreCase(name, entry.name);
        if (cmp == 0) {
            return RgbColor { r: entry.r, g: entry.g, b: entry.b, found: true };
        }
        if (cmp < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return RgbColor { r: 0, g: 0, b: 0, found: false };
}

/// Look up a CSS named color and pack it into `0xRRGGBB`, or return `-1` if not found.
pub fn lookupColorHex(name: string): i32 {
    const c: RgbColor = lookupColor(name);
    if (!c.found) {
        return -1;
    }
    return ((c.r as i32) << (16 as u5)) | ((c.g as i32) << (8 as u5)) | (c.b as i32);
}

/// Reverse-lookup a standard CSS color name from exact `(r, g, b)` values, or `""` if unmatched.
pub fn reverseLookupColor(r: u8, g: u8, b: u8): string {
    let i: usize = 0;
    while (i < CSS_COLOR_COUNT) {
        const e: ColorEntry = CN_TABLE[i];
        if (e.r == r && e.g == g && e.b == b) {
            return e.name;
        }
        i += 1;
    }
    return "";
}

/// Return `true` if `name` is a valid W3C CSS4 named color (case-insensitive).
pub fn isNamedColor(name: string): bool {
    return lookupColor(name).found;
}

export {
    CSS_COLOR_COUNT,
    RgbColor,
    ColorEntry,
    colorAt,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
    isNamedColor,
};
