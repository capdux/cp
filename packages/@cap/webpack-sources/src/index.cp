// @cap/webpack-sources — Zero-heap source concatenation and range replacement engine.

pub struct SourceEdit {
    start: usize,
    end: usize,
    replacement: string,
}

fn wsrcWriteStr(out: []u8, pos: usize, s: string): usize {
    let cur = pos;
    let i: usize = 0;
    while (i < s.len && cur < out.len) {
        out[cur] = s[i];
        cur += 1;
        i += 1;
    }
    return cur;
}

fn wsrcEmptyEdit(): SourceEdit {
    return SourceEdit{
        start: 0,
        end: 0,
        replacement: "",
    };
}

pub struct ReplaceSource {
    original: string,
    edits: [64]SourceEdit,
    edit_count: usize,

    static fn init(original: string): ReplaceSource {
        let buf: [64]SourceEdit = undefined;
        let i: usize = 0;
        while (i < 64) {
            buf[i] = wsrcEmptyEdit();
            i += 1;
        }
        return ReplaceSource{
            original: original,
            edits: buf,
            edit_count: 0,
        };
    }

    fn replace(*_, start: usize, end: usize, replacement: string): bool {
        if (start > end || end > _.original.len || _.edit_count >= 64) {
            return false;
        }
        _.edits[_.edit_count] = SourceEdit{
            start: start,
            end: end,
            replacement: replacement,
        };
        _.edit_count += 1;
        return true;
    }

    fn insert(*_, pos: usize, text: string): bool {
        if (pos > _.original.len || _.edit_count >= 64) {
            return false;
        }
        _.edits[_.edit_count] = SourceEdit{
            start: pos,
            end: pos,
            replacement: text,
        };
        _.edit_count += 1;
        return true;
    }

    fn remove(*_, start: usize, end: usize): bool {
        return _.replace(start, end, "");
    }

    fn render(out: []u8): usize {
        const count = _.edit_count;
        if (count == 0) {
            return wsrcWriteStr(out, 0, _.original);
        }

        let sorted: [64]SourceEdit = _.edits;
        // Stable insertion sort by start offset ascending
        let i: usize = 1;
        while (i < count) {
            const key = sorted[i];
            let j = i;
            while (j > 0 && sorted[j - 1].start > key.start) {
                sorted[j] = sorted[j - 1];
                j -= 1;
            }
            sorted[j] = key;
            i += 1;
        }

        let pos: usize = 0;
        let cursor: usize = 0;
        let idx: usize = 0;
        while (idx < count) {
            const ed = sorted[idx];
            if (ed.start >= cursor) {
                if (ed.start > cursor) {
                    pos = wsrcWriteStr(out, pos, _.original[cursor..ed.start]);
                    cursor = ed.start;
                }
                pos = wsrcWriteStr(out, pos, ed.replacement);
                if (ed.end > cursor) {
                    cursor = ed.end;
                }
            }
            idx += 1;
        }

        if (cursor < _.original.len) {
            pos = wsrcWriteStr(out, pos, _.original[cursor.._.original.len]);
        }
        return pos;
    }
}

pub fn concatSources(parts: []const string, sep: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < parts.len) {
        if (i > 0 && sep.len > 0) {
            pos = wsrcWriteStr(out, pos, sep);
        }
        pos = wsrcWriteStr(out, pos, parts[i]);
        i += 1;
    }
    return pos;
}
