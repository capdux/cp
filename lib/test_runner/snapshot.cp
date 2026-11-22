// Source: Absorbed from /data/dev/bun/src/test_runner/snapshot.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/test_runner/snapshot.cp — Bun-Style .snap Snapshot Store & Colored Diff
// ============================================================================

const std = @import("std");
import { exists, isDirectory, readFileSync, writeFileSync, mkdir } from "cap:fs";
import { PosixPath } from "cap:path";
import { DiffFormatOptions, DiffFormatter } from "cap:patch";

fn ensureDirTree(dir: []const u8) {
    if (dir.len == 0 || std.mem.eql(u8, dir, ".") || std.mem.eql(u8, dir, "/")) return;
    if (exists(dir)) return;
    let i: usize = if (dir[0] == '/') 1 else 0;
    while (i <= dir.len) : (i += 1) {
        if (i == dir.len || dir[i] == '/') {
            if (i > 0) {
                const sub = dir[0..i];
                if (!exists(sub)) {
                    mkdir(sub);
                }
            }
        }
    }
}

pub const SNAPSHOT_FILE_HEADER: []const u8 = "// Cap Snapshot v1, https://cp-lang.org/docs/test/snapshots\n";

pub enum SnapshotUpdateMode: u8 {
    Check = 0,
    Update = 1,
    CreateNew = 2,
}

pub enum SnapshotMatchStatus: u8 {
    Passed = 0,
    Added = 1,
    Updated = 2,
    Mismatched = 3,
    Missing = 4,
}

pub struct SnapshotMatchResult {
    status: SnapshotMatchStatus = .Passed,
    matched: bool = true,
    name: []const u8 = "",
    expected: []const u8 = "",
    actual: []const u8 = "",
    diff_report: []const u8 = "",
}

pub struct SnapshotEntry {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Escapes `\`, `` ` ``, and `${` for safe storage inside JS/Bun backtick template literals.
fn appendEscapedBacktickValue(
    allocator: std.mem.Allocator,
    out: *std.ArrayList(u8),
    raw: []const u8,
): ! {
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        const c = raw[i];
        if (c == '\\') {
            out.appendSlice(allocator, "\\\\")?;
        } else if (c == '`') {
            out.appendSlice(allocator, "\\`")?;
        } else if (c == '$' && i + 1 < raw.len && raw[i + 1] == '{') {
            out.appendSlice(allocator, "\\${")?;
            i += 1;
        } else {
            out.append(allocator, c)?;
        }
    }
}

/// Parses a backtick-delimited literal starting right after the opening `` ` `` at `src[start]`.
/// Returns `{ decoded_slice, next_pos_after_closing_backtick }` or `null` if unterminated.
struct ParsedBacktick {
    text: []u8 = "",
    next_pos: usize = 0,
}

fn parseBacktickLiteral(allocator: std.mem.Allocator, src: []const u8, start: usize): ParsedBacktick?! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    let i: usize = start;
    while (i < src.len) {
        const c = src[i];
        if (c == '`') {
            return ParsedBacktick{
                text: out.toOwnedSlice(allocator)?,
                next_pos: i + 1,
            };
        }
        if (c == '\\' && i + 1 < src.len) {
            const nxt = src[i + 1];
            if (nxt == '\\' || nxt == '`' || nxt == '$') {
                out.append(allocator, nxt)?;
                i += 2;
                continue;
            }
            if (nxt == 'n') {
                out.append(allocator, '\n')?;
                i += 2;
                continue;
            }
            if (nxt == 'r') {
                out.append(allocator, '\r')?;
                i += 2;
                continue;
            }
            if (nxt == 't') {
                out.append(allocator, '\t')?;
                i += 2;
                continue;
            }
        }
        out.append(allocator, c)?;
        i += 1;
    }
    out.deinit(allocator);
    return null;
}

/// Bun-compatible `.snap` snapshot store supporting `load`, `save`, `Check`/`Update`/`CreateNew`
/// verification, and character-level colored diff reporting via `cap:patch` `DiffFormatter`.
pub struct SnapshotStore {
    allocator: std.mem.Allocator,
    entries: std.ArrayList(SnapshotEntry) = .empty,
    index_map: std.StringHashMapUnmanaged(usize) = .empty,
    file_path: []const u8 = "",
    use_color: bool = true,
    total: usize = 0,
    passed: usize = 0,
    added: usize = 0,
    updated: usize = 0,
    failed: usize = 0,

    pub static fn init(allocator: std.mem.Allocator): SnapshotStore {
        return SnapshotStore{
            allocator: allocator,
        };
    }

    pub fn setColor(*_, use_color: bool) {
        _.use_color = use_color;
    }

    pub fn deinit(*_) {
        for (e of _.entries.items) {
            _.allocator.free(e.name);
            _.allocator.free(e.value);
        }
        _.entries.deinit(_.allocator);
        _.index_map.deinit(_.allocator);
        if (_.file_path.len > 0) _.allocator.free(_.file_path);
    }

    pub fn count(): usize {
        return _.entries.items.len;
    }

    pub fn get(name: []const u8): ([]const u8)? {
        if (idx := _.index_map.get(name)) {
            return _.entries.items[idx].value;
        }
        return null;
    }

    pub fn set(*_, name: []const u8, value: []const u8): ! {
        if (idx := _.index_map.get(name)) {
            const old_val = _.entries.items[idx].value;
            const val_dup = _.allocator.dupe(u8, value)?;
            _.allocator.free(old_val);
            _.entries.items[idx].value = val_dup;
            return;
        }
        const name_dup = _.allocator.dupe(u8, name)?;
        errdefer _.allocator.free(name_dup);
        const val_dup = _.allocator.dupe(u8, value)?;
        errdefer _.allocator.free(val_dup);
        const new_idx = _.entries.items.len;
        _.entries.append(_.allocator, SnapshotEntry{
            name: name_dup,
            value: val_dup,
        })?;
        _.index_map.put(_.allocator, name_dup, new_idx)?;
    }

    /// Parses `.snap` source text (`// Cap Snapshot v1` + `` exports[`name`] = `value`; ``).
    pub fn parseSource(*_, src: []const u8): ! {
        let pos: usize = 0;
        const marker = "exports[`";
        while (pos < src.len) {
            const found_opt = std.mem.indexOfPos(u8, src, pos, marker);
            const found = found_opt ?? break;
            const key_start = found + marker.len;
            const key_parsed = (parseBacktickLiteral(_.allocator, src, key_start)?) ?? break;
            defer _.allocator.free(key_parsed.text);

            let cur = key_parsed.next_pos;
            // Expect `] = ` with optional whitespace
            while (cur < src.len && src[cur] != '`') {
                cur += 1;
            }
            if (cur >= src.len) break;
            cur += 1; // skip opening backtick of value

            const val_parsed = (parseBacktickLiteral(_.allocator, src, cur)?) ?? break;
            defer _.allocator.free(val_parsed.text);

            _.set(key_parsed.text, val_parsed.text)?;
            pos = val_parsed.next_pos;
        }
    }

    /// Loads snapshots from `path` on disk. If the file does not yet exist, leaves store empty.
    pub fn load(*_, path: []const u8): ! {
        if (_.file_path.len > 0) _.allocator.free(_.file_path);
        _.file_path = _.allocator.dupe(u8, path)?;

        if (!exists(path) || isDirectory(path)) {
            return;
        }
        const raw = readFileSync(path);
        if (raw.len == 0) return;
        _.parseSource(raw)?;
    }

    /// Serializes all snapshots to `.snap` format and writes them to `path`.
    pub fn save(*_, path: []const u8): ! {
        const dir = PosixPath.dirname(path);
        if (dir.len > 0 && !std.mem.eql(u8, dir, ".")) {
            ensureDirTree(dir);
        }

        let buf: std.ArrayList(u8) = .empty;
        defer buf.deinit(_.allocator);

        buf.appendSlice(_.allocator, SNAPSHOT_FILE_HEADER)?;
        for (entry of _.entries.items) {
            buf.appendSlice(_.allocator, "\nexports[`")?;
            appendEscapedBacktickValue(_.allocator, &buf, entry.name)?;
            buf.appendSlice(_.allocator, "`] = `")?;
            appendEscapedBacktickValue(_.allocator, &buf, entry.value)?;
            buf.appendSlice(_.allocator, "`;\n")?;
        }

        writeFileSync(path, buf.items);
    }

    /// Matches `actual` against the stored snapshot `name` under `update_mode`.
    /// Generates a colored diff via `cap:patch` `DiffFormatter` on mismatch.
    pub fn matchSnapshot(
        *_,
        name: []const u8,
        actual: []const u8,
        update_mode: SnapshotUpdateMode,
    ): SnapshotMatchResult! {
        _.total += 1;

        if (expected := _.get(name)) {
            if (std.mem.eql(u8, expected, actual)) {
                _.passed += 1;
                return SnapshotMatchResult{
                    status: .Passed,
                    matched: true,
                    name: name,
                    expected: expected,
                    actual: actual,
                };
            }

            if (update_mode == .Update) {
                const prev_expected = _.allocator.dupe(u8, expected)?;
                _.set(name, actual)?;
                _.updated += 1;
                return SnapshotMatchResult{
                    status: .Updated,
                    matched: true,
                    name: name,
                    expected: prev_expected,
                    actual: actual,
                };
            }

            _.failed += 1;
            const diff_opts = DiffFormatOptions{
                use_color: _.use_color,
                context_lines: 3,
                show_line_numbers: true,
                expected_label: "Snapshot",
                received_label: "Received",
            };
            const diff_str = DiffFormatter.format(_.allocator, expected, actual, diff_opts)?;
            return SnapshotMatchResult{
                status: .Mismatched,
                matched: false,
                name: name,
                expected: expected,
                actual: actual,
                diff_report: diff_str,
            };
        }

        // Snapshot does not exist yet
        if (update_mode == .CreateNew || update_mode == .Update) {
            _.set(name, actual)?;
            _.added += 1;
            return SnapshotMatchResult{
                status: .Added,
                matched: true,
                name: name,
                expected: actual,
                actual: actual,
            };
        }

        _.failed += 1;
        return SnapshotMatchResult{
            status: .Missing,
            matched: false,
            name: name,
            expected: "",
            actual: actual,
            diff_report: std.fmt.allocPrint(_.allocator, "Snapshot `{s}` does not exist", .{name})?,
        };
    }
}
