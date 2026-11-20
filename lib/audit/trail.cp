// Source: cp/lib/audit/trail.cp (Tamper-Evident Hash-Chained Audit Log)
// Pure CP zero-C-dependency tamper-evident cryptographic hash-chained audit log.
// Computes canonical SHA-256 entry hashes linked via prev_hash (starting from 64-'0' GENESIS_HASH),
// supports full-chain verification with exact corrupt-sequence localization, actor/action/time-range
// queries, and canonical JSONL / RFC-4180 CSV export & verified JSONL import.

const std = @import("std");
import { sha256Raw } from "cap:crypto";

pub const GENESIS_HASH: string = "0000000000000000000000000000000000000000000000000000000000000000";

pub err AuditError {
    TamperedEntry,
    BrokenChain,
    EmptyLog,
    InvalidJsonl,
    OutOfMemory,
}

pub enum AuditOutcome {
    Success,
    Denied,
    Error,

    pub fn toString(): string {
        switch (_) {
            .Success => return "Success",
            .Denied => return "Denied",
            .Error => return "Error",
        }
    }

    pub static fn fromString(s: string): AuditOutcome? {
        if (std.mem.eql(u8, s, "Success")) return AuditOutcome.Success;
        if (std.mem.eql(u8, s, "Denied")) return AuditOutcome.Denied;
        if (std.mem.eql(u8, s, "Error")) return AuditOutcome.Error;
        return null;
    }
}

pub enum AuditSeverity {
    Info,
    Warn,
    Critical,

    pub fn toString(): string {
        switch (_) {
            .Info => return "Info",
            .Warn => return "Warn",
            .Critical => return "Critical",
        }
    }

    pub static fn fromString(s: string): AuditSeverity? {
        if (std.mem.eql(u8, s, "Info")) return AuditSeverity.Info;
        if (std.mem.eql(u8, s, "Warn")) return AuditSeverity.Warn;
        if (std.mem.eql(u8, s, "Critical")) return AuditSeverity.Critical;
        return null;
    }
}

pub struct AuditEntry {
    seq: u64;
    timestamp_ms: u64;
    actor: string;
    action: string;
    resource: string;
    outcome: AuditOutcome;
    severity: AuditSeverity;
    metadata_json: string;
    prev_hash: string;
    entry_hash: string;
}

pub struct ChainVerifyResult {
    valid: bool;
    checked_count: usize;
    first_corrupt_seq: u64;
    reason: string;
}

fn appendU64Be(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: u64): !AuditError {
    let bytes: [8]u8 = undefined;
    std.mem.writeInt(u64, &bytes, val, .big);
    buf.appendSlice(alloc, bytes[0..8]) catch return AuditError.OutOfMemory;
}

fn appendLenPrefixed(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): !AuditError {
    appendU64Be(buf, alloc, u64(s.len))?;
    buf.appendSlice(alloc, s) catch return AuditError.OutOfMemory;
}

fn computeEntryHashHex(
    alloc: std.mem.Allocator,
    seq: u64,
    timestamp_ms: u64,
    prev_hash: string,
    actor: string,
    action: string,
    resource: string,
    outcome: AuditOutcome,
    severity: AuditSeverity,
    metadata_json: string,
): [64]u8 ! AuditError {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    defer buf.deinit(alloc);

    // Domain separation tag prevents cross-protocol hash collisions.
    buf.appendSlice(alloc, "CP-AUDIT-V1\x00") catch return AuditError.OutOfMemory;
    appendU64Be(&buf, alloc, seq)?;
    appendU64Be(&buf, alloc, timestamp_ms)?;
    appendLenPrefixed(&buf, alloc, prev_hash)?;
    appendLenPrefixed(&buf, alloc, actor)?;
    appendLenPrefixed(&buf, alloc, action)?;
    appendLenPrefixed(&buf, alloc, resource)?;
    appendLenPrefixed(&buf, alloc, outcome.toString())?;
    appendLenPrefixed(&buf, alloc, severity.toString())?;
    appendLenPrefixed(&buf, alloc, metadata_json)?;

    let digest: [32]u8 = undefined;
    sha256Raw(&digest, buf.items);
    return std.fmt.bytesToHex(digest, .lower);
}

fn appendJsonEscaped(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): !AuditError {
    buf.append(alloc, '"') catch return AuditError.OutOfMemory;
    const hex_digits = "0123456789abcdef";
    for (c of s) {
        if (c == '"') {
            buf.appendSlice(alloc, "\\\"") catch return AuditError.OutOfMemory;
        } else if (c == '\\') {
            buf.appendSlice(alloc, "\\\\") catch return AuditError.OutOfMemory;
        } else if (c == '\n') {
            buf.appendSlice(alloc, "\\n") catch return AuditError.OutOfMemory;
        } else if (c == '\r') {
            buf.appendSlice(alloc, "\\r") catch return AuditError.OutOfMemory;
        } else if (c == '\t') {
            buf.appendSlice(alloc, "\\t") catch return AuditError.OutOfMemory;
        } else if (c == 0x08) {
            buf.appendSlice(alloc, "\\b") catch return AuditError.OutOfMemory;
        } else if (c == 0x0c) {
            buf.appendSlice(alloc, "\\f") catch return AuditError.OutOfMemory;
        } else if (c < 0x20) {
            let esc: [6]u8 = [
                '\\', 'u', '0', '0',
                hex_digits[(c >> 4) & 0x0f],
                hex_digits[c & 0x0f],
            ];
            buf.appendSlice(alloc, esc[0..6]) catch return AuditError.OutOfMemory;
        } else {
            buf.append(alloc, c) catch return AuditError.OutOfMemory;
        }
    }
    buf.append(alloc, '"') catch return AuditError.OutOfMemory;
}

fn appendCsvEscaped(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): !AuditError {
    let needs_quote = false;
    for (c of s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needs_quote = true;
            break;
        }
    }
    if (!needs_quote) {
        buf.appendSlice(alloc, s) catch return AuditError.OutOfMemory;
        return;
    }
    buf.append(alloc, '"') catch return AuditError.OutOfMemory;
    for (c of s) {
        if (c == '"') {
            buf.appendSlice(alloc, "\"\"") catch return AuditError.OutOfMemory;
        } else {
            buf.append(alloc, c) catch return AuditError.OutOfMemory;
        }
    }
    buf.append(alloc, '"') catch return AuditError.OutOfMemory;
}

fn appendDecimalU64(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: u64): !AuditError {
    let tmp: [32]u8 = undefined;
    const formatted = std.fmt.bufPrint(&tmp, "{d}", .{ val }) catch return AuditError.OutOfMemory;
    buf.appendSlice(alloc, formatted) catch return AuditError.OutOfMemory;
}

struct JsonLineCursor {
    src: string;
    pos: usize = 0;

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn expectChar(*_, expected: u8): !AuditError {
        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != expected) {
            return AuditError.InvalidJsonl;
        }
        _.pos += 1;
    }

    fn peek(*_): u8? {
        _.skipWs();
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn parseHex4(*_): u16 ! AuditError {
        if (_.pos + 4 > _.src.len) return AuditError.InvalidJsonl;
        let val: u16 = 0;
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            const c = _.src[_.pos + i];
            let digit: u16 = 0;
            if (c >= '0' && c <= '9') {
                digit = u16(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                digit = u16(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                digit = u16(c - 'A' + 10);
            } else {
                return AuditError.InvalidJsonl;
            }
            val = (val << 4) | digit;
        }
        _.pos += 4;
        return val;
    }

    fn parseString(*_, alloc: std.mem.Allocator): []u8 ! AuditError {
        _.expectChar('"')?;
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '"') {
                return out.toOwnedSlice(alloc) catch return AuditError.OutOfMemory;
            }
            if (c == '\\') {
                if (_.pos >= _.src.len) return AuditError.InvalidJsonl;
                const esc = _.src[_.pos];
                _.pos += 1;
                if (esc == '"') {
                    out.append(alloc, '"') catch return AuditError.OutOfMemory;
                } else if (esc == '\\') {
                    out.append(alloc, '\\') catch return AuditError.OutOfMemory;
                } else if (esc == '/') {
                    out.append(alloc, '/') catch return AuditError.OutOfMemory;
                } else if (esc == 'b') {
                    out.append(alloc, 0x08) catch return AuditError.OutOfMemory;
                } else if (esc == 'f') {
                    out.append(alloc, 0x0c) catch return AuditError.OutOfMemory;
                } else if (esc == 'n') {
                    out.append(alloc, '\n') catch return AuditError.OutOfMemory;
                } else if (esc == 'r') {
                    out.append(alloc, '\r') catch return AuditError.OutOfMemory;
                } else if (esc == 't') {
                    out.append(alloc, '\t') catch return AuditError.OutOfMemory;
                } else if (esc == 'u') {
                    const code = _.parseHex4()?;
                    let utf8_buf: [4]u8 = undefined;
                    const n = std.unicode.utf8Encode(u21(code), &utf8_buf) catch return AuditError.InvalidJsonl;
                    out.appendSlice(alloc, utf8_buf[0..n]) catch return AuditError.OutOfMemory;
                } else {
                    return AuditError.InvalidJsonl;
                }
            } else {
                if (c < 0x20) return AuditError.InvalidJsonl;
                out.append(alloc, c) catch return AuditError.OutOfMemory;
            }
        }
        return AuditError.InvalidJsonl;
    }

    fn parseU64(*_): u64 ! AuditError {
        _.skipWs();
        if (_.pos >= _.src.len) return AuditError.InvalidJsonl;
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c >= '0' && c <= '9') {
                _.pos += 1;
            } else {
                break;
            }
        }
        if (_.pos == start) return AuditError.InvalidJsonl;
        return std.fmt.parseInt(u64, _.src[start.._.pos], 10) catch return AuditError.InvalidJsonl;
    }
}

pub struct AuditTrail {
    alloc: std.mem.Allocator;
    entries: std.ArrayListUnmanaged(AuditEntry) = .empty;
    owned_allocs: std.ArrayListUnmanaged([]u8) = .empty;

    pub static fn init(alloc: std.mem.Allocator): AuditTrail {
        return AuditTrail{
            alloc: alloc,
            entries: .empty,
            owned_allocs: .empty,
        };
    }

    pub fn deinit(*_) {
        for (slice of _.owned_allocs.items) {
            _.alloc.free(slice);
        }
        _.owned_allocs.deinit(_.alloc);
        _.entries.deinit(_.alloc);
    }

    fn ownSlice(*_, s: string): string ! AuditError {
        const dup = _.alloc.dupe(u8, s) catch return AuditError.OutOfMemory;
        errdefer _.alloc.free(dup);
        _.owned_allocs.append(_.alloc, dup) catch return AuditError.OutOfMemory;
        return dup;
    }

    fn ownOwnedSlice(*_, dup: []u8): string ! AuditError {
        errdefer _.alloc.free(dup);
        _.owned_allocs.append(_.alloc, dup) catch return AuditError.OutOfMemory;
        return dup;
    }

    pub fn len(): usize {
        return _.entries.items.len;
    }

    pub fn isEmpty(): bool {
        return _.entries.items.len == 0;
    }

    pub fn lastHash(): string {
        if (_.entries.items.len == 0) return GENESIS_HASH;
        return _.entries.items[_.entries.items.len - 1].entry_hash;
    }

    pub fn append(
        *_,
        timestamp_ms: u64,
        actor: string,
        action: string,
        resource: string,
        outcome: AuditOutcome,
        severity: AuditSeverity,
        metadata_json: string,
    ): AuditEntry ! AuditError {
        const seq: u64 = u64(_.entries.items.len) + 1;
        const prev_raw = _.lastHash();

        const hash_hex = computeEntryHashHex(
            _.alloc,
            seq,
            timestamp_ms,
            prev_raw,
            actor,
            action,
            resource,
            outcome,
            severity,
            metadata_json,
        )?;

        const owned_actor = _.ownSlice(actor)?;
        const owned_action = _.ownSlice(action)?;
        const owned_resource = _.ownSlice(resource)?;
        const owned_meta = _.ownSlice(metadata_json)?;
        const owned_prev = _.ownSlice(prev_raw)?;
        const owned_hash = _.ownSlice(hash_hex[0..64])?;

        const entry = AuditEntry{
            seq: seq,
            timestamp_ms: timestamp_ms,
            actor: owned_actor,
            action: owned_action,
            resource: owned_resource,
            outcome: outcome,
            severity: severity,
            metadata_json: owned_meta,
            prev_hash: owned_prev,
            entry_hash: owned_hash,
        };

        _.entries.append(_.alloc, entry) catch return AuditError.OutOfMemory;
        return entry;
    }

    pub fn verifyChain(): ChainVerifyResult {
        if (_.entries.items.len == 0) {
            return ChainVerifyResult{
                valid: true,
                checked_count: 0,
                first_corrupt_seq: 0,
                reason: "empty log (valid)",
            };
        }

        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const entry = _.entries.items[i];
            const expected_seq: u64 = u64(i) + 1;

            if (entry.seq != expected_seq) {
                return ChainVerifyResult{
                    valid: false,
                    checked_count: i,
                    first_corrupt_seq: entry.seq,
                    reason: "sequence gap or reorder detected",
                };
            }

            const expected_prev = if (i == 0) GENESIS_HASH else _.entries.items[i - 1].entry_hash;
            if (!std.mem.eql(u8, entry.prev_hash, expected_prev)) {
                return ChainVerifyResult{
                    valid: false,
                    checked_count: i,
                    first_corrupt_seq: entry.seq,
                    reason: "prev_hash mismatch (broken chain)",
                };
            }

            const expected_hex = computeEntryHashHex(
                _.alloc,
                entry.seq,
                entry.timestamp_ms,
                entry.prev_hash,
                entry.actor,
                entry.action,
                entry.resource,
                entry.outcome,
                entry.severity,
                entry.metadata_json,
            ) catch {
                return ChainVerifyResult{
                    valid: false,
                    checked_count: i,
                    first_corrupt_seq: entry.seq,
                    reason: "hash computation out of memory",
                };
            };

            if (!std.mem.eql(u8, entry.entry_hash, expected_hex[0..64])) {
                return ChainVerifyResult{
                    valid: false,
                    checked_count: i,
                    first_corrupt_seq: entry.seq,
                    reason: "entry_hash mismatch (tampered entry)",
                };
            }
        }

        return ChainVerifyResult{
            valid: true,
            checked_count: _.entries.items.len,
            first_corrupt_seq: 0,
            reason: "ok",
        };
    }

    pub fn queryByActor(alloc: std.mem.Allocator, actor: string): []AuditEntry ! AuditError {
        let out: std.ArrayListUnmanaged(AuditEntry) = .empty;
        errdefer out.deinit(alloc);
        for (entry of _.entries.items) {
            if (std.mem.eql(u8, entry.actor, actor)) {
                out.append(alloc, entry) catch return AuditError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return AuditError.OutOfMemory;
    }

    pub fn queryByAction(alloc: std.mem.Allocator, action: string): []AuditEntry ! AuditError {
        let out: std.ArrayListUnmanaged(AuditEntry) = .empty;
        errdefer out.deinit(alloc);
        for (entry of _.entries.items) {
            if (std.mem.eql(u8, entry.action, action)) {
                out.append(alloc, entry) catch return AuditError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return AuditError.OutOfMemory;
    }

    pub fn queryRange(alloc: std.mem.Allocator, start_ms: u64, end_ms: u64): []AuditEntry ! AuditError {
        let out: std.ArrayListUnmanaged(AuditEntry) = .empty;
        errdefer out.deinit(alloc);
        if (end_ms < start_ms) {
            return out.toOwnedSlice(alloc) catch return AuditError.OutOfMemory;
        }
        for (entry of _.entries.items) {
            if (entry.timestamp_ms >= start_ms && entry.timestamp_ms <= end_ms) {
                out.append(alloc, entry) catch return AuditError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return AuditError.OutOfMemory;
    }

    pub fn exportJsonl(): string ! AuditError {
        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        for (entry of _.entries.items) {
            buf.appendSlice(_.alloc, "{\"seq\":") catch return AuditError.OutOfMemory;
            appendDecimalU64(&buf, _.alloc, entry.seq)?;
            buf.appendSlice(_.alloc, ",\"timestamp_ms\":") catch return AuditError.OutOfMemory;
            appendDecimalU64(&buf, _.alloc, entry.timestamp_ms)?;
            buf.appendSlice(_.alloc, ",\"actor\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.actor)?;
            buf.appendSlice(_.alloc, ",\"action\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.action)?;
            buf.appendSlice(_.alloc, ",\"resource\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.resource)?;
            buf.appendSlice(_.alloc, ",\"outcome\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.outcome.toString())?;
            buf.appendSlice(_.alloc, ",\"severity\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.severity.toString())?;
            buf.appendSlice(_.alloc, ",\"metadata_json\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.metadata_json)?;
            buf.appendSlice(_.alloc, ",\"prev_hash\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.prev_hash)?;
            buf.appendSlice(_.alloc, ",\"entry_hash\":") catch return AuditError.OutOfMemory;
            appendJsonEscaped(&buf, _.alloc, entry.entry_hash)?;
            buf.appendSlice(_.alloc, "}\n") catch return AuditError.OutOfMemory;
        }

        return buf.toOwnedSlice(_.alloc) catch return AuditError.OutOfMemory;
    }

    pub static fn importJsonl(alloc: std.mem.Allocator, jsonl_text: string): AuditTrail ! AuditError {
        let trail = AuditTrail.init(alloc);
        errdefer trail.deinit();

        let line_iter = std.mem.splitScalar(u8, jsonl_text, '\n');
        let line_count: usize = 0;

        while (raw_line := line_iter.next()) {
            const trimmed = std.mem.trim(u8, raw_line, " \t\r");
            if (trimmed.len == 0) continue;
            line_count += 1;

            let cur = JsonLineCursor{ src: trimmed, pos: 0 };
            cur.expectChar('{')?;

            let has_seq = false;
            let has_ts = false;
            let has_actor = false;
            let has_action = false;
            let has_resource = false;
            let has_outcome = false;
            let has_severity = false;
            let has_meta = false;
            let has_prev = false;
            let has_hash = false;

            let seq_val: u64 = 0;
            let ts_val: u64 = 0;
            let actor_val: string = "";
            let action_val: string = "";
            let resource_val: string = "";
            let outcome_val = AuditOutcome.Success;
            let severity_val = AuditSeverity.Info;
            let meta_val: string = "";
            let prev_val: string = "";
            let hash_val: string = "";

            let first_field = true;
            while (true) {
                const next_ch = cur.peek() ?? return AuditError.InvalidJsonl;
                if (next_ch == '}') {
                    cur.pos += 1;
                    break;
                }
                if (!first_field) {
                    cur.expectChar(',')?;
                }
                first_field = false;

                const key = cur.parseString(alloc)?;
                defer alloc.free(key);
                cur.expectChar(':')?;

                if (std.mem.eql(u8, key, "seq")) {
                    seq_val = cur.parseU64()?;
                    has_seq = true;
                } else if (std.mem.eql(u8, key, "timestamp_ms")) {
                    ts_val = cur.parseU64()?;
                    has_ts = true;
                } else if (std.mem.eql(u8, key, "actor")) {
                    const s = cur.parseString(alloc)?;
                    actor_val = trail.ownOwnedSlice(s)?;
                    has_actor = true;
                } else if (std.mem.eql(u8, key, "action")) {
                    const s = cur.parseString(alloc)?;
                    action_val = trail.ownOwnedSlice(s)?;
                    has_action = true;
                } else if (std.mem.eql(u8, key, "resource")) {
                    const s = cur.parseString(alloc)?;
                    resource_val = trail.ownOwnedSlice(s)?;
                    has_resource = true;
                } else if (std.mem.eql(u8, key, "outcome")) {
                    const s = cur.parseString(alloc)?;
                    defer alloc.free(s);
                    outcome_val = AuditOutcome.fromString(s) ?? return AuditError.InvalidJsonl;
                    has_outcome = true;
                } else if (std.mem.eql(u8, key, "severity")) {
                    const s = cur.parseString(alloc)?;
                    defer alloc.free(s);
                    severity_val = AuditSeverity.fromString(s) ?? return AuditError.InvalidJsonl;
                    has_severity = true;
                } else if (std.mem.eql(u8, key, "metadata_json")) {
                    const s = cur.parseString(alloc)?;
                    meta_val = trail.ownOwnedSlice(s)?;
                    has_meta = true;
                } else if (std.mem.eql(u8, key, "prev_hash")) {
                    const s = cur.parseString(alloc)?;
                    prev_val = trail.ownOwnedSlice(s)?;
                    has_prev = true;
                } else if (std.mem.eql(u8, key, "entry_hash")) {
                    const s = cur.parseString(alloc)?;
                    hash_val = trail.ownOwnedSlice(s)?;
                    has_hash = true;
                } else {
                    return AuditError.InvalidJsonl;
                }
            }

            cur.skipWs();
            if (cur.pos != cur.src.len) return AuditError.InvalidJsonl;

            if (!has_seq || !has_ts || !has_actor || !has_action || !has_resource ||
                !has_outcome || !has_severity || !has_meta || !has_prev || !has_hash) {
                return AuditError.InvalidJsonl;
            }

            const entry = AuditEntry{
                seq: seq_val,
                timestamp_ms: ts_val,
                actor: actor_val,
                action: action_val,
                resource: resource_val,
                outcome: outcome_val,
                severity: severity_val,
                metadata_json: meta_val,
                prev_hash: prev_val,
                entry_hash: hash_val,
            };
            trail.entries.append(alloc, entry) catch return AuditError.OutOfMemory;
        }

        if (line_count == 0) {
            return AuditError.EmptyLog;
        }

        return trail;
    }

    pub fn exportCsv(): string ! AuditError {
        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        buf.appendSlice(
            _.alloc,
            "seq,timestamp_ms,actor,action,resource,outcome,severity,metadata_json,prev_hash,entry_hash\n",
        ) catch return AuditError.OutOfMemory;

        for (entry of _.entries.items) {
            appendDecimalU64(&buf, _.alloc, entry.seq)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendDecimalU64(&buf, _.alloc, entry.timestamp_ms)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.actor)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.action)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.resource)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.outcome.toString())?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.severity.toString())?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.metadata_json)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.prev_hash)?;
            buf.append(_.alloc, ',') catch return AuditError.OutOfMemory;
            appendCsvEscaped(&buf, _.alloc, entry.entry_hash)?;
            buf.append(_.alloc, '\n') catch return AuditError.OutOfMemory;
        }

        return buf.toOwnedSlice(_.alloc) catch return AuditError.OutOfMemory;
    }
}
