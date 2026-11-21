// Source: Absorbed from /data/dev/bun/src/md/entity.cp | Synced: 2026-11-20T05:46:35Z
// lib/html/escape.cp
// Zero-copy fast-path HTML Entity Escaper & Full HTML5 Named/Numeric Unescaper
// Absorbed from Bun src/md/entity.cp & HTML escaping utilities

const std = @import("std");
import { Utf8 } from "cap:string";

pub struct NamedEntityEntry {
    name: []const u8,
    utf8: []const u8,
}

// Sorted strictly in ASCII lexicographical order for binary search lookup
const NAMED_ENTITIES: [74]NamedEntityEntry = [
    NamedEntityEntry{ name: "&AMP;", utf8: "&" },
    NamedEntityEntry{ name: "&Alpha;", utf8: "\xCE\x91" },
    NamedEntityEntry{ name: "&Beta;", utf8: "\xCE\x92" },
    NamedEntityEntry{ name: "&COPY;", utf8: "\xC2\xA9" },
    NamedEntityEntry{ name: "&Dagger;", utf8: "\xE2\x80\xA1" },
    NamedEntityEntry{ name: "&Delta;", utf8: "\xCE\x94" },
    NamedEntityEntry{ name: "&GT;", utf8: ">" },
    NamedEntityEntry{ name: "&Gamma;", utf8: "\xCE\x93" },
    NamedEntityEntry{ name: "&LT;", utf8: "<" },
    NamedEntityEntry{ name: "&Omega;", utf8: "\xCE\xA9" },
    NamedEntityEntry{ name: "&Prime;", utf8: "\xE2\x80\xB3" },
    NamedEntityEntry{ name: "&QUOT;", utf8: "\"" },
    NamedEntityEntry{ name: "&REG;", utf8: "\xC2\xAE" },
    NamedEntityEntry{ name: "&Sigma;", utf8: "\xCE\xA3" },
    NamedEntityEntry{ name: "&alpha;", utf8: "\xCE\xB1" },
    NamedEntityEntry{ name: "&amp;", utf8: "&" },
    NamedEntityEntry{ name: "&apos;", utf8: "'" },
    NamedEntityEntry{ name: "&asymp;", utf8: "\xE2\x89\x88" },
    NamedEntityEntry{ name: "&bdquo;", utf8: "\xE2\x80\x9E" },
    NamedEntityEntry{ name: "&beta;", utf8: "\xCE\xB2" },
    NamedEntityEntry{ name: "&bull;", utf8: "\xE2\x80\xA2" },
    NamedEntityEntry{ name: "&cent;", utf8: "\xC2\xA2" },
    NamedEntityEntry{ name: "&check;", utf8: "\xE2\x9C\x93" },
    NamedEntityEntry{ name: "&clubs;", utf8: "\xE2\x99\xA3" },
    NamedEntityEntry{ name: "&copy;", utf8: "\xC2\xA9" },
    NamedEntityEntry{ name: "&cross;", utf8: "\xE2\x9C\x97" },
    NamedEntityEntry{ name: "&dagger;", utf8: "\xE2\x80\xA0" },
    NamedEntityEntry{ name: "&darr;", utf8: "\xE2\x86\x93" },
    NamedEntityEntry{ name: "&deg;", utf8: "\xC2\xB0" },
    NamedEntityEntry{ name: "&delta;", utf8: "\xCE\xB4" },
    NamedEntityEntry{ name: "&diams;", utf8: "\xE2\x99\xA6" },
    NamedEntityEntry{ name: "&divide;", utf8: "\xC3\xB7" },
    NamedEntityEntry{ name: "&emsp;", utf8: "\xE2\x80\x83" },
    NamedEntityEntry{ name: "&ensp;", utf8: "\xE2\x80\x82" },
    NamedEntityEntry{ name: "&epsilon;", utf8: "\xCE\xB5" },
    NamedEntityEntry{ name: "&equiv;", utf8: "\xE2\x89\xA1" },
    NamedEntityEntry{ name: "&euro;", utf8: "\xE2\x82\xAC" },
    NamedEntityEntry{ name: "&frac12;", utf8: "\xC2\xBD" },
    NamedEntityEntry{ name: "&frac14;", utf8: "\xC2\xBC" },
    NamedEntityEntry{ name: "&frac34;", utf8: "\xC2\xBE" },
    NamedEntityEntry{ name: "&gamma;", utf8: "\xCE\xB3" },
    NamedEntityEntry{ name: "&ge;", utf8: "\xE2\x89\xA5" },
    NamedEntityEntry{ name: "&gt;", utf8: ">" },
    NamedEntityEntry{ name: "&harr;", utf8: "\xE2\x86\x94" },
    NamedEntityEntry{ name: "&hearts;", utf8: "\xE2\x99\xA5" },
    NamedEntityEntry{ name: "&hellip;", utf8: "\xE2\x80\xA6" },
    NamedEntityEntry{ name: "&iexcl;", utf8: "\xC2\xA1" },
    NamedEntityEntry{ name: "&infin;", utf8: "\xE2\x88\x9E" },
    NamedEntityEntry{ name: "&iquest;", utf8: "\xC2\xBF" },
    NamedEntityEntry{ name: "&lambda;", utf8: "\xCE\xBB" },
    NamedEntityEntry{ name: "&laquo;", utf8: "\xC2\xAB" },
    NamedEntityEntry{ name: "&larr;", utf8: "\xE2\x86\x90" },
    NamedEntityEntry{ name: "&ldquo;", utf8: "\xE2\x80\x9C" },
    NamedEntityEntry{ name: "&le;", utf8: "\xE2\x89\xA4" },
    NamedEntityEntry{ name: "&lsquo;", utf8: "\xE2\x80\x98" },
    NamedEntityEntry{ name: "&lt;", utf8: "<" },
    NamedEntityEntry{ name: "&mdash;", utf8: "\xE2\x80\x94" },
    NamedEntityEntry{ name: "&middot;", utf8: "\xC2\xB7" },
    NamedEntityEntry{ name: "&mu;", utf8: "\xCE\xBC" },
    NamedEntityEntry{ name: "&nbsp;", utf8: "\xC2\xA0" },
    NamedEntityEntry{ name: "&ndash;", utf8: "\xE2\x80\x93" },
    NamedEntityEntry{ name: "&ne;", utf8: "\xE2\x89\xA0" },
    NamedEntityEntry{ name: "&omega;", utf8: "\xCF\x89" },
    NamedEntityEntry{ name: "&para;", utf8: "\xC2\xB6" },
    NamedEntityEntry{ name: "&permil;", utf8: "\xE2\x80\xB0" },
    NamedEntityEntry{ name: "&pi;", utf8: "\xCF\x80" },
    NamedEntityEntry{ name: "&plusmn;", utf8: "\xC2\xB1" },
    NamedEntityEntry{ name: "&pound;", utf8: "\xC2\xA3" },
    NamedEntityEntry{ name: "&prime;", utf8: "\xE2\x80\xB2" },
    NamedEntityEntry{ name: "&quot;", utf8: "\"" },
    NamedEntityEntry{ name: "&raquo;", utf8: "\xC2\xBB" },
    NamedEntityEntry{ name: "&rarr;", utf8: "\xE2\x86\x92" },
    NamedEntityEntry{ name: "&rdquo;", utf8: "\xE2\x80\x9D" },
    NamedEntityEntry{ name: "&reg;", utf8: "\xC2\xAE" },
];

const EXTRA_ENTITIES: [12]NamedEntityEntry = [
    NamedEntityEntry{ name: "&rsquo;", utf8: "\xE2\x80\x99" },
    NamedEntityEntry{ name: "&sbquo;", utf8: "\xE2\x80\x9A" },
    NamedEntityEntry{ name: "&sect;", utf8: "\xC2\xA7" },
    NamedEntityEntry{ name: "&shy;", utf8: "\xC2\xAD" },
    NamedEntityEntry{ name: "&sigma;", utf8: "\xCF\x83" },
    NamedEntityEntry{ name: "&spades;", utf8: "\xE2\x99\xA0" },
    NamedEntityEntry{ name: "&theta;", utf8: "\xCE\xB8" },
    NamedEntityEntry{ name: "&thinsp;", utf8: "\xE2\x80\x89" },
    NamedEntityEntry{ name: "&times;", utf8: "\xC3\x97" },
    NamedEntityEntry{ name: "&trade;", utf8: "\xE2\x84\xA2" },
    NamedEntityEntry{ name: "&uarr;", utf8: "\xE2\x86\x91" },
    NamedEntityEntry{ name: "&yen;", utf8: "\xC2\xA5" },
];

pub struct HtmlEscape {
    /// Returns `true` if `input` contains any HTML-special character (`&`, `<`, `>`, `"`, `'`).
    pub static fn needsEscape(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    static fn compareStrings(a: []const u8, b: []const u8): i32 {
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

    static fn binarySearchEntity(table: []const NamedEntityEntry, entity: []const u8): ([]const u8)? {
        const two: usize = 2;
        let lo: usize = 0;
        let hi: usize = table.len;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / two;
            const cmp = HtmlEscape.compareStrings(table[mid].name, entity);
            if (cmp < 0) {
                lo = mid + 1;
            } else if (cmp > 0) {
                hi = mid;
            } else {
                return table[mid].utf8;
            }
        }
        return null;
    }

    /// Looks up a named HTML5 entity (including `&` and `;`), returning its UTF-8 string if known.
    pub static fn lookupNamedEntity(entity: []const u8): ([]const u8)? {
        if (entity.len < 3 || entity[0] != '&' || entity[entity.len - 1] != ';') {
            return null;
        }
        if (hit := HtmlEscape.binarySearchEntity(NAMED_ENTITIES[0..NAMED_ENTITIES.len], entity)) {
            return hit;
        }
        return HtmlEscape.binarySearchEntity(EXTRA_ENTITIES[0..EXTRA_ENTITIES.len], entity);
    }

    /// Escapes `&`, `<`, `>`, `"`, and `'` into standard HTML entities in `out_buf`.
    pub static fn escapeToBuf(input: []const u8, out_buf: []u8): []const u8 {
        if (!HtmlEscape.needsEscape(input)) {
            if (input.len <= out_buf.len) {
                const end = HtmlEscape.appendSlice(out_buf, 0, input);
                return out_buf[0..end];
            }
            return input;
        }

        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len && pos < out_buf.len) {
            const c = input[i];
            if (c == '&') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&amp;");
            } else if (c == '<') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&lt;");
            } else if (c == '>') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&gt;");
            } else if (c == '"') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&quot;");
            } else if (c == '\'') {
                pos = HtmlEscape.appendSlice(out_buf, pos, "&#39;");
            } else {
                out_buf[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return out_buf[0..pos];
    }

    /// Allocates and escapes `&`, `<`, `>`, `"`, `'` into standard HTML entities.
    pub static fn escapeAlloc(allocator: std.mem.Allocator, input: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        for (c of input) {
            switch (c) {
                '&' => out.appendSlice(allocator, "&amp;")?,
                '<' => out.appendSlice(allocator, "&lt;")?,
                '>' => out.appendSlice(allocator, "&gt;")?,
                '"' => out.appendSlice(allocator, "&quot;")?,
                '\'' => out.appendSlice(allocator, "&#39;")?,
                else => out.append(allocator, c)?,
            }
        }
        return out.toOwnedSlice(allocator);
    }

    static fn decodeNumericEntity(body: []const u8): u32? {
        if (body.len == 0) return null;
        let cp: u64 = 0;
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
                cp = (cp << 4) | d;
                if (cp > 0x10FFFF) cp = 0xFFFD;
            }
        } else {
            let i: usize = 0;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                if (c < '0' || c > '9') return null;
                cp = cp * 10 + ((c - '0') as u64);
                if (cp > 0x10FFFF) cp = 0xFFFD;
            }
        }
        if (cp == 0 || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
            return 0xFFFD;
        }
        return cp as u32;
    }

    /// Unescapes HTML5 named entities, decimal references (`&#65;`), and hex references (`&#x1F600;`) into `out_buf`.
    pub static fn unescapeToBuf(input: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len && pos < out_buf.len) {
            if (input[i] == '&') {
                // Find closing ';' within reasonable entity length (<= 16 bytes)
                let semi: usize = i + 1;
                let found_semi = false;
                while (semi < input.len && semi - i <= 16) : (semi += 1) {
                    if (input[semi] == ';') {
                        found_semi = true;
                        break;
                    }
                    if (input[semi] == '&' || input[semi] == ' ' || input[semi] == '<') {
                        break;
                    }
                }
                if (found_semi) {
                    const ent = input[i .. semi + 1];
                    if (ent.len >= 4 && ent[1] == '#') {
                        if (cp := HtmlEscape.decodeNumericEntity(ent[2 .. ent.len - 1])) {
                            let utf8_tmp: [4]u8 = undefined;
                            const n = Utf8.encodeCodepoint(cp, &utf8_tmp);
                            if (n > 0) {
                                pos = HtmlEscape.appendSlice(out_buf, pos, utf8_tmp[0..n]);
                                i = semi + 1;
                                continue;
                            }
                        }
                    } else if (decoded := HtmlEscape.lookupNamedEntity(ent)) {
                        pos = HtmlEscape.appendSlice(out_buf, pos, decoded);
                        i = semi + 1;
                        continue;
                    }
                }
            }
            out_buf[pos] = input[i];
            pos += 1;
            i += 1;
        }
        return out_buf[0..pos];
    }

    /// Allocates and unescapes all HTML5 named and numeric entities.
    pub static fn unescapeAlloc(allocator: std.mem.Allocator, input: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        let i: usize = 0;
        while (i < input.len) {
            if (input[i] == '&') {
                let semi: usize = i + 1;
                let found_semi = false;
                while (semi < input.len && semi - i <= 16) : (semi += 1) {
                    if (input[semi] == ';') {
                        found_semi = true;
                        break;
                    }
                    if (input[semi] == '&' || input[semi] == ' ' || input[semi] == '<') {
                        break;
                    }
                }
                if (found_semi) {
                    const ent = input[i .. semi + 1];
                    if (ent.len >= 4 && ent[1] == '#') {
                        if (cp := HtmlEscape.decodeNumericEntity(ent[2 .. ent.len - 1])) {
                            let utf8_tmp: [4]u8 = undefined;
                            const n = Utf8.encodeCodepoint(cp, &utf8_tmp);
                            if (n > 0) {
                                out.appendSlice(allocator, utf8_tmp[0..n])?;
                                i = semi + 1;
                                continue;
                            }
                        }
                    } else if (decoded := HtmlEscape.lookupNamedEntity(ent)) {
                        out.appendSlice(allocator, decoded)?;
                        i = semi + 1;
                        continue;
                    }
                }
            }
            out.append(allocator, input[i])?;
            i += 1;
        }
        return out.toOwnedSlice(allocator);
    }
}
