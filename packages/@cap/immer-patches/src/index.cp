// @cap/immer-patches — Zero-heap Immer / RFC 6902 State Patches & RFC 6901 JSON Pointer (clean-room CP)

pub enum PatchOp {
    Add,
    Replace,
    Remove,
}

pub struct StatePatch {
    op: PatchOp,
    path: string,
    value: string,
    prev_value: string,
}

fn immpWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

pub fn patchOpName(op: PatchOp): string {
    return switch (op) {
        .Add => "add",
        .Replace => "replace",
        .Remove => "remove",
    };
}

pub fn invertPatch(p: StatePatch): StatePatch {
    return switch (p.op) {
        .Add => StatePatch{
            op: .Remove,
            path: p.path,
            value: "",
            prev_value: p.value,
        },
        .Remove => StatePatch{
            op: .Add,
            path: p.path,
            value: p.prev_value,
            prev_value: "",
        },
        .Replace => StatePatch{
            op: .Replace,
            path: p.path,
            value: p.prev_value,
            prev_value: p.value,
        },
    };
}

pub fn formatJsonPointer(segments: []const string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < segments.len) : (i += 1) {
        pos = immpWriteByte(out, pos, '/');
        const seg = segments[i];
        let j: usize = 0;
        while (j < seg.len) : (j += 1) {
            const c = seg[j];
            if (c == '~') {
                pos = immpWriteByte(out, pos, '~');
                pos = immpWriteByte(out, pos, '0');
            } else if (c == '/') {
                pos = immpWriteByte(out, pos, '~');
                pos = immpWriteByte(out, pos, '1');
            } else {
                pos = immpWriteByte(out, pos, c);
            }
        }
    }
    return pos;
}

pub fn parseJsonPointer(ptr: string, out: []string): usize {
    if (ptr.len == 0 || ptr[0] != '/') {
        return 0;
    }
    let count: usize = 0;
    let start: usize = 1;
    let i: usize = 1;
    while (i <= ptr.len) : (i += 1) {
        if (i == ptr.len || ptr[i] == '/') {
            if (count < out.len) {
                out[count] = ptr[start..i];
                count += 1;
            }
            start = i + 1;
        }
    }
    return count;
}

pub fn unescapePointerToken(token: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < token.len) : (i += 1) {
        if (token[i] == '~' && i + 1 < token.len) {
            if (token[i + 1] == '0') {
                pos = immpWriteByte(out, pos, '~');
                i += 1;
            } else if (token[i + 1] == '1') {
                pos = immpWriteByte(out, pos, '/');
                i += 1;
            } else {
                pos = immpWriteByte(out, pos, token[i]);
            }
        } else {
            pos = immpWriteByte(out, pos, token[i]);
        }
    }
    return pos;
}
