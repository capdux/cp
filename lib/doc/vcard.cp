// Reference-hardened against RFC 5545 (iCalendar) & RFC 6350 (vCard 4.0) (MIT OR Apache-2.0)
// Source: cp/lib/vcard/vcard.cp (Pure CP RFC 6350 / 2426 vCard 3.0 & 4.0 Parser & Generator)
// Zero external C dependencies; supports 16-lane SIMD scanning, line unfolding, escaping, group prefixes, TYPE params, N/ADR fields, and round-trip generation.

const std = @import("std");
import { simdCalFindLineDelim, simdCalHasControlByte } from "./ical.cp";

pub const VCARD_MAX_LINE_BYTES: usize = 65536;
pub const VCARD_MAX_PROPERTIES: usize = 16384;

pub err VCardError {
    InvalidFormat,
    MissingFn,
    UnsupportedVersion,
    UnbalancedCard,
    OutOfMemory,
}

pub enum VCardVersion {
    V3_0,
    V4_0,

    fn toStr(): string {
        return switch (_) {
            .V3_0 => "3.0",
            .V4_0 => "4.0",
        };
    }

    static fn fromStr(s: string): VCardVersion? {
        let trimmed = vcTrimSpaces(s);
        if (std.mem.eql(u8, trimmed, "3.0")) return .V3_0;
        if (std.mem.eql(u8, trimmed, "4.0")) return .V4_0;
        return null;
    }
}

pub struct StructuredName {
    family: string = "",
    given: string = "",
    additional: string = "",
    prefix: string = "",
    suffix: string = "",

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.family.len > 0) alloc.free(_.family);
        if (_.given.len > 0) alloc.free(_.given);
        if (_.additional.len > 0) alloc.free(_.additional);
        if (_.prefix.len > 0) alloc.free(_.prefix);
        if (_.suffix.len > 0) alloc.free(_.suffix);
        _.family = "";
        _.given = "";
        _.additional = "";
        _.prefix = "";
        _.suffix = "";
    }
}

pub struct TypedValue {
    types: []string = [],
    value: string = "",

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (t of _.types) {
            if (t.len > 0) alloc.free(t);
        }
        if (_.types.len > 0) alloc.free(_.types);
        if (_.value.len > 0) alloc.free(_.value);
        _.types = [];
        _.value = "";
    }
}

pub struct VCardAddress {
    types: []string = [],
    street: string = "",
    city: string = "",
    region: string = "",
    postal_code: string = "",
    country: string = "",

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (t of _.types) {
            if (t.len > 0) alloc.free(t);
        }
        if (_.types.len > 0) alloc.free(_.types);
        if (_.street.len > 0) alloc.free(_.street);
        if (_.city.len > 0) alloc.free(_.city);
        if (_.region.len > 0) alloc.free(_.region);
        if (_.postal_code.len > 0) alloc.free(_.postal_code);
        if (_.country.len > 0) alloc.free(_.country);
        _.types = [];
        _.street = "";
        _.city = "";
        _.region = "";
        _.postal_code = "";
        _.country = "";
    }
}

pub struct VCardContact {
    version: VCardVersion = .V4_0,
    uid: string = "",
    fn_name: string = "",
    name: StructuredName? = null,
    org: string = "",
    title: string = "",
    phones: []TypedValue = [],
    emails: []TypedValue = [],
    urls: []TypedValue = [],
    addresses: []VCardAddress = [],
    note: string = "",
    alloc: std.mem.Allocator? = null,

    static fn init(alloc: std.mem.Allocator): VCardContact {
        return VCardContact{
            alloc: alloc,
        };
    }

    fn deinit(*_) {
        let alloc = _.alloc ?? return;
        if (_.uid.len > 0) alloc.free(_.uid);
        if (_.fn_name.len > 0) alloc.free(_.fn_name);
        if (*n := _.name) {
            n.deinit(alloc);
        }
        if (_.org.len > 0) alloc.free(_.org);
        if (_.title.len > 0) alloc.free(_.title);
        if (_.note.len > 0) alloc.free(_.note);

        for (*p of _.phones) {
            p.deinit(alloc);
        }
        if (_.phones.len > 0) alloc.free(_.phones);

        for (*e of _.emails) {
            e.deinit(alloc);
        }
        if (_.emails.len > 0) alloc.free(_.emails);

        for (*u of _.urls) {
            u.deinit(alloc);
        }
        if (_.urls.len > 0) alloc.free(_.urls);

        for (*a of _.addresses) {
            a.deinit(alloc);
        }
        if (_.addresses.len > 0) alloc.free(_.addresses);

        _.uid = "";
        _.fn_name = "";
        _.name = null;
        _.org = "";
        _.title = "";
        _.note = "";
        _.phones = [];
        _.emails = [];
        _.urls = [];
        _.addresses = [];
        _.alloc = null;
    }
}

// ============================================================================
// Internal String & Escaping Helpers (16-lane SIMD + 256-entry LUT)
// ============================================================================

type VCardVec16 = @Vector(16, u8);

fn makeVcEscLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[usize('\\')] = 1;
    lut[usize(';')] = 1;
    lut[usize(',')] = 1;
    lut[usize('\n')] = 1;
    lut[usize('\r')] = 1;
    return lut;
}

const VC_ESC_LUT: [u8; 256] = makeVcEscLut();

fn vcIsSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn vcTrimSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && vcIsSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && vcIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn vcAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const lo_A: VCardVec16 = @splat(65 as u8);
    const hi_Z: VCardVec16 = @splat(90 as u8);
    const case_bit: VCardVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: VCardVec16 = a[i .. i + 16][0..16].*;
        const vb: VCardVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) : (i += 1) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

fn vcDupSlice(alloc: std.mem.Allocator, s: string): string ! VCardError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return VCardError.OutOfMemory;
}

fn vcHasCrLfOrControl(s: string): bool {
    if (simdCalHasControlByte(s)) return true;
    let i: usize = 0;
    const vcr: VCardVec16 = @splat(13 as u8);
    const vlf: VCardVec16 = @splat(10 as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: VCardVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == vcr) | (v == vlf))) return true;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n') return true;
    }
    return false;
}

/// Unfolds RFC 6350 / 2426 folded lines via 16-lane SIMD delimiter scanning: removes CRLF (or LF)
/// followed by a single SPACE or TAB. Rejects bare `\r` not followed by `\n`.
fn vcUnfoldLines(alloc: std.mem.Allocator, text: string): ([]u8) ! VCardError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < text.len) {
        const hit = simdCalFindLineDelim(text, i);
        if (hit > i) {
            out.appendSlice(alloc, text[i..hit]) catch return VCardError.OutOfMemory;
            i = hit;
            if (i >= text.len) break;
        }
        const ch = text[i];
        if (ch == '\r') {
            if (i + 1 >= text.len || text[i + 1] != '\n') {
                return VCardError.InvalidFormat;
            }
            if (i + 2 < text.len && (text[i + 2] == ' ' || text[i + 2] == '\t')) {
                i += 3;
                continue;
            }
            out.append(alloc, '\n') catch return VCardError.OutOfMemory;
            i += 2;
        } else if (ch == '\n') {
            if (i + 1 < text.len && (text[i + 1] == ' ' || text[i + 1] == '\t')) {
                i += 2;
                continue;
            }
            out.append(alloc, '\n') catch return VCardError.OutOfMemory;
            i += 1;
        } else {
            out.append(alloc, ch) catch return VCardError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
}

fn vcUnescapeText(alloc: std.mem.Allocator, text: string): string ! VCardError {
    if (text.len == 0) return "";
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    const vbs: VCardVec16 = @splat(92 as u8);
    while (i < text.len) {
        const run_start = i;
        while (i + 16 <= text.len) : (i += 16) {
            const v: VCardVec16 = text[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == vbs)) break;
        }
        while (i < text.len && text[i] != '\\') : (i += 1) {}
        if (i > run_start) {
            out.appendSlice(alloc, text[run_start..i]) catch return VCardError.OutOfMemory;
        }
        if (i >= text.len) break;
        if (i + 1 < text.len) {
            let next = text[i + 1];
            switch (next) {
                'n' | 'N' => {
                    out.append(alloc, '\n') catch return VCardError.OutOfMemory;
                    i += 2;
                },
                ',' => {
                    out.append(alloc, ',') catch return VCardError.OutOfMemory;
                    i += 2;
                },
                ';' => {
                    out.append(alloc, ';') catch return VCardError.OutOfMemory;
                    i += 2;
                },
                '\\' => {
                    out.append(alloc, '\\') catch return VCardError.OutOfMemory;
                    i += 2;
                },
                else => {
                    out.append(alloc, text[i]) catch return VCardError.OutOfMemory;
                    i += 1;
                },
            }
        } else {
            out.append(alloc, text[i]) catch return VCardError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
}

fn vcAppendEscapedText(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, text: string): void ! VCardError {
    let i: usize = 0;
    const vbs: VCardVec16 = @splat(92 as u8);
    const vsemi: VCardVec16 = @splat(59 as u8);
    const vcom: VCardVec16 = @splat(44 as u8);
    const vlf: VCardVec16 = @splat(10 as u8);
    const vcr: VCardVec16 = @splat(13 as u8);
    while (i < text.len) {
        const run_start = i;
        while (i + 16 <= text.len) : (i += 16) {
            const v: VCardVec16 = text[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == vbs) | (v == vsemi) | (v == vcom) | (v == vlf) | (v == vcr))) break;
        }
        while (i < text.len && VC_ESC_LUT[usize(text[i])] == 0) : (i += 1) {}
        if (i > run_start) {
            buf.appendSlice(alloc, text[run_start..i]) catch return VCardError.OutOfMemory;
        }
        if (i >= text.len) break;
        const c = text[i];
        i += 1;
        switch (c) {
            '\\' => buf.appendSlice(alloc, "\\\\") catch return VCardError.OutOfMemory,
            ';' => buf.appendSlice(alloc, "\\;") catch return VCardError.OutOfMemory,
            ',' => buf.appendSlice(alloc, "\\,") catch return VCardError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return VCardError.OutOfMemory,
            '\r' => {},
            else => buf.append(alloc, c) catch return VCardError.OutOfMemory,
        }
    }
}

/// Splits a structured value (like N or ADR) by unescaped `;` and unescapes each component.
fn splitStructuredComponents(alloc: std.mem.Allocator, raw: string, max_parts: usize): ([]string) ! VCardError {
    let parts: std.ArrayList(string) = .empty;
    errdefer {
        for (p of parts.items) {
            if (p.len > 0) alloc.free(p);
        }
        parts.deinit(alloc);
    }

    let start: usize = 0;
    let i: usize = 0;
    let escaped: bool = false;
    while (i < raw.len) : (i += 1) {
        let ch = raw[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == ';' && parts.items.len + 1 < max_parts) {
            let comp = vcUnescapeText(alloc, raw[start..i])?;
            parts.append(alloc, comp) catch {
                if (comp.len > 0) alloc.free(comp);
                return VCardError.OutOfMemory;
            };
            start = i + 1;
        }
    }

    let last = vcUnescapeText(alloc, raw[start..])?;
    parts.append(alloc, last) catch {
        if (last.len > 0) alloc.free(last);
        return VCardError.OutOfMemory;
    };

    return parts.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
}

fn freeStringSlice(alloc: std.mem.Allocator, items: []string) {
    for (s of items) {
        if (s.len > 0) alloc.free(s);
    }
    if (items.len > 0) alloc.free(items);
}

struct ContentLine {
    name: string,
    params: string,
    value: string,
}

/// Parses `[group "."] name *(";" param) ":" value`.
fn parseContentLine(line: string): ContentLine? {
    let in_quotes: bool = false;
    let colon_idx: usize? = null;
    let semi_idx: usize? = null;
    let i: usize = 0;
    while (i < line.len) : (i += 1) {
        let ch = line[i];
        if (ch == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes) {
            if (ch == ';' && semi_idx == null) {
                semi_idx = i;
            } else if (ch == ':') {
                colon_idx = i;
                break;
            }
        }
    }
    if (in_quotes) return null;
    let c_idx = colon_idx ?? return null;
    let name_end = semi_idx ?? c_idx;
    let raw_name = vcTrimSpaces(line[0..name_end]);
    if (raw_name.len == 0) return null;

    // Strip optional `group.` prefix (e.g. `item1.TEL` -> `TEL`)
    let prop_name = raw_name;
    let dot_idx: usize? = std.mem.indexOfScalar(u8, raw_name, '.');
    if (d := dot_idx) {
        prop_name = vcTrimSpaces(raw_name[d + 1..]);
        if (prop_name.len == 0) return null;
    }

    let params: string = "";
    if (s_idx := semi_idx) {
        if (s_idx + 1 <= c_idx) {
            params = line[s_idx + 1..c_idx];
        }
    }

    return ContentLine{
        name: prop_name,
        params: params,
        value: line[c_idx + 1..],
    };
}

/// Extracts all `TYPE=...` values from property parameters, splitting comma-separated values.
fn parseTypeParams(alloc: std.mem.Allocator, params: string): ([]string) ! VCardError {
    if (params.len == 0) return [];
    let types: std.ArrayList(string) = .empty;
    errdefer {
        for (t of types.items) {
            if (t.len > 0) alloc.free(t);
        }
        types.deinit(alloc);
    }

    // Split params by `;` outside quotes
    let start: usize = 0;
    let i: usize = 0;
    let in_quotes: bool = false;
    while (i <= params.len) : (i += 1) {
        let at_end = i == params.len;
        if (!at_end && params[i] == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (at_end || (!in_quotes && params[i] == ';')) {
            let segment = vcTrimSpaces(params[start..i]);
            start = i + 1;
            if (segment.len == 0) continue;

            if (eq_idx := std.mem.indexOfScalar(u8, segment, '=')) {
                let p_name = vcTrimSpaces(segment[0..eq_idx]);
                if (vcAsciiEqlIgnoreCase(p_name, "TYPE")) {
                    let p_val = vcTrimSpaces(segment[eq_idx + 1..]);
                    if (p_val.len >= 2 && p_val[0] == '"' && p_val[p_val.len - 1] == '"') {
                        p_val = p_val[1..p_val.len - 1];
                    }
                    let tok_it = std.mem.splitScalar(u8, p_val, ',');
                    while (tok := tok_it.next()) {
                        let t = vcTrimSpaces(tok);
                        if (t.len == 0) continue;
                        let dup = vcDupSlice(alloc, t)?;
                        types.append(alloc, dup) catch {
                            if (dup.len > 0) alloc.free(dup);
                            return VCardError.OutOfMemory;
                        };
                    }
                }
            }
        }
    }

    if (types.items.len == 0) {
        types.deinit(alloc);
        return [];
    }
    return types.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
}

/// Appends a line with optional RFC 6350 75-octet line folding (`\r\n ` continuation).
fn appendFoldedLine(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, line: string, fold_75: bool): void ! VCardError {
    if (!fold_75 || line.len <= 75) {
        buf.appendSlice(alloc, line) catch return VCardError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return VCardError.OutOfMemory;
        return;
    }

    let pos: usize = 0;
    let first: bool = true;
    while (pos < line.len) {
        let max_bytes: usize = if (first) 75 else 74;
        let remain = line.len - pos;
        if (remain <= max_bytes) {
            if (!first) {
                buf.append(alloc, ' ') catch return VCardError.OutOfMemory;
            }
            buf.appendSlice(alloc, line[pos..]) catch return VCardError.OutOfMemory;
            buf.appendSlice(alloc, "\r\n") catch return VCardError.OutOfMemory;
            break;
        }

        // Avoid splitting mid-UTF-8 codepoint
        let end = pos + max_bytes;
        while (end > pos && (line[end] & 0xC0) == 0x80) {
            end -= 1;
        }
        if (end == pos) {
            end = pos + max_bytes;
        }

        if (!first) {
            buf.append(alloc, ' ') catch return VCardError.OutOfMemory;
        }
        buf.appendSlice(alloc, line[pos..end]) catch return VCardError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return VCardError.OutOfMemory;
        pos = end;
        first = false;
    }
}

fn appendTypedProperty(
    buf: *std.ArrayList(u8),
    line_buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    prop_name: string,
    tv: TypedValue,
    fold_75: bool,
): void ! VCardError {
    line_buf.clearRetainingCapacity();
    line_buf.appendSlice(alloc, prop_name) catch return VCardError.OutOfMemory;
    if (tv.types.len > 0) {
        line_buf.appendSlice(alloc, ";TYPE=") catch return VCardError.OutOfMemory;
        let idx: usize = 0;
        for (t of tv.types) {
            if (vcHasCrLfOrControl(t)) return VCardError.InvalidFormat;
            if (idx > 0) line_buf.append(alloc, ',') catch return VCardError.OutOfMemory;
            line_buf.appendSlice(alloc, t) catch return VCardError.OutOfMemory;
            idx += 1;
        }
    }
    line_buf.append(alloc, ':') catch return VCardError.OutOfMemory;
    vcAppendEscapedText(line_buf, alloc, tv.value)?;
    appendFoldedLine(buf, alloc, line_buf.items, fold_75)?;
}

// ============================================================================
// VCardCodec: Batch Parse & RFC 6350 Generation
// ============================================================================

pub static class VCardCodec {
    static fn parseAll(alloc: std.mem.Allocator, vcf_text: string): ([]VCardContact) ! VCardError {
        let src = vcf_text;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            src = src[3..];
        }
        if (simdCalHasControlByte(src)) {
            return VCardError.InvalidFormat;
        }
        let unfolded = vcUnfoldLines(alloc, src)?;
        defer alloc.free(unfolded);

        let contacts: std.ArrayList(VCardContact) = .empty;
        errdefer {
            for (*c of contacts.items) {
                c.deinit();
            }
            contacts.deinit(alloc);
        }

        let in_card: bool = false;
        let saw_fn: bool = false;
        let current = VCardContact.init(alloc);
        let phones_list: std.ArrayList(TypedValue) = .empty;
        let emails_list: std.ArrayList(TypedValue) = .empty;
        let urls_list: std.ArrayList(TypedValue) = .empty;
        let addrs_list: std.ArrayList(VCardAddress) = .empty;

        errdefer {
            current.deinit();
            for (*p of phones_list.items) p.deinit(alloc);
            phones_list.deinit(alloc);
            for (*e of emails_list.items) e.deinit(alloc);
            emails_list.deinit(alloc);
            for (*u of urls_list.items) u.deinit(alloc);
            urls_list.deinit(alloc);
            for (*a of addrs_list.items) a.deinit(alloc);
            addrs_list.deinit(alloc);
        }

        let prop_count: usize = 0;
        let line_it = std.mem.splitScalar(u8, unfolded, '\n');
        while (raw_line := line_it.next()) {
            if (raw_line.len > VCARD_MAX_LINE_BYTES) {
                return VCardError.InvalidFormat;
            }
            let line = vcTrimSpaces(raw_line);
            if (line.len == 0) continue;

            prop_count += 1;
            if (prop_count > VCARD_MAX_PROPERTIES) {
                return VCardError.InvalidFormat;
            }

            let parts = parseContentLine(line) ?? return VCardError.InvalidFormat;

            if (vcAsciiEqlIgnoreCase(parts.name, "BEGIN")) {
                if (!vcAsciiEqlIgnoreCase(vcTrimSpaces(parts.value), "VCARD")) {
                    return VCardError.InvalidFormat;
                }
                if (in_card) {
                    return VCardError.UnbalancedCard;
                }
                in_card = true;
                saw_fn = false;
                current = VCardContact.init(alloc);
                continue;
            }

            if (vcAsciiEqlIgnoreCase(parts.name, "END")) {
                if (!vcAsciiEqlIgnoreCase(vcTrimSpaces(parts.value), "VCARD")) {
                    return VCardError.InvalidFormat;
                }
                if (!in_card) {
                    return VCardError.UnbalancedCard;
                }
                if (!saw_fn || current.fn_name.len == 0) {
                    return VCardError.MissingFn;
                }

                current.phones = phones_list.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
                current.emails = emails_list.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
                current.urls = urls_list.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
                current.addresses = addrs_list.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;

                contacts.append(alloc, current) catch return VCardError.OutOfMemory;
                current = VCardContact.init(alloc);
                in_card = false;
                continue;
            }

            if (!in_card) {
                return VCardError.InvalidFormat;
            }

            if (vcAsciiEqlIgnoreCase(parts.name, "VERSION")) {
                let ver = VCardVersion.fromStr(parts.value) ?? return VCardError.UnsupportedVersion;
                current.version = ver;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "UID")) {
                if (current.uid.len > 0) alloc.free(current.uid);
                current.uid = vcUnescapeText(alloc, parts.value)?;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "FN")) {
                if (current.fn_name.len > 0) alloc.free(current.fn_name);
                current.fn_name = vcUnescapeText(alloc, parts.value)?;
                saw_fn = true;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "ORG")) {
                if (current.org.len > 0) alloc.free(current.org);
                current.org = vcUnescapeText(alloc, parts.value)?;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "TITLE")) {
                if (current.title.len > 0) alloc.free(current.title);
                current.title = vcUnescapeText(alloc, parts.value)?;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "NOTE")) {
                if (current.note.len > 0) alloc.free(current.note);
                current.note = vcUnescapeText(alloc, parts.value)?;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "N")) {
                let comps = splitStructuredComponents(alloc, parts.value, 5)?;
                if (*prev_n := current.name) {
                    prev_n.deinit(alloc);
                }
                let sn = StructuredName{};
                if (comps.len > 0) sn.family = comps[0];
                if (comps.len > 1) sn.given = comps[1];
                if (comps.len > 2) sn.additional = comps[2];
                if (comps.len > 3) sn.prefix = comps[3];
                if (comps.len > 4) sn.suffix = comps[4];
                if (comps.len > 0) alloc.free(comps);
                current.name = sn;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "TEL")) {
                let types = parseTypeParams(alloc, parts.params)?;
                errdefer freeStringSlice(alloc, types);
                let val = vcUnescapeText(alloc, parts.value)?;
                errdefer if (val.len > 0) alloc.free(val);
                phones_list.append(alloc, TypedValue{ types: types, value: val }) catch return VCardError.OutOfMemory;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "EMAIL")) {
                let types = parseTypeParams(alloc, parts.params)?;
                errdefer freeStringSlice(alloc, types);
                let val = vcUnescapeText(alloc, parts.value)?;
                errdefer if (val.len > 0) alloc.free(val);
                emails_list.append(alloc, TypedValue{ types: types, value: val }) catch return VCardError.OutOfMemory;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "URL")) {
                let types = parseTypeParams(alloc, parts.params)?;
                errdefer freeStringSlice(alloc, types);
                let val = vcUnescapeText(alloc, parts.value)?;
                errdefer if (val.len > 0) alloc.free(val);
                urls_list.append(alloc, TypedValue{ types: types, value: val }) catch return VCardError.OutOfMemory;
            } else if (vcAsciiEqlIgnoreCase(parts.name, "ADR")) {
                let types = parseTypeParams(alloc, parts.params)?;
                errdefer freeStringSlice(alloc, types);
                let comps = splitStructuredComponents(alloc, parts.value, 7)?;
                // PO Box [0] and Extended [1] are not stored in VCardAddress, free them
                if (comps.len > 0 && comps[0].len > 0) alloc.free(comps[0]);
                if (comps.len > 1 && comps[1].len > 0) alloc.free(comps[1]);

                let addr = VCardAddress{ types: types };
                if (comps.len > 2) addr.street = comps[2];
                if (comps.len > 3) addr.city = comps[3];
                if (comps.len > 4) addr.region = comps[4];
                if (comps.len > 5) addr.postal_code = comps[5];
                if (comps.len > 6) addr.country = comps[6];
                if (comps.len > 0) alloc.free(comps);

                addrs_list.append(alloc, addr) catch {
                    addr.deinit(alloc);
                    return VCardError.OutOfMemory;
                };
            }
        }

        if (in_card) {
            return VCardError.UnbalancedCard;
        }

        return contacts.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
    }

    static fn parseOne(alloc: std.mem.Allocator, vcf_text: string): VCardContact ! VCardError {
        let list = VCardCodec.parseAll(alloc, vcf_text)?;
        if (list.len == 0) {
            alloc.free(list);
            return VCardError.InvalidFormat;
        }
        let first = list[0];
        let i: usize = 1;
        while (i < list.len) : (i += 1) {
            list[i].deinit();
        }
        alloc.free(list);
        return first;
    }

    static fn freeContacts(alloc: std.mem.Allocator, list: []VCardContact) {
        for (c of list) {
            let item = c;
            item.deinit();
        }
        if (list.len > 0) alloc.free(list);
    }

    static fn toVcf(alloc: std.mem.Allocator, contacts: []const VCardContact): ([]u8) ! VCardError {
        return VCardCodec.toVcfWithOptions(alloc, contacts, true);
    }

    static fn toVcfWithOptions(alloc: std.mem.Allocator, contacts: []const VCardContact, fold_75: bool): ([]u8) ! VCardError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let line_buf: std.ArrayList(u8) = .empty;
        defer line_buf.deinit(alloc);

        for (c of contacts) {
            if (c.fn_name.len == 0) return VCardError.MissingFn;

            out.appendSlice(alloc, "BEGIN:VCARD\r\n") catch return VCardError.OutOfMemory;
            out.appendSlice(alloc, "VERSION:") catch return VCardError.OutOfMemory;
            out.appendSlice(alloc, c.version.toStr()) catch return VCardError.OutOfMemory;
            out.appendSlice(alloc, "\r\n") catch return VCardError.OutOfMemory;

            if (c.uid.len > 0) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "UID:") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, c.uid)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            line_buf.clearRetainingCapacity();
            line_buf.appendSlice(alloc, "FN:") catch return VCardError.OutOfMemory;
            vcAppendEscapedText(*line_buf, alloc, c.fn_name)?;
            appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;

            if (sn := c.name) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "N:") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, sn.family)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, sn.given)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, sn.additional)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, sn.prefix)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, sn.suffix)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            if (c.org.len > 0) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "ORG:") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, c.org)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            if (c.title.len > 0) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "TITLE:") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, c.title)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            for (p of c.phones) {
                appendTypedProperty(*out, *line_buf, alloc, "TEL", p, fold_75)?;
            }

            for (e of c.emails) {
                appendTypedProperty(*out, *line_buf, alloc, "EMAIL", e, fold_75)?;
            }

            for (u of c.urls) {
                appendTypedProperty(*out, *line_buf, alloc, "URL", u, fold_75)?;
            }

            for (a of c.addresses) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "ADR") catch return VCardError.OutOfMemory;
                if (a.types.len > 0) {
                    line_buf.appendSlice(alloc, ";TYPE=") catch return VCardError.OutOfMemory;
                    let idx: usize = 0;
                    for (t of a.types) {
                        if (idx > 0) line_buf.append(alloc, ',') catch return VCardError.OutOfMemory;
                        line_buf.appendSlice(alloc, t) catch return VCardError.OutOfMemory;
                        idx += 1;
                    }
                }
                line_buf.appendSlice(alloc, ":;;") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, a.street)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, a.city)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, a.region)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, a.postal_code)?;
                line_buf.append(alloc, ';') catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, a.country)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            if (c.note.len > 0) {
                line_buf.clearRetainingCapacity();
                line_buf.appendSlice(alloc, "NOTE:") catch return VCardError.OutOfMemory;
                vcAppendEscapedText(*line_buf, alloc, c.note)?;
                appendFoldedLine(*out, alloc, line_buf.items, fold_75)?;
            }

            out.appendSlice(alloc, "END:VCARD\r\n") catch return VCardError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return VCardError.OutOfMemory;
    }
}

/// Zero-heap strict RFC 6350 / 2426 vCard validator:
/// rejects control bytes, bare CR, oversized lines/property floods, unbalanced BEGIN/END:VCARD,
/// invalid VERSION (only 3.0 / 4.0 allowed), missing FN, and unclosed parameter quotes.
pub fn validateVcardStrict(vcf_text: string): bool ! VCardError {
    let src = vcf_text;
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        src = src[3..];
    }
    if (src.len == 0 || simdCalHasControlByte(src)) {
        return VCardError.InvalidFormat;
    }

    let in_card: bool = false;
    let saw_card: bool = false;
    let saw_version: bool = false;
    let saw_fn: bool = false;
    let prop_count: usize = 0;

    let pos: usize = 0;
    while (pos < src.len) {
        const logical_start: usize = pos;
        let logical_len: usize = 0;
        let first_seg_end: usize = pos;
        let is_first_seg: bool = true;

        while (pos < src.len) {
            const hit = simdCalFindLineDelim(src, pos);
            if (hit < src.len && (src[hit] == '\r' || src[hit] == '\n')) {
                const seg_len = hit - pos;
                logical_len += seg_len;
                if (logical_len > VCARD_MAX_LINE_BYTES) {
                    return VCardError.InvalidFormat;
                }
                if (is_first_seg) {
                    first_seg_end = hit;
                    is_first_seg = false;
                }
                if (src[hit] == '\r') {
                    if (hit + 1 >= src.len || src[hit + 1] != '\n') {
                        return VCardError.InvalidFormat;
                    }
                    pos = hit + 2;
                } else {
                    pos = hit + 1;
                }
                if (pos < src.len && (src[pos] == ' ' || src[pos] == '\t')) {
                    pos += 1;
                    continue;
                }
                break;
            }
            if (hit >= src.len) {
                logical_len += src.len - pos;
                if (logical_len > VCARD_MAX_LINE_BYTES) {
                    return VCardError.InvalidFormat;
                }
                if (is_first_seg) {
                    first_seg_end = src.len;
                    is_first_seg = false;
                }
                pos = src.len;
                break;
            }
            logical_len += (hit - pos) + 1;
            if (logical_len > VCARD_MAX_LINE_BYTES) {
                return VCardError.InvalidFormat;
            }
            pos = hit + 1;
        }

        if (logical_len == 0) continue;
        const first_line = vcTrimSpaces(src[logical_start..first_seg_end]);
        if (first_line.len == 0 && logical_len == (first_seg_end - logical_start)) continue;

        prop_count += 1;
        if (prop_count > VCARD_MAX_PROPERTIES) {
            return VCardError.InvalidFormat;
        }

        // Validate property name, balanced parameter quotes, and ':' separator
        let in_q: bool = false;
        let colon_pos: usize? = null;
        let semi_pos: usize? = null;
        let scan_i: usize = logical_start;
        while (scan_i < pos) : (scan_i += 1) {
            const ch = src[scan_i];
            if (ch == '\r' || ch == '\n') {
                if (ch == '\r' && scan_i + 1 < pos && src[scan_i + 1] == '\n') {
                    scan_i += 1;
                }
                if (scan_i + 1 < pos && (src[scan_i + 1] == ' ' || src[scan_i + 1] == '\t')) {
                    scan_i += 1;
                    continue;
                }
                break;
            }
            if (ch == '"') {
                in_q = !in_q;
            } else if (!in_q) {
                if (ch == ';' && semi_pos == null) {
                    semi_pos = scan_i;
                } else if (ch == ':') {
                    colon_pos = scan_i;
                    break;
                }
            }
        }
        if (in_q) return VCardError.InvalidFormat;
        const c_idx = colon_pos ?? return VCardError.InvalidFormat;
        const name_end = semi_pos ?? c_idx;
        let raw_name = vcTrimSpaces(src[logical_start..name_end]);
        if (raw_name.len == 0) return VCardError.InvalidFormat;
        if (d := std.mem.indexOfScalar(u8, raw_name, '.')) {
            raw_name = vcTrimSpaces(raw_name[d + 1..]);
            if (raw_name.len == 0) return VCardError.InvalidFormat;
        }

        if (vcAsciiEqlIgnoreCase(raw_name, "BEGIN")) {
            const val = vcTrimSpaces(src[c_idx + 1 .. first_seg_end]);
            if (!vcAsciiEqlIgnoreCase(val, "VCARD")) return VCardError.InvalidFormat;
            if (in_card) return VCardError.UnbalancedCard;
            in_card = true;
            saw_card = true;
            saw_version = false;
            saw_fn = false;
        } else if (vcAsciiEqlIgnoreCase(raw_name, "END")) {
            const val = vcTrimSpaces(src[c_idx + 1 .. first_seg_end]);
            if (!vcAsciiEqlIgnoreCase(val, "VCARD")) return VCardError.InvalidFormat;
            if (!in_card) return VCardError.UnbalancedCard;
            if (!saw_version) return VCardError.UnsupportedVersion;
            if (!saw_fn) return VCardError.MissingFn;
            in_card = false;
        } else {
            if (!in_card) return VCardError.InvalidFormat;
            if (vcAsciiEqlIgnoreCase(raw_name, "VERSION")) {
                const val = vcTrimSpaces(src[c_idx + 1 .. first_seg_end]);
                if (VCardVersion.fromStr(val) == null) return VCardError.UnsupportedVersion;
                saw_version = true;
            } else if (vcAsciiEqlIgnoreCase(raw_name, "FN")) {
                // Non-empty FN value (either on first segment or folded continuation)
                const seg_val = vcTrimSpaces(src[c_idx + 1 .. first_seg_end]);
                if (seg_val.len > 0 || logical_len > (first_seg_end - logical_start)) {
                    saw_fn = true;
                }
            }
        }
    }

    if (!saw_card || in_card) {
        return VCardError.UnbalancedCard;
    }
    return true;
}
