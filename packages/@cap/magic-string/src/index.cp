// @cap/magic-string — Zero-allocation original-offset source string editor in pure CP
// Clean-room MIT implementation inspired by npm `magic-string`.

pub const MAGIC_STRING_MAX_CHUNKS: usize = 64;
const MSTR_MAX_INSERTS: usize = 32;
const MSTR_MAX_ENDS: usize = 16;

struct MstrChunk {
    start: usize,
    end: usize,
    content: string,
    edited: bool,
}

struct MstrInsert {
    index: usize,
    content: string,
}

fn mstrWriteStr(src: string, out: []u8, pos: usize): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < src.len && w < out.len) : (i += 1) {
        out[w] = src[i];
        w += 1;
    }
    return w;
}

fn mstrWriteLeftInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize {
    let w: usize = pos;
    let i: usize = count;
    while (i > 0) {
        i -= 1;
        if (inserts[i].index == target_idx) {
            w = mstrWriteStr(inserts[i].content, out, w);
        }
    }
    return w;
}

fn mstrWriteRightInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (inserts[i].index == target_idx) {
            w = mstrWriteStr(inserts[i].content, out, w);
        }
    }
    return w;
}

/// Zero-heap source string editor operating on original character offsets (`magic-string` parity).
pub struct MagicString {
    original: string,
    changed: bool,
    chunks: [64]MstrChunk,
    chunk_count: usize,
    global_intros: [16]string,
    global_intro_len: usize,
    global_outros: [16]string,
    global_outro_len: usize,
    left_inserts: [32]MstrInsert,
    left_insert_len: usize,
    right_inserts: [32]MstrInsert,
    right_insert_len: usize,

    /// Create a new `MagicString` wrapping `original`.
    static fn new(original: string): MagicString {
        let ms: MagicString = MagicString {
            original: original,
            changed: false,
            chunks: undefined,
            chunk_count: 0,
            global_intros: undefined,
            global_intro_len: 0,
            global_outros: undefined,
            global_outro_len: 0,
            left_inserts: undefined,
            left_insert_len: 0,
            right_inserts: undefined,
            right_insert_len: 0,
        };
        if (original.len > 0) {
            ms.chunks[0] = MstrChunk {
                start: 0,
                end: original.len,
                content: original,
                edited: false,
            };
            ms.chunk_count = 1;
        }
        return ms;
    }

    /// Prepend `s` to the very start of the string (latest `prepend` appears first).
    fn prepend(*_, s: string): bool {
        if (s.len == 0) {
            return true;
        }
        if (_.global_intro_len >= MSTR_MAX_ENDS) {
            return false;
        }
        _.global_intros[_.global_intro_len] = s;
        _.global_intro_len += 1;
        _.changed = true;
        return true;
    }

    /// Append `s` to the very end of the string.
    fn append(*_, s: string): bool {
        if (s.len == 0) {
            return true;
        }
        if (_.global_outro_len >= MSTR_MAX_ENDS) {
            return false;
        }
        _.global_outros[_.global_outro_len] = s;
        _.global_outro_len += 1;
        _.changed = true;
        return true;
    }

    /// Split the chunk containing `split_at` (if strictly inside an unedited chunk).
    /// Returns `false` if `split_at` falls strictly inside an already-edited chunk or capacity is full.
    fn splitAt(*_, split_at: usize): bool {
        if (split_at == 0 || split_at >= _.original.len) {
            return true;
        }
        let i: usize = 0;
        while (i < _.chunk_count) : (i += 1) {
            const c: MstrChunk = _.chunks[i];
            if (split_at == c.start || split_at == c.end) {
                return true;
            }
            if (split_at > c.start && split_at < c.end) {
                if (c.edited) {
                    return false;
                }
                if (_.chunk_count >= MAGIC_STRING_MAX_CHUNKS) {
                    return false;
                }
                // Shift chunks right to insert the split tail at i + 1
                let k: usize = _.chunk_count;
                while (k > i + 1) {
                    _.chunks[k] = _.chunks[k - 1];
                    k -= 1;
                }
                _.chunks[i] = MstrChunk {
                    start: c.start,
                    end: split_at,
                    content: _.original[c.start..split_at],
                    edited: false,
                };
                _.chunks[i + 1] = MstrChunk {
                    start: split_at,
                    end: c.end,
                    content: _.original[split_at..c.end],
                    edited: false,
                };
                _.chunk_count += 1;
                return true;
            }
        }
        return true;
    }

    /// Overwrite original range `[start, end)` with `content`.
    /// Returns `false` if range is invalid, empty, or splits an already-edited chunk.
    fn overwrite(*_, start: usize, end: usize, content: string): bool {
        if (start >= end || end > _.original.len) {
            return false;
        }
        if (!_.splitAt(start)) {
            return false;
        }
        if (!_.splitAt(end)) {
            return false;
        }
        let first_idx: usize = _.chunk_count;
        let last_idx: usize = 0;
        let i: usize = 0;
        while (i < _.chunk_count) : (i += 1) {
            if (_.chunks[i].start == start) {
                first_idx = i;
            }
            if (_.chunks[i].end == end) {
                last_idx = i;
                break;
            }
        }
        if (first_idx >= _.chunk_count || last_idx < first_idx) {
            return false;
        }
        // Collapse chunks[first_idx..=last_idx] into a single edited chunk
        _.chunks[first_idx] = MstrChunk {
            start: start,
            end: end,
            content: content,
            edited: true,
        };
        const span_removed: usize = last_idx - first_idx;
        if (span_removed > 0) {
            let dst: usize = first_idx + 1;
            let src: usize = last_idx + 1;
            while (src < _.chunk_count) {
                _.chunks[dst] = _.chunks[src];
                dst += 1;
                src += 1;
            }
            _.chunk_count -= span_removed;
        }
        _.changed = true;
        return true;
    }

    /// Remove original range `[start, end)` (equivalent to `overwrite(start, end, "")`).
    fn remove(*_, start: usize, end: usize): bool {
        if (start == end && end <= _.original.len) {
            return true;
        }
        return _.overwrite(start, end, "");
    }

    /// Insert `content` immediately to the left of original offset `index`.
    fn prependLeft(*_, index: usize, content: string): bool {
        if (index > _.original.len) {
            return false;
        }
        if (content.len == 0) {
            return true;
        }
        if (_.left_insert_len >= MSTR_MAX_INSERTS) {
            return false;
        }
        if (!_.splitAt(index)) {
            return false;
        }
        _.left_inserts[_.left_insert_len] = MstrInsert {
            index: index,
            content: content,
        };
        _.left_insert_len += 1;
        _.changed = true;
        return true;
    }

    /// Insert `content` immediately to the right of original offset `index`.
    fn appendRight(*_, index: usize, content: string): bool {
        if (index > _.original.len) {
            return false;
        }
        if (content.len == 0) {
            return true;
        }
        if (_.right_insert_len >= MSTR_MAX_INSERTS) {
            return false;
        }
        if (!_.splitAt(index)) {
            return false;
        }
        _.right_inserts[_.right_insert_len] = MstrInsert {
            index: index,
            content: content,
        };
        _.right_insert_len += 1;
        _.changed = true;
        return true;
    }

    /// Returns `true` if any mutation has been applied to this `MagicString`.
    fn hasChanged(): bool {
        return _.changed;
    }

    /// Render the edited string into `out`. Returns the number of bytes written.
    fn writeTo(out: []u8): usize {
        let w: usize = 0;

        // 1. Global intros (reverse order: latest prepend first)
        let gi: usize = _.global_intro_len;
        while (gi > 0) {
            gi -= 1;
            w = mstrWriteStr(_.global_intros[gi], out, w);
        }

        // 2. Left inserts at index 0
        w = mstrWriteLeftInserts(_.left_inserts, _.left_insert_len, 0, out, w);

        // 3. Chunks and interior boundary inserts
        if (_.chunk_count == 0) {
            w = mstrWriteRightInserts(_.right_inserts, _.right_insert_len, 0, out, w);
        } else {
            let i: usize = 0;
            while (i < _.chunk_count) : (i += 1) {
                const c: MstrChunk = _.chunks[i];
                w = mstrWriteRightInserts(_.right_inserts, _.right_insert_len, c.start, out, w);
                w = mstrWriteStr(c.content, out, w);
                w = mstrWriteLeftInserts(_.left_inserts, _.left_insert_len, c.end, out, w);
            }
            w = mstrWriteRightInserts(_.right_inserts, _.right_insert_len, _.original.len, out, w);
        }

        // 4. Global outros (FIFO order)
        let go: usize = 0;
        while (go < _.global_outro_len) : (go += 1) {
            w = mstrWriteStr(_.global_outros[go], out, w);
        }

        return w;
    }

    /// Render the edited string into `out` and return the rendered `string` view `out[0..len]`.
    fn toString(out: []u8): string {
        const n: usize = _.writeTo(out);
        return out[0..n];
    }

    /// Render the slice corresponding to original offsets `[start, end)` into `out`.
    /// Returns the number of bytes written.
    fn slice(start: usize, end: usize, out: []u8): usize {
        if (start >= end || start >= _.original.len) {
            return 0;
        }
        let clamped_end: usize = end;
        if (clamped_end > _.original.len) {
            clamped_end = _.original.len;
        }
        let w: usize = 0;
        let i: usize = 0;
        while (i < _.chunk_count) : (i += 1) {
            const c: MstrChunk = _.chunks[i];
            if (c.end <= start) {
                continue;
            }
            if (c.start >= clamped_end) {
                break;
            }
            if (c.start >= start) {
                w = mstrWriteRightInserts(_.right_inserts, _.right_insert_len, c.start, out, w);
            }
            if (c.edited) {
                w = mstrWriteStr(c.content, out, w);
            } else {
                let lo: usize = c.start;
                if (start > lo) {
                    lo = start;
                }
                let hi: usize = c.end;
                if (clamped_end < hi) {
                    hi = clamped_end;
                }
                w = mstrWriteStr(_.original[lo..hi], out, w);
            }
            if (c.end <= clamped_end) {
                w = mstrWriteLeftInserts(_.left_inserts, _.left_insert_len, c.end, out, w);
            }
        }
        return w;
    }
}

export {
    MAGIC_STRING_MAX_CHUNKS,
    MagicString,
};
