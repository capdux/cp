// @cap/kleur — Pure CP zero-allocation composable chainable ANSI style builder.
// Clean-room MIT implementation inspired by npm `kleur` v4.
// Reuses ANSI SGR StylePair constants from `@cap/ansi-styles` with zero duplication.

import {
    StylePair,
    RESET,
    BOLD,
    DIM,
    ITALIC,
    UNDERLINE,
    INVERSE,
    HIDDEN,
    STRIKETHROUGH,
    FG_BLACK,
    FG_RED,
    FG_GREEN,
    FG_YELLOW,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_WHITE,
    FG_GRAY,
    BG_BLACK,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE
} from "../../ansi-styles/src/index.cp";

pub const KLEUR_MAX_DEPTH: usize = 8;

fn klrStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn klrWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn klrMatchAt(haystack: string, offset: usize, needle: string): bool {
    if (needle.len == 0 || offset + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[offset + i] != needle[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Zero-heap composable chainable ANSI style builder (up to 8 stacked styles).
/// Supports fluent immutable chaining: `KleurChain.new(true).bold().red().bgYellow().format(...)`.
pub struct KleurChain {
    pub enabled: bool,
    pub stack: [8]StylePair,
    pub depth: usize,

    pub static fn new(enabled: bool): KleurChain {
        return KleurChain {
            enabled: enabled,
            stack: [RESET; 8],
            depth: 0
        };
    }

    pub static fn on(): KleurChain {
        return KleurChain.new(true);
    }

    pub static fn off(): KleurChain {
        return KleurChain.new(false);
    }

    pub fn depthCount(): usize {
        return _.depth;
    }

    pub fn withStyle(pair: StylePair): KleurChain {
        let next = KleurChain {
            enabled: _.enabled,
            stack: _.stack,
            depth: _.depth
        };
        if (next.depth < KLEUR_MAX_DEPTH) {
            next.stack[next.depth] = pair;
            next.depth = next.depth + 1;
        }
        return next;
    }

    // Modifiers
    pub fn reset(): KleurChain { return _.withStyle(RESET); }
    pub fn bold(): KleurChain { return _.withStyle(BOLD); }
    pub fn dim(): KleurChain { return _.withStyle(DIM); }
    pub fn italic(): KleurChain { return _.withStyle(ITALIC); }
    pub fn underline(): KleurChain { return _.withStyle(UNDERLINE); }
    pub fn inverse(): KleurChain { return _.withStyle(INVERSE); }
    pub fn hidden(): KleurChain { return _.withStyle(HIDDEN); }
    pub fn strikethrough(): KleurChain { return _.withStyle(STRIKETHROUGH); }

    // Foreground colors
    pub fn black(): KleurChain { return _.withStyle(FG_BLACK); }
    pub fn red(): KleurChain { return _.withStyle(FG_RED); }
    pub fn green(): KleurChain { return _.withStyle(FG_GREEN); }
    pub fn yellow(): KleurChain { return _.withStyle(FG_YELLOW); }
    pub fn blue(): KleurChain { return _.withStyle(FG_BLUE); }
    pub fn magenta(): KleurChain { return _.withStyle(FG_MAGENTA); }
    pub fn cyan(): KleurChain { return _.withStyle(FG_CYAN); }
    pub fn white(): KleurChain { return _.withStyle(FG_WHITE); }
    pub fn gray(): KleurChain { return _.withStyle(FG_GRAY); }

    // Background colors
    pub fn bgBlack(): KleurChain { return _.withStyle(BG_BLACK); }
    pub fn bgRed(): KleurChain { return _.withStyle(BG_RED); }
    pub fn bgGreen(): KleurChain { return _.withStyle(BG_GREEN); }
    pub fn bgYellow(): KleurChain { return _.withStyle(BG_YELLOW); }
    pub fn bgBlue(): KleurChain { return _.withStyle(BG_BLUE); }
    pub fn bgMagenta(): KleurChain { return _.withStyle(BG_MAGENTA); }
    pub fn bgCyan(): KleurChain { return _.withStyle(BG_CYAN); }
    pub fn bgWhite(): KleurChain { return _.withStyle(BG_WHITE); }

    /// Formats `text` into `out` applying all active styles in order,
    /// recovering from nested close or `\x1b[0m` reset sequences, and closing in reverse order.
    /// Passes `text` through verbatim when `enabled == false` or `depth == 0`.
    pub fn format(text: string, out: []u8): usize {
        if (text.len == 0 || out.len == 0) {
            return 0;
        }
        if (!_.enabled || _.depth == 0) {
            return klrWriteStr(out, 0, text);
        }

        let p: usize = 0;

        // 1. Emit open codes in forward stack order
        let k: usize = 0;
        while (k < _.depth && p < out.len) {
            p = klrWriteStr(out, p, _.stack[k].open);
            k = k + 1;
        }

        // 2. Emit text with nested close/reset recovery
        let i: usize = 0;
        while (i < text.len && p < out.len) {
            // Check for embedded full reset `\x1b[0m`
            if (klrMatchAt(text, i, RESET.close)) {
                p = klrWriteStr(out, p, RESET.close);
                let r: usize = 0;
                while (r < _.depth && p < out.len) {
                    p = klrWriteStr(out, p, _.stack[r].open);
                    r = r + 1;
                }
                i = i + RESET.close.len;
                continue;
            }

            // Check if any active layer's close sequence appears in `text`
            let matched_idx: i64 = -1;
            let s: usize = _.depth;
            while (s > 0) {
                s = s - 1;
                if (klrMatchAt(text, i, _.stack[s].close)) {
                    matched_idx = s as i64;
                    break;
                }
            }

            if (matched_idx >= 0) {
                let layer = _.stack[matched_idx as usize];
                if (klrStrEq(layer.close, "\x1b[22m")) {
                    p = klrWriteStr(out, p, layer.close);
                }
                p = klrWriteStr(out, p, layer.open);
                i = i + layer.close.len;
            } else {
                out[p] = text[i];
                p = p + 1;
                i = i + 1;
            }
        }

        // 3. Emit close codes in reverse stack order
        let c: usize = _.depth;
        while (c > 0 && p < out.len) {
            c = c - 1;
            p = klrWriteStr(out, p, _.stack[c].close);
        }

        return p;
    }
}

export {
    KLEUR_MAX_DEPTH,
    KleurChain
};
