// Origin: Pure-CP Stdlib (Zero-Alloc W3C Baggage Distributed Context & Redaction Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation W3C Baggage (Trace Context Level 2) Propagation & Redaction Engine:
//   - Fixed-capacity zero-heap storage strictly enforcing W3C specification limits (entry cap,
//     per-member 4096B cap, and total header <= 8192B cap)
//   - RFC-compliant `parseBaggageHeader` / `formatBaggageHeader`: comma-separated members,
//     `key=value;prop1=v1;prop2` metadata properties, OWS trimming, `%XX` percent-encoding/decoding,
//     Last-Write-Wins duplicate key deduplication, and strict RFC 7230 token key validation
//   - `BaggageContext`: `init`, `set`, `setWithProps`, `get`, `getProps`, `remove`, `has`, `len`,
//     `clear`, `mergeFrom` (cross-process / cross-agent propagation with prefix allowlist &
//     overwrite policy), `redactKeys` (in-place PII/secret masking), `estimatedHeaderBytes`
//   - Security boundary guards: blocks CRLF/control-char injection, oversized header floods,
//     invalid token keys, malformed `%XX` escapes & high-cardinality overflow (`BaggageParseReport`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed maximum number of baggage entries stored inline in `BaggageContext`.
pub const BAGGAGE_MAX_ENTRIES: usize = 32;

/// W3C Baggage specification maximum total header byte length (8 KiB).
pub const BAGGAGE_MAX_HEADER_BYTES: usize = 8192;

/// W3C Baggage specification maximum single list-member byte length (4 KiB).
pub const BAGGAGE_MAX_MEMBER_BYTES: usize = 4096;

/// Maximum byte length of a single baggage key.
pub const BAGGAGE_MAX_KEY_LEN: usize = 64;

/// Maximum decoded byte length of a single baggage value.
pub const BAGGAGE_MAX_VALUE_LEN: usize = 160;

/// Maximum byte length of optional semicolon-delimited member properties.
pub const BAGGAGE_MAX_PROPS_LEN: usize = 128;

/// Operation or parse status returned by `BaggageContext` & `parseBaggageHeader`.
pub enum BaggageOpStatus {
    ok,
    invalid_key,
    invalid_value,
    invalid_props,
    crlf_rejected,
    header_too_large,
    capacity_full,
    buffer_too_small,
}

/// Single W3C baggage key-value entry with optional metadata properties.
pub struct BaggageEntry {
    active: bool = false;
    key_buf: [BAGGAGE_MAX_KEY_LEN]u8 = [0; BAGGAGE_MAX_KEY_LEN];
    key_len: usize = 0;
    val_buf: [BAGGAGE_MAX_VALUE_LEN]u8 = [0; BAGGAGE_MAX_VALUE_LEN];
    val_len: usize = 0;
    props_buf: [BAGGAGE_MAX_PROPS_LEN]u8 = [0; BAGGAGE_MAX_PROPS_LEN];
    props_len: usize = 0;

    /// Return the entry key slice.
    fn key(): []const u8 {
        return key_buf[0..key_len];
    }

    /// Return the decoded entry value slice.
    fn value(): []const u8 {
        return val_buf[0..val_len];
    }

    /// Return the optional properties slice (without leading `;`).
    fn props(): []const u8 {
        return props_buf[0..props_len];
    }
}

/// Telemetry & truncation report returned by `parseBaggageHeader`.
pub struct BaggageParseReport {
    status: BaggageOpStatus = BaggageOpStatus.ok;
    parsed_members: usize = 0;
    stored_entries: usize = 0;
    overwritten_duplicates: usize = 0;
    skipped_invalid: usize = 0;
    dropped_overflow: usize = 0;
    header_bytes: usize = 0;
}

// ============================================================================
// 2. Character Validation, Percent-Encoding & Trimming Helpers
// ============================================================================

fn hasCrlfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n' || s[i] == 0) return true;
    }
    return false;
}

fn hasCtlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < 0x20 || s[i] == 0x7F) return true;
    }
    return false;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isOws(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isOws(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn isTchar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' ||
        c == '*' || c == '+' || c == '-' || c == '.' || c == '^' || c == '_' ||
        c == '`' || c == '|' || c == '~';
}

/// Validate a W3C Baggage key (non-empty RFC 7230 token within `BAGGAGE_MAX_KEY_LEN`).
pub fn isValidBaggageKey(key: []const u8): bool {
    if (key.len == 0 || key.len > BAGGAGE_MAX_KEY_LEN) return false;
    let i: usize = 0;
    while (i < key.len) : (i += 1) {
        if (!isTchar(key[i])) return false;
    }
    return true;
}

/// Return true if byte `c` is a safe unencoded W3C `baggage-octet` (excludes CTL, space, `"`, `,`, `;`, `\`, `%`, non-ASCII).
fn isUnencodedBaggageOctet(c: u8): bool {
    if (c <= 0x20 || c >= 0x7F) return false;
    if (c == '"' || c == ',' || c == ';' || c == '\\' || c == '%') return false;
    return true;
}

fn isValidBaggageProps(props: []const u8): bool {
    if (props.len > BAGGAGE_MAX_PROPS_LEN) return false;
    let i: usize = 0;
    while (i < props.len) : (i += 1) {
        const c = props[i];
        if (c < 0x20 || c == 0x7F || c == ',') return false;
    }
    return true;
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return null;
}

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn encodedValueLen(val: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        if (isUnencodedBaggageOctet(val[i])) {
            total += 1;
        } else {
            total += 3;
        }
    }
    return total;
}

fn percentEncodeValueInto(val: []const u8, out_buf: *[]u8, pos: *usize): bool {
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        const c = val[i];
        if (isUnencodedBaggageOctet(c)) {
            if (pos + 1 > out_buf.len) return false;
            out_buf[pos] = c;
            pos += 1;
        } else {
            if (pos + 3 > out_buf.len) return false;
            out_buf[pos] = '%';
            out_buf[pos + 1] = HEX_UPPER[@as(usize, (c >> 4) & 0x0F)];
            out_buf[pos + 2] = HEX_UPPER[@as(usize, c & 0x0F)];
            pos += 3;
        }
    }
    return true;
}

fn percentDecodeInto(raw: []const u8, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '%') {
            if (i + 2 >= raw.len) return null;
            const hi = hexVal(raw[i + 1]) ?? return null;
            const lo = hexVal(raw[i + 2]) ?? return null;
            const b: u8 = (hi << 4) | lo;
            if (b == '\r' || b == '\n' || b == 0) return null;
            if (pos >= out_buf.len) return null;
            out_buf[pos] = b;
            pos += 1;
            i += 3;
        } else {
            if (raw[i] < 0x20 || raw[i] == 0x7F) return null;
            if (pos >= out_buf.len) return null;
            out_buf[pos] = raw[i];
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn asciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn startsWithBytes(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return bytesEq(s[0..prefix.len], prefix);
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

// ============================================================================
// 3. BaggageContext — Zero-Heap Distributed Context Container
// ============================================================================

/// Zero-heap-allocation W3C Baggage context store with propagation merge & redaction.
pub struct BaggageContext {
    entries: [BAGGAGE_MAX_ENTRIES]BaggageEntry = [BaggageEntry{}; BAGGAGE_MAX_ENTRIES];
    count: usize = 0;

    /// Create an empty `BaggageContext`.
    static fn init(): BaggageContext {
        return BaggageContext{};
    }

    /// Return current number of active baggage entries.
    fn len(): usize {
        return count;
    }

    /// Remove all entries from the context.
    fn clear(*_) {
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            entries[i] = BaggageEntry{};
        }
        count = 0;
    }

    fn findSlot(key: []const u8): usize? {
        if (key.len == 0) return null;
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active && bytesEq(entries[i].key(), key)) {
                return i;
            }
        }
        return null;
    }

    /// Return true if `key` exists in the context.
    fn has(key: []const u8): bool {
        return findSlot(key) != null;
    }

    /// Retrieve the decoded value for `key`, or `null` if absent.
    fn get(key: []const u8): ([]const u8)? {
        if (idx := findSlot(key)) {
            return entries[idx].val_buf[0..entries[idx].val_len];
        }
        return null;
    }

    /// Retrieve the optional metadata properties string for `key` (empty slice if none),
    /// or `null` if `key` is absent.
    fn getProps(key: []const u8): ([]const u8)? {
        if (idx := findSlot(key)) {
            return entries[idx].props_buf[0..entries[idx].props_len];
        }
        return null;
    }

    /// Estimate exact serialized W3C `baggage` header byte length.
    fn estimatedHeaderBytes(): usize {
        let total: usize = 0;
        let seen: usize = 0;
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            if (seen > 0) total += 1; // ','
            total += entries[i].key_len + 1 + encodedValueLen(entries[i].value()); // key=val
            if (entries[i].props_len > 0) {
                total += 1 + entries[i].props_len; // ;props
            }
            seen += 1;
        }
        return total;
    }

    /// Insert or update a baggage entry with empty properties.
    fn set(*_, key: []const u8, value: []const u8): BaggageOpStatus {
        return setWithProps(key, value, "");
    }

    /// Insert or update a baggage entry with optional semicolon-delimited metadata properties.
    /// Enforces CRLF guard, RFC 7230 key token syntax, entry cap, and 8192B header cap.
    fn setWithProps(*_, key: []const u8, value: []const u8, props: []const u8): BaggageOpStatus {
        if (hasCrlfOrNul(key) || hasCrlfOrNul(value) || hasCrlfOrNul(props)) {
            return BaggageOpStatus.crlf_rejected;
        }
        if (!isValidBaggageKey(key)) return BaggageOpStatus.invalid_key;
        if (value.len > BAGGAGE_MAX_VALUE_LEN || hasCtlChar(value)) return BaggageOpStatus.invalid_value;
        if (!isValidBaggageProps(props)) return BaggageOpStatus.invalid_props;

        const new_member_bytes = key.len + 1 + encodedValueLen(value) + (if (props.len > 0) 1 + props.len else 0);
        if (new_member_bytes > BAGGAGE_MAX_MEMBER_BYTES) {
            return BaggageOpStatus.header_too_large;
        }

        if (existing := findSlot(key)) {
            const old_member_bytes = entries[existing].key_len + 1 +
                encodedValueLen(entries[existing].value()) +
                (if (entries[existing].props_len > 0) 1 + entries[existing].props_len else 0);
            const cur_total = estimatedHeaderBytes();
            const next_total = (cur_total - old_member_bytes) + new_member_bytes;
            if (next_total > BAGGAGE_MAX_HEADER_BYTES) {
                return BaggageOpStatus.header_too_large;
            }
            let v_i: usize = 0;
            while (v_i < value.len) : (v_i += 1) {
                entries[existing].val_buf[v_i] = value[v_i];
            }
            entries[existing].val_len = value.len;
            let p_i: usize = 0;
            while (p_i < props.len) : (p_i += 1) {
                entries[existing].props_buf[p_i] = props[p_i];
            }
            entries[existing].props_len = props.len;
            return BaggageOpStatus.ok;
        }

        // New entry: check capacity and total header size
        let free_idx: usize = BAGGAGE_MAX_ENTRIES;
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == BAGGAGE_MAX_ENTRIES) return BaggageOpStatus.capacity_full;

        const cur_total = estimatedHeaderBytes();
        const sep_bytes: usize = if (count > 0) 1 else 0;
        if (cur_total + sep_bytes + new_member_bytes > BAGGAGE_MAX_HEADER_BYTES) {
            return BaggageOpStatus.header_too_large;
        }

        let e = BaggageEntry{};
        e.active = true;
        let k_i: usize = 0;
        while (k_i < key.len) : (k_i += 1) {
            e.key_buf[k_i] = key[k_i];
        }
        e.key_len = key.len;
        let v_i: usize = 0;
        while (v_i < value.len) : (v_i += 1) {
            e.val_buf[v_i] = value[v_i];
        }
        e.val_len = value.len;
        let p_i: usize = 0;
        while (p_i < props.len) : (p_i += 1) {
            e.props_buf[p_i] = props[p_i];
        }
        e.props_len = props.len;

        entries[free_idx] = e;
        count += 1;
        return BaggageOpStatus.ok;
    }

    /// Remove an entry by `key`. Returns `true` if removed.
    fn remove(*_, key: []const u8): bool {
        const idx = findSlot(key) ?? return false;
        entries[idx] = BaggageEntry{};
        if (count > 0) count -= 1;
        return true;
    }

    /// Merge entries from `src` into this context for cross-process / cross-agent propagation.
    /// - If `allowed_prefix` is non-empty, only keys starting with `allowed_prefix` are imported.
    /// - If `overwrite_existing` is false, existing keys in this context are preserved read-only.
    /// Returns the number of entries added or updated.
    fn mergeFrom(
        *_,
        src: BaggageContext,
        overwrite_existing: bool,
        allowed_prefix: []const u8,
    ): usize {
        let merged: usize = 0;
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            if (!src.entries[i].active) continue;
            const k = src.entries[i].key();
            if (allowed_prefix.len > 0 && !startsWithBytes(k, allowed_prefix)) continue;
            if (!overwrite_existing && has(k)) continue;
            if (setWithProps(k, src.entries[i].value(), src.entries[i].props()) == BaggageOpStatus.ok) {
                merged += 1;
            }
        }
        return merged;
    }

    /// Redact values of entries matching any key in `sensitive_keys` (case-insensitive)
    /// by replacing their values in-place with `mask` (defaults to `"[REDACTED]"` if empty).
    /// Returns the number of entries redacted.
    fn redactKeys(*_, sensitive_keys: []const []const u8, mask: []const u8): usize {
        const eff_mask: []const u8 = if (mask.len == 0) "[REDACTED]" else mask;
        if (eff_mask.len > BAGGAGE_MAX_VALUE_LEN || hasCtlChar(eff_mask)) return 0;

        let redacted: usize = 0;
        let i: usize = 0;
        while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active) continue;
            let matched = false;
            let s_i: usize = 0;
            while (s_i < sensitive_keys.len) : (s_i += 1) {
                if (asciiEqIgnoreCase(entries[i].key(), sensitive_keys[s_i])) {
                    matched = true;
                    break;
                }
            }
            if (matched) {
                let m_i: usize = 0;
                while (m_i < eff_mask.len) : (m_i += 1) {
                    entries[i].val_buf[m_i] = eff_mask[m_i];
                }
                entries[i].val_len = eff_mask.len;
                redacted += 1;
            }
        }
        return redacted;
    }
}

// ============================================================================
// 4. W3C Baggage Header Parser & Formatter
// ============================================================================

/// Parse a W3C `baggage` HTTP header into `out_ctx` (cleared first).
/// Enforces CRLF guard, 8192B header cap, 4096B per-member cap, `%XX` decoding,
/// RFC 7230 key validation, and Last-Write-Wins duplicate deduplication.
pub fn parseBaggageHeader(header: []const u8, out_ctx: *BaggageContext): BaggageParseReport {
    out_ctx.clear();
    let rep = BaggageParseReport{};
    rep.header_bytes = header.len;

    if (hasCrlfOrNul(header)) {
        rep.status = BaggageOpStatus.crlf_rejected;
        return rep;
    }
    if (header.len > BAGGAGE_MAX_HEADER_BYTES) {
        rep.status = BaggageOpStatus.header_too_large;
        return rep;
    }
    if (header.len == 0) {
        rep.status = BaggageOpStatus.ok;
        return rep;
    }

    let start: usize = 0;
    while (start <= header.len) {
        let end = start;
        while (end < header.len && header[end] != ',') : (end += 1) {}
        const member_raw = header[start..end];
        const member = trimOws(member_raw);

        if (member.len > 0) {
            rep.parsed_members += 1;
            if (member.len > BAGGAGE_MAX_MEMBER_BYTES) {
                rep.skipped_invalid += 1;
            } else {
                // Split optional `;props`
                let semi_idx = member.len;
                let k: usize = 0;
                while (k < member.len) : (k += 1) {
                    if (member[k] == ';') {
                        semi_idx = k;
                        break;
                    }
                }
                const kv_part = trimOws(member[0..semi_idx]);
                const props_part: []const u8 = if (semi_idx < member.len) trimOws(member[semi_idx + 1 ..]) else "";

                // Split `key=value` on first '='
                let eq_idx = kv_part.len;
                let e_i: usize = 0;
                while (e_i < kv_part.len) : (e_i += 1) {
                    if (kv_part[e_i] == '=') {
                        eq_idx = e_i;
                        break;
                    }
                }

                if (eq_idx == kv_part.len) {
                    rep.skipped_invalid += 1;
                } else {
                    const key = trimOws(kv_part[0..eq_idx]);
                    const raw_val = trimOws(kv_part[eq_idx + 1 ..]);

                    let dec_buf: [BAGGAGE_MAX_VALUE_LEN]u8 = [0; BAGGAGE_MAX_VALUE_LEN];
                    let dec_s: []u8 = dec_buf[0..];
                    if (dec_len := percentDecodeInto(raw_val, *dec_s)) {
                        const existed = out_ctx.has(key);
                        const st = out_ctx.setWithProps(key, dec_buf[0..dec_len], props_part);
                        if (st == BaggageOpStatus.ok) {
                            if (existed) {
                                rep.overwritten_duplicates += 1;
                            }
                        } else if (st == BaggageOpStatus.capacity_full) {
                            rep.dropped_overflow += 1;
                            rep.status = BaggageOpStatus.capacity_full;
                        } else {
                            rep.skipped_invalid += 1;
                        }
                    } else {
                        rep.skipped_invalid += 1;
                    }
                }
            }
        }

        if (end >= header.len) break;
        start = end + 1;
    }

    rep.stored_entries = out_ctx.len();
    return rep;
}

/// Format `ctx` into a W3C-compliant `baggage` HTTP header value in `out_buf`.
/// Automatically percent-encodes values and preserves `;props` metadata.
/// Returns bytes written, or `null` if `out_buf` is too small or exceeds `BAGGAGE_MAX_HEADER_BYTES`.
pub fn formatBaggageHeader(ctx: BaggageContext, out_buf: *[]u8): usize? {
    const needed = ctx.estimatedHeaderBytes();
    if (needed > BAGGAGE_MAX_HEADER_BYTES || needed > out_buf.len) return null;

    let pos: usize = 0;
    let written_entries: usize = 0;
    let i: usize = 0;
    while (i < BAGGAGE_MAX_ENTRIES) : (i += 1) {
        if (!ctx.entries[i].active) continue;
        if (written_entries > 0) {
            if (!appendBytes(out_buf, *pos, ",")) return null;
        }
        if (!appendBytes(out_buf, *pos, ctx.entries[i].key())) return null;
        if (!appendBytes(out_buf, *pos, "=")) return null;
        if (!percentEncodeValueInto(ctx.entries[i].value(), out_buf, *pos)) return null;
        if (ctx.entries[i].props_len > 0) {
            if (!appendBytes(out_buf, *pos, ";")) return null;
            if (!appendBytes(out_buf, *pos, ctx.entries[i].props())) return null;
        }
        written_entries += 1;
    }
    return pos;
}
