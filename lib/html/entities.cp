// Source: cp/lib/html/entities.cp (WHATWG HTML5 Character Reference Codec)
// WHATWG HTML5 Entity Encoder & Decoder with C1 Windows-1252 Replacements

const std = @import("std");
import { Utf8 } from "cap:string";

struct HtmlEntityEntry {
    name: []const u8,
    utf8: []const u8,
}

// Strictly sorted in ASCII lexicographical order for O(log N) binary search
const HTML5_NAMED_ENTITIES: [206]HtmlEntityEntry = [
    HtmlEntityEntry{ name: "&AElig;", utf8: "\xC3\x86" },
    HtmlEntityEntry{ name: "&AMP;", utf8: "&" },
    HtmlEntityEntry{ name: "&Aacute;", utf8: "\xC3\x81" },
    HtmlEntityEntry{ name: "&Acirc;", utf8: "\xC3\x82" },
    HtmlEntityEntry{ name: "&Agrave;", utf8: "\xC3\x80" },
    HtmlEntityEntry{ name: "&Alpha;", utf8: "\xCE\x91" },
    HtmlEntityEntry{ name: "&Aring;", utf8: "\xC3\x85" },
    HtmlEntityEntry{ name: "&Atilde;", utf8: "\xC3\x83" },
    HtmlEntityEntry{ name: "&Auml;", utf8: "\xC3\x84" },
    HtmlEntityEntry{ name: "&Beta;", utf8: "\xCE\x92" },
    HtmlEntityEntry{ name: "&COPY;", utf8: "\xC2\xA9" },
    HtmlEntityEntry{ name: "&Ccedil;", utf8: "\xC3\x87" },
    HtmlEntityEntry{ name: "&Chi;", utf8: "\xCE\xA7" },
    HtmlEntityEntry{ name: "&Dagger;", utf8: "\xE2\x80\xA1" },
    HtmlEntityEntry{ name: "&Delta;", utf8: "\xCE\x94" },
    HtmlEntityEntry{ name: "&Eacute;", utf8: "\xC3\x89" },
    HtmlEntityEntry{ name: "&Ecirc;", utf8: "\xC3\x8A" },
    HtmlEntityEntry{ name: "&Egrave;", utf8: "\xC3\x88" },
    HtmlEntityEntry{ name: "&Epsilon;", utf8: "\xCE\x95" },
    HtmlEntityEntry{ name: "&Eta;", utf8: "\xCE\x97" },
    HtmlEntityEntry{ name: "&Euml;", utf8: "\xC3\x8B" },
    HtmlEntityEntry{ name: "&GT;", utf8: ">" },
    HtmlEntityEntry{ name: "&Gamma;", utf8: "\xCE\x93" },
    HtmlEntityEntry{ name: "&Iota;", utf8: "\xCE\x99" },
    HtmlEntityEntry{ name: "&Kappa;", utf8: "\xCE\x9A" },
    HtmlEntityEntry{ name: "&LT;", utf8: "<" },
    HtmlEntityEntry{ name: "&Lambda;", utf8: "\xCE\x9B" },
    HtmlEntityEntry{ name: "&Mu;", utf8: "\xCE\x9C" },
    HtmlEntityEntry{ name: "&Ntilde;", utf8: "\xC3\x91" },
    HtmlEntityEntry{ name: "&Nu;", utf8: "\xCE\x9D" },
    HtmlEntityEntry{ name: "&Omega;", utf8: "\xCE\xA9" },
    HtmlEntityEntry{ name: "&Omicron;", utf8: "\xCE\x9F" },
    HtmlEntityEntry{ name: "&Ouml;", utf8: "\xC3\x96" },
    HtmlEntityEntry{ name: "&Phi;", utf8: "\xCE\xA6" },
    HtmlEntityEntry{ name: "&Pi;", utf8: "\xCE\xA0" },
    HtmlEntityEntry{ name: "&Prime;", utf8: "\xE2\x80\xB3" },
    HtmlEntityEntry{ name: "&Psi;", utf8: "\xCE\xA8" },
    HtmlEntityEntry{ name: "&QUOT;", utf8: "\"" },
    HtmlEntityEntry{ name: "&REG;", utf8: "\xC2\xAE" },
    HtmlEntityEntry{ name: "&Rho;", utf8: "\xCE\xA1" },
    HtmlEntityEntry{ name: "&Sigma;", utf8: "\xCE\xA3" },
    HtmlEntityEntry{ name: "&Tau;", utf8: "\xCE\xA4" },
    HtmlEntityEntry{ name: "&Theta;", utf8: "\xCE\x98" },
    HtmlEntityEntry{ name: "&Upsilon;", utf8: "\xCE\xA5" },
    HtmlEntityEntry{ name: "&Uuml;", utf8: "\xC3\x9C" },
    HtmlEntityEntry{ name: "&Xi;", utf8: "\xCE\x9E" },
    HtmlEntityEntry{ name: "&Zeta;", utf8: "\xCE\x96" },
    HtmlEntityEntry{ name: "&aacute;", utf8: "\xC3\xA1" },
    HtmlEntityEntry{ name: "&acirc;", utf8: "\xC3\xA2" },
    HtmlEntityEntry{ name: "&aelig;", utf8: "\xC3\xA6" },
    HtmlEntityEntry{ name: "&agrave;", utf8: "\xC3\xA0" },
    HtmlEntityEntry{ name: "&alpha;", utf8: "\xCE\xB1" },
    HtmlEntityEntry{ name: "&amp;", utf8: "&" },
    HtmlEntityEntry{ name: "&and;", utf8: "\xE2\x88\xA7" },
    HtmlEntityEntry{ name: "&ang;", utf8: "\xE2\x88\xA0" },
    HtmlEntityEntry{ name: "&apos;", utf8: "'" },
    HtmlEntityEntry{ name: "&aring;", utf8: "\xC3\xA5" },
    HtmlEntityEntry{ name: "&asymp;", utf8: "\xE2\x89\x88" },
    HtmlEntityEntry{ name: "&atilde;", utf8: "\xC3\xA3" },
    HtmlEntityEntry{ name: "&auml;", utf8: "\xC3\xA4" },
    HtmlEntityEntry{ name: "&bdquo;", utf8: "\xE2\x80\x9E" },
    HtmlEntityEntry{ name: "&beta;", utf8: "\xCE\xB2" },
    HtmlEntityEntry{ name: "&bull;", utf8: "\xE2\x80\xA2" },
    HtmlEntityEntry{ name: "&cap;", utf8: "\xE2\x88\xA9" },
    HtmlEntityEntry{ name: "&ccedil;", utf8: "\xC3\xA7" },
    HtmlEntityEntry{ name: "&cent;", utf8: "\xC2\xA2" },
    HtmlEntityEntry{ name: "&check;", utf8: "\xE2\x9C\x93" },
    HtmlEntityEntry{ name: "&chi;", utf8: "\xCF\x87" },
    HtmlEntityEntry{ name: "&clubs;", utf8: "\xE2\x99\xA3" },
    HtmlEntityEntry{ name: "&cong;", utf8: "\xE2\x89\x85" },
    HtmlEntityEntry{ name: "&copy;", utf8: "\xC2\xA9" },
    HtmlEntityEntry{ name: "&crarr;", utf8: "\xE2\x86\xB5" },
    HtmlEntityEntry{ name: "&cross;", utf8: "\xE2\x9C\x97" },
    HtmlEntityEntry{ name: "&cup;", utf8: "\xE2\x88\xAA" },
    HtmlEntityEntry{ name: "&curren;", utf8: "\xC2\xA4" },
    HtmlEntityEntry{ name: "&dArr;", utf8: "\xE2\x87\x93" },
    HtmlEntityEntry{ name: "&dagger;", utf8: "\xE2\x80\xA0" },
    HtmlEntityEntry{ name: "&darr;", utf8: "\xE2\x86\x93" },
    HtmlEntityEntry{ name: "&deg;", utf8: "\xC2\xB0" },
    HtmlEntityEntry{ name: "&delta;", utf8: "\xCE\xB4" },
    HtmlEntityEntry{ name: "&diams;", utf8: "\xE2\x99\xA6" },
    HtmlEntityEntry{ name: "&divide;", utf8: "\xC3\xB7" },
    HtmlEntityEntry{ name: "&eacute;", utf8: "\xC3\xA9" },
    HtmlEntityEntry{ name: "&ecirc;", utf8: "\xC3\xAA" },
    HtmlEntityEntry{ name: "&egrave;", utf8: "\xC3\xA8" },
    HtmlEntityEntry{ name: "&empty;", utf8: "\xE2\x88\x85" },
    HtmlEntityEntry{ name: "&emsp;", utf8: "\xE2\x80\x83" },
    HtmlEntityEntry{ name: "&ensp;", utf8: "\xE2\x80\x82" },
    HtmlEntityEntry{ name: "&epsilon;", utf8: "\xCE\xB5" },
    HtmlEntityEntry{ name: "&equiv;", utf8: "\xE2\x89\xA1" },
    HtmlEntityEntry{ name: "&eta;", utf8: "\xCE\xB7" },
    HtmlEntityEntry{ name: "&euml;", utf8: "\xC3\xAB" },
    HtmlEntityEntry{ name: "&euro;", utf8: "\xE2\x82\xAC" },
    HtmlEntityEntry{ name: "&exist;", utf8: "\xE2\x88\x83" },
    HtmlEntityEntry{ name: "&forall;", utf8: "\xE2\x88\x80" },
    HtmlEntityEntry{ name: "&frac12;", utf8: "\xC2\xBD" },
    HtmlEntityEntry{ name: "&frac14;", utf8: "\xC2\xBC" },
    HtmlEntityEntry{ name: "&frac34;", utf8: "\xC2\xBE" },
    HtmlEntityEntry{ name: "&frasl;", utf8: "\xE2\x81\x84" },
    HtmlEntityEntry{ name: "&gamma;", utf8: "\xCE\xB3" },
    HtmlEntityEntry{ name: "&ge;", utf8: "\xE2\x89\xA5" },
    HtmlEntityEntry{ name: "&gt;", utf8: ">" },
    HtmlEntityEntry{ name: "&hArr;", utf8: "\xE2\x87\x94" },
    HtmlEntityEntry{ name: "&harr;", utf8: "\xE2\x86\x94" },
    HtmlEntityEntry{ name: "&hearts;", utf8: "\xE2\x99\xA5" },
    HtmlEntityEntry{ name: "&hellip;", utf8: "\xE2\x80\xA6" },
    HtmlEntityEntry{ name: "&iacute;", utf8: "\xC3\xAD" },
    HtmlEntityEntry{ name: "&iexcl;", utf8: "\xC2\xA1" },
    HtmlEntityEntry{ name: "&infin;", utf8: "\xE2\x88\x9E" },
    HtmlEntityEntry{ name: "&int;", utf8: "\xE2\x88\xAB" },
    HtmlEntityEntry{ name: "&iota;", utf8: "\xCE\xB9" },
    HtmlEntityEntry{ name: "&iquest;", utf8: "\xC2\xBF" },
    HtmlEntityEntry{ name: "&isin;", utf8: "\xE2\x88\x88" },
    HtmlEntityEntry{ name: "&kappa;", utf8: "\xCE\xBA" },
    HtmlEntityEntry{ name: "&lArr;", utf8: "\xE2\x87\x90" },
    HtmlEntityEntry{ name: "&lambda;", utf8: "\xCE\xBB" },
    HtmlEntityEntry{ name: "&laquo;", utf8: "\xC2\xAB" },
    HtmlEntityEntry{ name: "&larr;", utf8: "\xE2\x86\x90" },
    HtmlEntityEntry{ name: "&ldquo;", utf8: "\xE2\x80\x9C" },
    HtmlEntityEntry{ name: "&le;", utf8: "\xE2\x89\xA4" },
    HtmlEntityEntry{ name: "&lowast;", utf8: "\xE2\x88\x97" },
    HtmlEntityEntry{ name: "&loz;", utf8: "\xE2\x97\x8A" },
    HtmlEntityEntry{ name: "&lrm;", utf8: "\xE2\x80\x8E" },
    HtmlEntityEntry{ name: "&lsaquo;", utf8: "\xE2\x80\xB9" },
    HtmlEntityEntry{ name: "&lsquo;", utf8: "\xE2\x80\x98" },
    HtmlEntityEntry{ name: "&lt;", utf8: "<" },
    HtmlEntityEntry{ name: "&mdash;", utf8: "\xE2\x80\x94" },
    HtmlEntityEntry{ name: "&micro;", utf8: "\xC2\xB5" },
    HtmlEntityEntry{ name: "&middot;", utf8: "\xC2\xB7" },
    HtmlEntityEntry{ name: "&minus;", utf8: "\xE2\x88\x92" },
    HtmlEntityEntry{ name: "&mu;", utf8: "\xCE\xBC" },
    HtmlEntityEntry{ name: "&nabla;", utf8: "\xE2\x88\x87" },
    HtmlEntityEntry{ name: "&nbsp;", utf8: "\xC2\xA0" },
    HtmlEntityEntry{ name: "&ndash;", utf8: "\xE2\x80\x93" },
    HtmlEntityEntry{ name: "&ne;", utf8: "\xE2\x89\xA0" },
    HtmlEntityEntry{ name: "&ni;", utf8: "\xE2\x88\x8B" },
    HtmlEntityEntry{ name: "&not;", utf8: "\xC2\xAC" },
    HtmlEntityEntry{ name: "&notin;", utf8: "\xE2\x88\x89" },
    HtmlEntityEntry{ name: "&nsub;", utf8: "\xE2\x8A\x84" },
    HtmlEntityEntry{ name: "&ntilde;", utf8: "\xC3\xB1" },
    HtmlEntityEntry{ name: "&nu;", utf8: "\xCE\xBD" },
    HtmlEntityEntry{ name: "&oacute;", utf8: "\xC3\xB3" },
    HtmlEntityEntry{ name: "&oline;", utf8: "\xE2\x80\xBE" },
    HtmlEntityEntry{ name: "&omega;", utf8: "\xCF\x89" },
    HtmlEntityEntry{ name: "&omicron;", utf8: "\xCE\xBF" },
    HtmlEntityEntry{ name: "&oplus;", utf8: "\xE2\x8A\x95" },
    HtmlEntityEntry{ name: "&or;", utf8: "\xE2\x88\xA8" },
    HtmlEntityEntry{ name: "&ordf;", utf8: "\xC2\xAA" },
    HtmlEntityEntry{ name: "&ordm;", utf8: "\xC2\xBA" },
    HtmlEntityEntry{ name: "&otimes;", utf8: "\xE2\x8A\x97" },
    HtmlEntityEntry{ name: "&ouml;", utf8: "\xC3\xB6" },
    HtmlEntityEntry{ name: "&para;", utf8: "\xC2\xB6" },
    HtmlEntityEntry{ name: "&permil;", utf8: "\xE2\x80\xB0" },
    HtmlEntityEntry{ name: "&perp;", utf8: "\xE2\x8A\xA5" },
    HtmlEntityEntry{ name: "&phi;", utf8: "\xCF\x86" },
    HtmlEntityEntry{ name: "&pi;", utf8: "\xCF\x80" },
    HtmlEntityEntry{ name: "&plusmn;", utf8: "\xC2\xB1" },
    HtmlEntityEntry{ name: "&pound;", utf8: "\xC2\xA3" },
    HtmlEntityEntry{ name: "&prime;", utf8: "\xE2\x80\xB2" },
    HtmlEntityEntry{ name: "&prod;", utf8: "\xE2\x88\x8F" },
    HtmlEntityEntry{ name: "&prop;", utf8: "\xE2\x88\x9D" },
    HtmlEntityEntry{ name: "&psi;", utf8: "\xCF\x88" },
    HtmlEntityEntry{ name: "&quot;", utf8: "\"" },
    HtmlEntityEntry{ name: "&rArr;", utf8: "\xE2\x87\x92" },
    HtmlEntityEntry{ name: "&radic;", utf8: "\xE2\x88\x9A" },
    HtmlEntityEntry{ name: "&raquo;", utf8: "\xC2\xBB" },
    HtmlEntityEntry{ name: "&rarr;", utf8: "\xE2\x86\x92" },
    HtmlEntityEntry{ name: "&rdquo;", utf8: "\xE2\x80\x9D" },
    HtmlEntityEntry{ name: "&reg;", utf8: "\xC2\xAE" },
    HtmlEntityEntry{ name: "&rho;", utf8: "\xCF\x81" },
    HtmlEntityEntry{ name: "&rlm;", utf8: "\xE2\x80\x8F" },
    HtmlEntityEntry{ name: "&rsaquo;", utf8: "\xE2\x80\xBA" },
    HtmlEntityEntry{ name: "&rsquo;", utf8: "\xE2\x80\x99" },
    HtmlEntityEntry{ name: "&sbquo;", utf8: "\xE2\x80\x9A" },
    HtmlEntityEntry{ name: "&sdot;", utf8: "\xE2\x8B\x85" },
    HtmlEntityEntry{ name: "&sect;", utf8: "\xC2\xA7" },
    HtmlEntityEntry{ name: "&shy;", utf8: "\xC2\xAD" },
    HtmlEntityEntry{ name: "&sigma;", utf8: "\xCF\x83" },
    HtmlEntityEntry{ name: "&sigmaf;", utf8: "\xCF\x82" },
    HtmlEntityEntry{ name: "&sim;", utf8: "\xE2\x88\xBC" },
    HtmlEntityEntry{ name: "&spades;", utf8: "\xE2\x99\xA0" },
    HtmlEntityEntry{ name: "&sub;", utf8: "\xE2\x8A\x82" },
    HtmlEntityEntry{ name: "&sube;", utf8: "\xE2\x8A\x86" },
    HtmlEntityEntry{ name: "&sum;", utf8: "\xE2\x88\x91" },
    HtmlEntityEntry{ name: "&sup1;", utf8: "\xC2\xB9" },
    HtmlEntityEntry{ name: "&sup2;", utf8: "\xC2\xB2" },
    HtmlEntityEntry{ name: "&sup3;", utf8: "\xC2\xB3" },
    HtmlEntityEntry{ name: "&sup;", utf8: "\xE2\x8A\x83" },
    HtmlEntityEntry{ name: "&supe;", utf8: "\xE2\x8A\x87" },
    HtmlEntityEntry{ name: "&szlig;", utf8: "\xC3\x9F" },
    HtmlEntityEntry{ name: "&tau;", utf8: "\xCF\x84" },
    HtmlEntityEntry{ name: "&there4;", utf8: "\xE2\x88\xB4" },
    HtmlEntityEntry{ name: "&theta;", utf8: "\xCE\xB8" },
    HtmlEntityEntry{ name: "&thinsp;", utf8: "\xE2\x80\x89" },
    HtmlEntityEntry{ name: "&times;", utf8: "\xC3\x97" },
    HtmlEntityEntry{ name: "&trade;", utf8: "\xE2\x84\xA2" },
    HtmlEntityEntry{ name: "&uArr;", utf8: "\xE2\x87\x91" },
    HtmlEntityEntry{ name: "&uacute;", utf8: "\xC3\xBA" },
    HtmlEntityEntry{ name: "&uarr;", utf8: "\xE2\x86\x91" },
    HtmlEntityEntry{ name: "&upsilon;", utf8: "\xCF\x85" },
    HtmlEntityEntry{ name: "&uuml;", utf8: "\xC3\xBC" },
    HtmlEntityEntry{ name: "&xi;", utf8: "\xCE\xBE" },
    HtmlEntityEntry{ name: "&yen;", utf8: "\xC2\xA5" },
    HtmlEntityEntry{ name: "&zeta;", utf8: "\xCE\xB6" },
    HtmlEntityEntry{ name: "&zwj;", utf8: "\xE2\x80\x8D" },
    HtmlEntityEntry{ name: "&zwnj;", utf8: "\xE2\x80\x8C" },
];

pub struct HtmlEntities {
    static fn compareAscii(a: []const u8, b: []const u8): i32 {
        const min_len = if (a.len < b.len) a.len else b.len;
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if (a[i] < b[i]) return -1;
            if (a[i] > b[i]) return 1;
        }
        if (a.len < b.len) return -1;
        if (a.len > b.len) return 1;
        return 0;
    }

    /// Looks up a named HTML5 entity (including `&` and `;`) via binary search.
    pub static fn lookupNamedEntity(entity: []const u8): ([]const u8)? {
        if (entity.len < 3 || entity[0] != '&' || entity[entity.len - 1] != ';') {
            return null;
        }
        const two: usize = 2;
        let lo: usize = 0;
        let hi: usize = HTML5_NAMED_ENTITIES.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / two;
            const cmp = HtmlEntities.compareAscii(HTML5_NAMED_ENTITIES[mid].name, entity);
            if (cmp < 0) {
                lo = mid + 1;
            } else if (cmp > 0) {
                hi = mid;
            } else {
                return HTML5_NAMED_ENTITIES[mid].utf8;
            }
        }
        return null;
    }

    /// Applies WHATWG HTML5 numeric character reference rules (C1 Windows-1252 replacements and U+FFFD fallback).
    pub static fn mapNumericCodepoint(cp: u64): u32 {
        if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return 0xFFFD;
        }
        return switch (cp) {
            0x80 => 0x20AC,
            0x82 => 0x201A,
            0x83 => 0x0192,
            0x84 => 0x201E,
            0x85 => 0x2026,
            0x86 => 0x2020,
            0x87 => 0x2021,
            0x88 => 0x02C6,
            0x89 => 0x2030,
            0x8A => 0x0160,
            0x8B => 0x2039,
            0x8C => 0x0152,
            0x8E => 0x017D,
            0x91 => 0x2018,
    // 0x92..0x9F Windows-1252 mappings
            0x92 => 0x2019,
            0x93 => 0x201C,
            0x94 => 0x201D,
            0x95 => 0x2022,
            0x96 => 0x2013,
            0x97 => 0x2014,
            0x98 => 0x02DC,
            0x99 => 0x2122,
            0x9A => 0x0161,
            0x9B => 0x203A,
            0x9C => 0x0153,
            0x9E => 0x017E,
            0x9F => 0x0178,
            else => cp as u32,
        };
    }

    static fn parseNumericBody(body: []const u8): u32? {
        if (body.len == 0) return null;
        let cp: u64 = 0;
        let overflowed = false;
        if (body[0] == 'x' || body[0] == 'X') {
            if (body.len == 1) return null;
            let i: usize = 1;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                let d: u64 = 0;
                if (c >= '0' && c <= '9') {
                    d = (c - '0') as u64;
                } else if (c >= 'a' && c <= 'f') {
                    d = (c - 'a' + 10) as u64;
                } else if (c >= 'A' && c <= 'F') {
                    d = (c - 'A' + 10) as u64;
                } else {
                    return null;
                }
                if (!overflowed) {
                    cp = (cp << 4) | d;
                    if (cp > 0x10FFFF) {
                        overflowed = true;
                        cp = 0x110000;
                    }
                }
            }
        } else {
            let i: usize = 0;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                if (c < '0' || c > '9') return null;
                if (!overflowed) {
                    cp = cp * 10 + ((c - '0') as u64);
                    if (cp > 0x10FFFF) {
                        overflowed = true;
                        cp = 0x110000;
                    }
                }
            }
        }
        return HtmlEntities.mapNumericCodepoint(cp);
    }

    /// Escapes `&`, `<`, and `>` for HTML text nodes. Caller owns returned slice.
    pub static fn encodeText(alloc: std.mem.Allocator, text: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        try out.ensureTotalCapacity(alloc, text.len);
        for (c of text) {
            switch (c) {
                '&' => out.appendSlice(alloc, "&amp;")?,
                '<' => out.appendSlice(alloc, "&lt;")?,
                '>' => out.appendSlice(alloc, "&gt;")?,
                else => out.append(alloc, c)?,
            }
        }
        return out.toOwnedSlice(alloc);
    }

    /// Escapes `&`, `<`, `>`, `"`, and `'` for HTML attribute values. Caller owns returned slice.
    pub static fn encodeAttr(alloc: std.mem.Allocator, text: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        try out.ensureTotalCapacity(alloc, text.len);
        for (c of text) {
            switch (c) {
                '&' => out.appendSlice(alloc, "&amp;")?,
                '<' => out.appendSlice(alloc, "&lt;")?,
                '>' => out.appendSlice(alloc, "&gt;")?,
                '"' => out.appendSlice(alloc, "&quot;")?,
                '\'' => out.appendSlice(alloc, "&#39;")?,
                else => out.append(alloc, c)?,
            }
        }
        return out.toOwnedSlice(alloc);
    }

    /// Decodes HTML5 named entities and decimal/hex numeric character references (with WHATWG C1/FFFD rules).
    pub static fn decode(alloc: std.mem.Allocator, html: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        try out.ensureTotalCapacity(alloc, html.len);

        let i: usize = 0;
        while (i < html.len) {
            if (html[i] == '&') {
                let semi: usize = i + 1;
                let found_semi = false;
                while (semi < html.len && semi - i <= 32) : (semi += 1) {
                    const ch = html[semi];
                    if (ch == ';') {
                        found_semi = true;
                        break;
                    }
                    if (ch == '&' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '<') {
                        break;
                    }
                }
                if (found_semi) {
                    const ent = html[i .. semi + 1];
                    if (ent.len >= 4 && ent[1] == '#') {
                        if (cp := HtmlEntities.parseNumericBody(ent[2 .. ent.len - 1])) {
                            let utf8_tmp: [4]u8 = undefined;
                            const n = Utf8.encodeCodepoint(cp, &utf8_tmp);
                            if (n > 0) {
                                out.appendSlice(alloc, utf8_tmp[0..n])?;
                                i = semi + 1;
                                continue;
                            }
                        }
                    } else if (decoded := HtmlEntities.lookupNamedEntity(ent)) {
                        out.appendSlice(alloc, decoded)?;
                        i = semi + 1;
                        continue;
                    }
                }
            }
            out.append(alloc, html[i])?;
            i += 1;
        }
        return out.toOwnedSlice(alloc);
    }
}
