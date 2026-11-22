// Source: Absorbed from /data/dev/bun/src/watcher/WatcherTrace.cp | Synced: 2026-11-22T15:07:00Z
// Pure-CP zero-allocation Filesystem Watcher NDJSON Event Trace Formatter & Batch Deduplicator:
// - Formats watcher event batches into Bun `BUN_WATCHER_TRACE` NDJSON lines:
//   `{"timestamp":<ms>,"files":{"<path>":{"events":[...],"changed":[...]}}}\n`
// - Properly escapes arbitrary UTF-8 file paths and changed child names per RFC 8259 JSON
// - Omits `"changed"` key when no child names are recorded; falls back to `"(unknown)"` for empty paths
// - Supports per-path event deduplication and op-flag merging without heap allocation.

/// 单个追踪批次支持的最大文件事件数
pub const WATCHER_TRACE_MAX_EVENTS: usize = 32;
/// 单个目录事件支持的最大变更子文件名数
pub const WATCHER_TRACE_MAX_CHANGED_NAMES: usize = 16;
/// Bun 追踪输出环境变量名
pub const WATCHER_TRACE_ENV_VAR: []const u8 = "BUN_WATCHER_TRACE";

/// Watcher 追踪错误集
pub err WatcherTraceError {
    TooManyEvents,
    TooManyChangedNames,
    BufferTooSmall,
}

/// 文件系统监控事件操作位集合（对齐 Bun `Watcher.WatchEvent.Op`）
pub struct WatchTraceOpMask {
    delete: bool = false,
    metadata: bool = false,
    rename: bool = false,
    write: bool = false,
    move_to: bool = false,
    create: bool = false,
    access: bool = false,
    is_dir: bool = false,

    fn any(): bool {
        return _.delete || _.metadata || _.rename || _.write ||
            _.move_to || _.create || _.access || _.is_dir;
    }

    fn merge(other: WatchTraceOpMask): WatchTraceOpMask {
        return WatchTraceOpMask{
            delete: _.delete || other.delete,
            metadata: _.metadata || other.metadata,
            rename: _.rename || other.rename,
            write: _.write || other.write,
            move_to: _.move_to || other.move_to,
            create: _.create || other.create,
            access: _.access || other.access,
            is_dir: _.is_dir || other.is_dir,
        };
    }
}

/// 单个被监控路径的追踪事件条目
pub struct WatchTraceEntry {
    file_path: []const u8 = "",
    op: WatchTraceOpMask = WatchTraceOpMask{},
    changed: [WATCHER_TRACE_MAX_CHANGED_NAMES]([]const u8) = [""; WATCHER_TRACE_MAX_CHANGED_NAMES],
    changed_count: usize = 0,

    fn addChangedName(*_, name: []const u8): usize! {
        if (name.len == 0) return _.changed_count;
        // 避免同一批次内重复记录同名变更文件
        let i: usize = 0;
        while (i < _.changed_count) {
            if (bytesEql(_.changed[i], name)) {
                return _.changed_count;
            }
            i += 1;
        }
        if (_.changed_count >= WATCHER_TRACE_MAX_CHANGED_NAMES) {
            return err.TooManyChangedNames;
        }
        _.changed[_.changed_count] = name;
        _.changed_count += 1;
        return _.changed_count;
    }

    fn changedSlice(): []const ([]const u8) {
        return _.changed[0.._.changed_count];
    }
}

/// 单批次 NDJSON 追踪事件集合（支持按路径去重合并）
pub struct WatchTraceBatch {
    timestamp_ms: i64 = 0,
    entries: [WATCHER_TRACE_MAX_EVENTS]WatchTraceEntry = [WatchTraceEntry{}; WATCHER_TRACE_MAX_EVENTS],
    entry_count: usize = 0,

    static fn init(timestamp_ms: i64): WatchTraceBatch {
        return WatchTraceBatch{ timestamp_ms: timestamp_ms };
    }

    fn reset(*_, timestamp_ms: i64) {
        _.timestamp_ms = timestamp_ms;
        _.entry_count = 0;
    }

    fn entrySlice(): []const WatchTraceEntry {
        return _.entries[0.._.entry_count];
    }

    fn addEntry(*_, entry: WatchTraceEntry): usize! {
        if (_.entry_count >= WATCHER_TRACE_MAX_EVENTS) {
            return err.TooManyEvents;
        }
        _.entries[_.entry_count] = entry;
        _.entry_count += 1;
        return _.entry_count;
    }

    /// 按路径去重追加事件：若路径已存在则合并 `op` 标志并追加 `changed_name`
    fn addOrMergePath(
        *_,
        path: []const u8,
        op: WatchTraceOpMask,
        changed_name: ([]const u8)?,
    ): usize! {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (bytesEql(_.entries[i].file_path, path)) {
                _.entries[i].op = _.entries[i].op.merge(op);
                if (cname := changed_name) {
                    * = _.entries[i].addChangedName(cname)?;
                }
                return i;
            }
            i += 1;
        }
        if (_.entry_count >= WATCHER_TRACE_MAX_EVENTS) {
            return err.TooManyEvents;
        }
        let ent = WatchTraceEntry{ file_path: path, op: op };
        if (cname := changed_name) {
            * = ent.addChangedName(cname)?;
        }
        const idx = _.entry_count;
        _.entries[idx] = ent;
        _.entry_count += 1;
        return idx;
    }

    /// 序列化为单行 Bun `WatcherTrace` NDJSON 记录（以 `\n` 结尾）
    fn formatNdjsonLine(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "{\"timestamp\":")?;
        pos = appendI64Dec(out, pos, _.timestamp_ms)?;
        pos = appendBytes(out, pos, ",\"files\":{")?;

        let i: usize = 0;
        while (i < _.entry_count) {
            if (i > 0) {
                pos = appendChar(out, pos, ',')?;
            }
            const ent = _.entries[i];
            const display_path = if (ent.file_path.len > 0) ent.file_path else "(unknown)";
            pos = appendJsonQuoted(out, pos, display_path)?;
            pos = appendBytes(out, pos, ":{\"events\":[")?;
            pos = appendOpList(out, pos, ent.op)?;
            pos = appendChar(out, pos, ']')?;

            if (ent.changed_count > 0) {
                pos = appendBytes(out, pos, ",\"changed\":[")?;
                let j: usize = 0;
                while (j < ent.changed_count) {
                    if (j > 0) {
                        pos = appendChar(out, pos, ',')?;
                    }
                    pos = appendJsonQuoted(out, pos, ent.changed[j])?;
                    j += 1;
                }
                pos = appendChar(out, pos, ']')?;
            }

            pos = appendChar(out, pos, '}')?;
            i += 1;
        }

        pos = appendBytes(out, pos, "}}\n")?;
        return pos;
    }
}

/// 将字符串按 RFC 8259 转义为带双引号的 JSON 字符串字面量
pub fn formatJsonEscapedStr(s: []const u8, out: []u8): usize! {
    return appendJsonQuoted(out, 0, s);
}

// ----------------------------------------------------------------------------
// 内部零分配 JSON 与数字格式化辅助
// ----------------------------------------------------------------------------

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) return err.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendI64Dec(out: []u8, start_pos: usize, val: i64): usize! {
    let pos = start_pos;
    if (val == 0) {
        return appendChar(out, pos, '0');
    }
    let mag: u64 = 0;
    if (val < 0) {
        pos = appendChar(out, pos, '-')?;
        mag = @as(u64, @intCast(-(val + 1))) + 1;
    } else {
        mag = @as(u64, @intCast(val));
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    while (mag > 0) {
        tmp[n] = @as(u8, @truncate(mag % 10)) + '0';
        mag /= 10;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) {
        pos = appendChar(out, pos, tmp[n - 1 - i])?;
        i += 1;
    }
    return pos;
}

fn appendJsonQuoted(out: []u8, start_pos: usize, s: []const u8): usize! {
    let pos = appendChar(out, start_pos, '"')?;
    const hex: []const u8 = "0123456789abcdef";
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            pos = appendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            pos = appendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            pos = appendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            pos = appendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            pos = appendBytes(out, pos, "\\t")?;
        } else if (c == 0x08) {
            pos = appendBytes(out, pos, "\\b")?;
        } else if (c == 0x0c) {
            pos = appendBytes(out, pos, "\\f")?;
        } else if (c < 0x20) {
            pos = appendBytes(out, pos, "\\u00")?;
            pos = appendChar(out, pos, hex[@as(usize, c >> 4)])?;
            pos = appendChar(out, pos, hex[@as(usize, c & 0x0f)])?;
        } else {
            pos = appendChar(out, pos, c)?;
        }
        i += 1;
    }
    return appendChar(out, pos, '"');
}

fn appendOpTag(out: []u8, pos: usize, first: *bool, tag: []const u8): usize! {
    let p = pos;
    if (!first.*) {
        p = appendChar(out, p, ',')?;
    }
    first.* = false;
    p = appendChar(out, p, '"')?;
    p = appendBytes(out, p, tag)?;
    p = appendChar(out, p, '"')?;
    return p;
}

fn appendOpList(out: []u8, start_pos: usize, op: WatchTraceOpMask): usize! {
    let pos = start_pos;
    let first = true;
    if (op.delete) pos = appendOpTag(out, pos, *first, "delete")?;
    if (op.metadata) pos = appendOpTag(out, pos, *first, "metadata")?;
    if (op.rename) pos = appendOpTag(out, pos, *first, "rename")?;
    if (op.write) pos = appendOpTag(out, pos, *first, "write")?;
    if (op.move_to) pos = appendOpTag(out, pos, *first, "move_to")?;
    if (op.create) pos = appendOpTag(out, pos, *first, "create")?;
    if (op.access) pos = appendOpTag(out, pos, *first, "access")?;
    if (op.is_dir) pos = appendOpTag(out, pos, *first, "is_dir")?;
    return pos;
}
