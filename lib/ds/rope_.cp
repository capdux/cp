// Reference Only: Functional overlap with lib/ds/rope.cp | Kept for comparison/analysis
// Source: cp/lib/rope/piece_table.cp (Piece Table / Rope core engine with line/col mapping & undo/redo)
const std = @import("std");

pub enum RopeError {
    OutOfBounds,
    OutOfMemory,
}

pub struct LineCol {
    pub line: usize,
    pub col: usize,
}

pub enum BufferKind {
    Original,
    Add,
}

pub struct Piece {
    pub kind: BufferKind,
    pub start: usize,
    pub len: usize,
    pub line_breaks: usize,
}

struct UndoOp {
    offset: usize,
    deleted_text: []const u8,
    add_start: usize,
    inserted_len: usize,
}

fn countNewlines(bytes: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        if (bytes[i] == '\n') {
            count += 1;
        }
        i += 1;
    }
    return count;
}

fn findNthNewline(bytes: []const u8, n: usize): usize {
    let seen: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        if (bytes[i] == '\n') {
            if (seen == n) {
                return i;
            }
            seen += 1;
        }
        i += 1;
    }
    return bytes.len;
}

pub struct RopeText {
    alloc: std.mem.Allocator,
    original: []u8,
    add_buf: std.ArrayList(u8),
    pieces: std.ArrayList(Piece),
    total_len: usize,
    total_newlines: usize,
    undo_stack: std.ArrayList(UndoOp),
    redo_stack: std.ArrayList(UndoOp),

    pub fn initFromText(alloc: std.mem.Allocator, initial_text: []const u8): !RopeText {
        const orig = alloc.dupe(u8, initial_text) catch return RopeError.OutOfMemory;
        errdefer alloc.free(orig);

        let pieces = std.ArrayList(Piece).empty;
        errdefer pieces.deinit(alloc);

        const nl = countNewlines(orig);
        if (orig.len > 0) {
            pieces.append(alloc, Piece{
                .kind = BufferKind.Original,
                .start = 0,
                .len = orig.len,
                .line_breaks = nl,
            }) catch return RopeError.OutOfMemory;
        }

        return RopeText{
            .alloc = alloc,
            .original = orig,
            .add_buf = std.ArrayList(u8).empty,
            .pieces = pieces,
            .total_len = orig.len,
            .total_newlines = nl,
            .undo_stack = std.ArrayList(UndoOp).empty,
            .redo_stack = std.ArrayList(UndoOp).empty,
        };
    }

    pub fn deinit(*_) {
        _.clearStack(&_.undo_stack);
        _.clearStack(&_.redo_stack);
        _.undo_stack.deinit(_.alloc);
        _.redo_stack.deinit(_.alloc);
        _.pieces.deinit(_.alloc);
        _.add_buf.deinit(_.alloc);
        _.alloc.free(_.original);
    }

    fn clearStack(*_, stack: *std.ArrayList(UndoOp)) {
        let i: usize = 0;
        while (i < stack.items.len) {
            if (stack.items[i].deleted_text.len > 0) {
                _.alloc.free(stack.items[i].deleted_text);
            }
            i += 1;
        }
        stack.items.len = 0;
    }

    pub fn len(): usize {
        return _.total_len;
    }

    pub fn pieceCount(): usize {
        return _.pieces.items.len;
    }

    pub fn lineCount(): usize {
        return _.total_newlines + 1;
    }

    pub fn canUndo(): bool {
        return _.undo_stack.items.len > 0;
    }

    pub fn canRedo(): bool {
        return _.redo_stack.items.len > 0;
    }

    fn pieceBytes(p: Piece): []const u8 {
        if (p.kind == BufferKind.Original) {
            return _.original[p.start .. p.start + p.len];
        }
        return _.add_buf.items[p.start .. p.start + p.len];
    }

    pub fn byteAt(offset: usize): !u8 {
        if (offset >= _.total_len) {
            return RopeError.OutOfBounds;
        }
        let cur: usize = 0;
        let i: usize = 0;
        while (i < _.pieces.items.len) {
            const p = _.pieces.items[i];
            if (offset < cur + p.len) {
                const bytes = _.pieceBytes(p);
                return bytes[offset - cur];
            }
            cur += p.len;
            i += 1;
        }
        return RopeError.OutOfBounds;
    }

    pub fn slice(alloc: std.mem.Allocator, start: usize, end: usize): ![]u8 {
        if (start > end or end > _.total_len) {
            return RopeError.OutOfBounds;
        }
        const out_len = end - start;
        const out = alloc.alloc(u8, out_len) catch return RopeError.OutOfMemory;
        if (out_len == 0) {
            return out;
        }

        let written: usize = 0;
        let cur: usize = 0;
        let i: usize = 0;
        while (i < _.pieces.items.len and written < out_len) {
            const p = _.pieces.items[i];
            const p_end = cur + p.len;
            if (p_end > start and cur < end) {
                const copy_start = if (start > cur) start - cur else 0;
                const copy_end = if (end < p_end) end - cur else p.len;
                const bytes = _.pieceBytes(p);
                const chunk = bytes[copy_start..copy_end];
                @memcpy(out[written .. written + chunk.len], chunk);
                written += chunk.len;
            }
            cur = p_end;
            i += 1;
        }
        return out;
    }

    pub fn toString(alloc: std.mem.Allocator): ![]u8 {
        return _.slice(alloc, 0, _.total_len);
    }

    fn insertPieceRaw(*_, offset: usize, kind: BufferKind, start: usize, ins_len: usize, nl: usize): !void {
        if (ins_len == 0) {
            return;
        }
        if (offset == _.total_len and _.pieces.items.len > 0) {
            const last_idx = _.pieces.items.len - 1;
            const last = _.pieces.items[last_idx];
            if (last.kind == BufferKind.Add and kind == BufferKind.Add and last.start + last.len == start) {
                _.pieces.items[last_idx].len += ins_len;
                _.pieces.items[last_idx].line_breaks += nl;
                _.total_len += ins_len;
                _.total_newlines += nl;
                return;
            }
        }

        let cur: usize = 0;
        let pi: usize = 0;
        while (pi < _.pieces.items.len) {
            const p = _.pieces.items[pi];
            if (offset <= cur + p.len) {
                break;
            }
            cur += p.len;
            pi += 1;
        }

        const new_piece = Piece{
            .kind = kind,
            .start = start,
            .len = ins_len,
            .line_breaks = nl,
        };

        if (pi == _.pieces.items.len) {
            _.pieces.append(_.alloc, new_piece) catch return RopeError.OutOfMemory;
        } else {
            const p = _.pieces.items[pi];
            const inner = offset - cur;
            if (inner == 0) {
                if (pi > 0) {
                    const prev = _.pieces.items[pi - 1];
                    if (prev.kind == BufferKind.Add and kind == BufferKind.Add and prev.start + prev.len == start) {
                        _.pieces.items[pi - 1].len += ins_len;
                        _.pieces.items[pi - 1].line_breaks += nl;
                        _.total_len += ins_len;
                        _.total_newlines += nl;
                        return;
                    }
                }
                _.insertPieceAt(pi, new_piece) catch return RopeError.OutOfMemory;
            } else if (inner == p.len) {
                if (p.kind == BufferKind.Add and kind == BufferKind.Add and p.start + p.len == start) {
                    _.pieces.items[pi].len += ins_len;
                    _.pieces.items[pi].line_breaks += nl;
                    _.total_len += ins_len;
                    _.total_newlines += nl;
                    return;
                }
                _.insertPieceAt(pi + 1, new_piece) catch return RopeError.OutOfMemory;
            } else {
                const bytes = _.pieceBytes(p);
                const left_nl = countNewlines(bytes[0..inner]);
                const right_nl = p.line_breaks - left_nl;
                const left = Piece{
                    .kind = p.kind,
                    .start = p.start,
                    .len = inner,
                    .line_breaks = left_nl,
                };
                const right = Piece{
                    .kind = p.kind,
                    .start = p.start + inner,
                    .len = p.len - inner,
                    .line_breaks = right_nl,
                };
                _.pieces.items[pi] = left;
                _.insertPieceAt(pi + 1, new_piece) catch return RopeError.OutOfMemory;
                _.insertPieceAt(pi + 2, right) catch return RopeError.OutOfMemory;
            }
        }

        _.total_len += ins_len;
        _.total_newlines += nl;
    }

    fn insertPieceAt(*_, idx: usize, piece: Piece): !void {
        const old_len = _.pieces.items.len;
        _.pieces.resize(_.alloc, old_len + 1) catch return RopeError.OutOfMemory;
        let j: usize = old_len;
        while (j > idx) {
            _.pieces.items[j] = _.pieces.items[j - 1];
            j -= 1;
        }
        _.pieces.items[idx] = piece;
    }

    fn deleteRangeRaw(*_, start: usize, del_len: usize): !void {
        if (del_len == 0) {
            return;
        }
        const end = start + del_len;
        let next_pieces = std.ArrayList(Piece).empty;
        errdefer next_pieces.deinit(_.alloc);

        let cur: usize = 0;
        let i: usize = 0;
        let removed_nl: usize = 0;
        while (i < _.pieces.items.len) {
            const p = _.pieces.items[i];
            const p_end = cur + p.len;
            if (p_end <= start or cur >= end) {
                next_pieces.append(_.alloc, p) catch return RopeError.OutOfMemory;
            } else {
                const bytes = _.pieceBytes(p);
                if (cur < start) {
                    const keep_left = start - cur;
                    const left_nl = countNewlines(bytes[0..keep_left]);
                    next_pieces.append(_.alloc, Piece{
                        .kind = p.kind,
                        .start = p.start,
                        .len = keep_left,
                        .line_breaks = left_nl,
                    }) catch return RopeError.OutOfMemory;
                }
                if (p_end > end) {
                    const drop_end = end - cur;
                    const right_len = p_end - end;
                    const right_nl = countNewlines(bytes[drop_end..p.len]);
                    next_pieces.append(_.alloc, Piece{
                        .kind = p.kind,
                        .start = p.start + drop_end,
                        .len = right_len,
                        .line_breaks = right_nl,
                    }) catch return RopeError.OutOfMemory;
                }
                const ov_start = if (start > cur) start - cur else 0;
                const ov_end = if (end < p_end) end - cur else p.len;
                removed_nl += countNewlines(bytes[ov_start..ov_end]);
            }
            cur = p_end;
            i += 1;
        }

        _.pieces.deinit(_.alloc);
        _.pieces = next_pieces;
        _.total_len -= del_len;
        _.total_newlines -= removed_nl;
    }

    pub fn insert(*_, offset: usize, text: []const u8): !void {
        if (offset > _.total_len) {
            return RopeError.OutOfBounds;
        }
        if (text.len == 0) {
            return;
        }
        const add_start = _.add_buf.items.len;
        _.add_buf.appendSlice(_.alloc, text) catch return RopeError.OutOfMemory;
        const nl = countNewlines(text);
        try _.insertPieceRaw(offset, BufferKind.Add, add_start, text.len, nl);

        _.clearStack(&_.redo_stack);
        _.undo_stack.append(_.alloc, UndoOp{
            .offset = offset,
            .deleted_text = "",
            .add_start = add_start,
            .inserted_len = text.len,
        }) catch return RopeError.OutOfMemory;
    }

    pub fn deleteRange(*_, start: usize, del_len: usize): !void {
        if (start > _.total_len or del_len > _.total_len - start) {
            return RopeError.OutOfBounds;
        }
        if (del_len == 0) {
            return;
        }
        const deleted = try _.slice(_.alloc, start, start + del_len);
        errdefer _.alloc.free(deleted);

        try _.deleteRangeRaw(start, del_len);

        _.clearStack(&_.redo_stack);
        _.undo_stack.append(_.alloc, UndoOp{
            .offset = start,
            .deleted_text = deleted,
            .add_start = 0,
            .inserted_len = 0,
        }) catch return RopeError.OutOfMemory;
    }

    pub fn replaceRange(*_, start: usize, del_len: usize, text: []const u8): !void {
        if (start > _.total_len or del_len > _.total_len - start) {
            return RopeError.OutOfBounds;
        }
        if (del_len == 0 and text.len == 0) {
            return;
        }
        if (del_len == 0) {
            return _.insert(start, text);
        }
        if (text.len == 0) {
            return _.deleteRange(start, del_len);
        }

        const deleted = try _.slice(_.alloc, start, start + del_len);
        errdefer _.alloc.free(deleted);

        const add_start = _.add_buf.items.len;
        _.add_buf.appendSlice(_.alloc, text) catch return RopeError.OutOfMemory;
        const nl = countNewlines(text);

        try _.deleteRangeRaw(start, del_len);
        try _.insertPieceRaw(start, BufferKind.Add, add_start, text.len, nl);

        _.clearStack(&_.redo_stack);
        _.undo_stack.append(_.alloc, UndoOp{
            .offset = start,
            .deleted_text = deleted,
            .add_start = add_start,
            .inserted_len = text.len,
        }) catch return RopeError.OutOfMemory;
    }

    pub fn undo(*_): !bool {
        if (_.undo_stack.items.len == 0) {
            return false;
        }
        const op = _.undo_stack.items[_.undo_stack.items.len - 1];
        _.undo_stack.items.len -= 1;

        if (op.inserted_len > 0) {
            try _.deleteRangeRaw(op.offset, op.inserted_len);
        }
        if (op.deleted_text.len > 0) {
            const add_start = _.add_buf.items.len;
            _.add_buf.appendSlice(_.alloc, op.deleted_text) catch return RopeError.OutOfMemory;
            const nl = countNewlines(op.deleted_text);
            try _.insertPieceRaw(op.offset, BufferKind.Add, add_start, op.deleted_text.len, nl);
        }

        _.redo_stack.append(_.alloc, op) catch return RopeError.OutOfMemory;
        return true;
    }

    pub fn redo(*_): !bool {
        if (_.redo_stack.items.len == 0) {
            return false;
        }
        const op = _.redo_stack.items[_.redo_stack.items.len - 1];
        _.redo_stack.items.len -= 1;

        if (op.deleted_text.len > 0) {
            try _.deleteRangeRaw(op.offset, op.deleted_text.len);
        }
        if (op.inserted_len > 0) {
            const nl = countNewlines(_.add_buf.items[op.add_start .. op.add_start + op.inserted_len]);
            try _.insertPieceRaw(op.offset, BufferKind.Add, op.add_start, op.inserted_len, nl);
        }

        _.undo_stack.append(_.alloc, op) catch return RopeError.OutOfMemory;
        return true;
    }

    fn lineStartOffset(line_idx: usize): usize {
        if (line_idx == 0) {
            return 0;
        }
        let nl_seen: usize = 0;
        let cur: usize = 0;
        let i: usize = 0;
        while (i < _.pieces.items.len) {
            const p = _.pieces.items[i];
            if (nl_seen + p.line_breaks >= line_idx) {
                const need = line_idx - nl_seen - 1;
                const bytes = _.pieceBytes(p);
                const pos = findNthNewline(bytes, need);
                return cur + pos + 1;
            }
            nl_seen += p.line_breaks;
            cur += p.len;
            i += 1;
        }
        return _.total_len;
    }

    pub fn offsetToLineCol(offset: usize): !LineCol {
        if (offset > _.total_len) {
            return RopeError.OutOfBounds;
        }
        let line: usize = 0;
        let last_nl_offset: usize = 0;
        let has_nl = false;
        let cur: usize = 0;
        let i: usize = 0;
        while (i < _.pieces.items.len) {
            const p = _.pieces.items[i];
            if (cur >= offset) {
                break;
            }
            if (cur + p.len <= offset) {
                if (p.line_breaks > 0) {
                    line += p.line_breaks;
                    const bytes = _.pieceBytes(p);
                    const pos = findNthNewline(bytes, p.line_breaks - 1);
                    last_nl_offset = cur + pos;
                    has_nl = true;
                }
            } else {
                const limit = offset - cur;
                const bytes = _.pieceBytes(p);
                let j: usize = 0;
                while (j < limit) {
                    if (bytes[j] == '\n') {
                        line += 1;
                        last_nl_offset = cur + j;
                        has_nl = true;
                    }
                    j += 1;
                }
                break;
            }
            cur += p.len;
            i += 1;
        }
        const line_start = if (has_nl) last_nl_offset + 1 else 0;
        return LineCol{
            .line = line,
            .col = offset - line_start,
        };
    }

    pub fn lineColToOffset(line: usize, col: usize): !usize {
        if (line >= _.lineCount()) {
            return RopeError.OutOfBounds;
        }
        const start = _.lineStartOffset(line);
        const end = if (line + 1 < _.lineCount()) _.lineStartOffset(line + 1) - 1 else _.total_len;
        const line_len = end - start;
        if (col > line_len) {
            return RopeError.OutOfBounds;
        }
        return start + col;
    }

    pub fn getLine(alloc: std.mem.Allocator, line_idx: usize): ![]u8 {
        if (line_idx >= _.lineCount()) {
            return RopeError.OutOfBounds;
        }
        const start = _.lineStartOffset(line_idx);
        let end = if (line_idx + 1 < _.lineCount()) _.lineStartOffset(line_idx + 1) - 1 else _.total_len;
        if (end > start) {
            const prev_byte = try _.byteAt(end - 1);
            if (prev_byte == '\r') {
                end -= 1;
            }
        }
        return _.slice(alloc, start, end);
    }
}
