// Source: Absorbed from /data/dev/bun/src/sourcemap/Mapping.cp
// Pure-CP zero-allocation SourceMap v3 VLQ `mappings` decoder and O(log N)
// binary-search original-position lookup index.

pub struct MappingEntry {
    gen_line: i32,
    gen_col: i32,
    src_idx: i32,
    orig_line: i32,
    orig_col: i32,
    /// -1 when no name is associated with this segment.
    name_idx: i32,
}

struct VlqDecodeRes {
    value: i32,
    next_pos: usize,
}

/// Zero-allocation SourceMap v3 mapping index (up to 256 decoded segments).
pub struct SourceMapIndex {
    entries: [256]MappingEntry,
    len: usize,

    pub static fn init(): SourceMapIndex {
        return SourceMapIndex{
            entries: [MappingEntry{
                gen_line: 0,
                gen_col: 0,
                src_idx: 0,
                orig_line: 0,
                orig_col: 0,
                name_idx: -1,
            }; 256],
            len: 0,
        };
    }

    /// Decode a standard SourceMap v3 VLQ `mappings` string (e.g. `"AAAA,IAAI;AACA"`).
    /// Returns null on malformed Base64-VLQ input or capacity overflow.
    pub static fn parseMappings(mappings: []const u8): SourceMapIndex? {
        let idx = SourceMapIndex.init();
        let gen_line: i32 = 0;
        let gen_col: i32 = 0;
        let src_idx: i32 = 0;
        let orig_line: i32 = 0;
        let orig_col: i32 = 0;
        let name_idx: i32 = 0;

        let pos: usize = 0;
        while (pos < mappings.len) {
            const c = mappings[pos];
            if (c == ';') {
                gen_line += 1;
                gen_col = 0;
                pos += 1;
                continue;
            }
            if (c == ',') {
                pos += 1;
                continue;
            }

            // Decode 1, 4, or 5 VLQ fields in this segment
            const f0 = decodeVlq(mappings, pos) ?? return null;
            gen_col += f0.value;
            pos = f0.next_pos;

            if (pos >= mappings.len || mappings[pos] == ',' || mappings[pos] == ';') {
                // 1-field segment (generated column only, no original source)
                continue;
            }

            const f1 = decodeVlq(mappings, pos) ?? return null;
            src_idx += f1.value;
            pos = f1.next_pos;

            const f2 = decodeVlq(mappings, pos) ?? return null;
            orig_line += f2.value;
            pos = f2.next_pos;

            const f3 = decodeVlq(mappings, pos) ?? return null;
            orig_col += f3.value;
            pos = f3.next_pos;

            let seg_name: i32 = -1;
            if (pos < mappings.len && mappings[pos] != ',' && mappings[pos] != ';') {
                const f4 = decodeVlq(mappings, pos) ?? return null;
                name_idx += f4.value;
                seg_name = name_idx;
                pos = f4.next_pos;
            }

            if (idx.len >= 256) return null;
            idx.entries[idx.len] = MappingEntry{
                gen_line: gen_line,
                gen_col: gen_col,
                src_idx: src_idx,
                orig_line: orig_line,
                orig_col: orig_col,
                name_idx: seg_name,
            };
            idx.len += 1;
        }
        return idx;
    }

    /// Binary-search the greatest mapping on `line` (0-based) with `gen_col <= col` (0-based),
    /// matching Bun's `Mapping.List.findIndexFromGenerated`.
    pub fn find(line: i32, col: i32): MappingEntry? {
        let count = _.len;
        let index: usize = 0;
        while (count > 0) {
            const step = count / 2;
            const i = index + step;
            const m = _.entries[i];
            if (m.gen_line < line || (m.gen_line == line && m.gen_col <= col)) {
                index = i + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }
        if (index > 0) {
            const found = _.entries[index - 1];
            if (found.gen_line == line) return found;
        }
        return null;
    }
}

fn decodeVlq(s: []const u8, start: usize): VlqDecodeRes? {
    let pos = start;
    let result: u32 = 0;
    let shift: u5 = 0;
    while (pos < s.len) {
        const digit = b64Digit(s[pos]) ?? return null;
        pos += 1;
        const cont = (digit & 32) != 0;
        const chunk: u32 = @as(u32, digit & 31);
        result |= chunk << shift;
        if (!cont) {
            const is_neg = (result & 1) != 0;
            const mag: i32 = @intCast(result >> 1);
            const signed_val = if (is_neg) 0 - mag else mag;
            return VlqDecodeRes{ value: signed_val, next_pos: pos };
        }
        if (shift >= 25) return null;
        shift += 5;
    }
    return null;
}

fn b64Digit(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}
