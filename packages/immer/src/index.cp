// @cap/immer — Zero-heap Immer immutable state producer, RFC 6902 patches & RFC 6901 JSON Pointer
// Clean-room MIT port of npm `immer` (`produce`, `produceWithPatches`, `applyPatches`, `freeze`, `isFrozen`).

pub const IMMER_MAX_ENTRIES: usize = 64;
pub const IMMER_MAX_PATCHES: usize = 64;

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

pub struct StateEntry {
    pub key: string,
    pub value: string,
}

fn immpStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn immpWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn immpWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn immpNormalizeKey(path: string): string {
    if (path.len > 0 && path[0] == '/') {
        return path[1..path.len];
    }
    return path;
}

fn immpEmptyPatch(): StatePatch {
    return StatePatch{ op: .Add, path: "", value: "", prev_value: "" };
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

/// Generates the full undo patch sequence in reverse order (`inversePatches`).
pub fn inversePatches(patches: []const StatePatch, out: []StatePatch): usize {
    let count: usize = 0;
    let i: usize = patches.len;
    while (i > 0 && count < out.len) {
        i -= 1;
        out[count] = invertPatch(patches[i]);
        count += 1;
    }
    return count;
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

/// Formats a `StatePatch` as an RFC 6902 JSON object into `out`.
pub fn formatPatchJson(p: StatePatch, out: []u8): usize {
    let pos: usize = 0;
    pos = immpWriteStr(out, pos, "{\"op\":\"");
    pos = immpWriteStr(out, pos, patchOpName(p.op));
    pos = immpWriteStr(out, pos, "\",\"path\":\"");
    pos = immpWriteStr(out, pos, p.path);
    pos = immpWriteByte(out, pos, '"');
    if (p.op != .Remove) {
        pos = immpWriteStr(out, pos, ",\"value\":\"");
        pos = immpWriteStr(out, pos, p.value);
        pos = immpWriteByte(out, pos, '"');
    }
    pos = immpWriteByte(out, pos, '}');
    return pos;
}

// --- Immer immutable state, freeze/isFrozen, Draft, produce, produceWithPatches, applyPatches ---

pub struct ImmerState {
    pub entries: [64]StateEntry,
    pub len: usize,
    pub frozen: bool,

    pub static fn new(): ImmerState {
        let arr: [64]StateEntry = undefined;
        let i: usize = 0;
        while (i < IMMER_MAX_ENTRIES) : (i += 1) {
            arr[i] = StateEntry { key: "", value: "" };
        }
        return ImmerState { entries: arr, len: 0, frozen: false };
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn get(key: string): string? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (immpStrEq(_.entries[i].key, key)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn has(key: string): bool {
        return _.get(key) != null;
    }

    /// Direct mutation helper that respects `frozen` immutability guard.
    pub fn setUnfrozen(*_, key: string, value: string): bool {
        if (_.frozen) { return false; }
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (immpStrEq(_.entries[i].key, key)) {
                _.entries[i].value = value;
                return true;
            }
        }
        if (_.len >= IMMER_MAX_ENTRIES) { return false; }
        _.entries[_.len] = StateEntry { key: key, value: value };
        _.len += 1;
        return true;
    }

    /// Direct removal helper that respects `frozen` immutability guard.
    pub fn removeUnfrozen(*_, key: string): bool {
        if (_.frozen) { return false; }
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (immpStrEq(_.entries[i].key, key)) {
                let j = i;
                while (j + 1 < _.len) : (j += 1) {
                    _.entries[j] = _.entries[j + 1];
                }
                _.len -= 1;
                return true;
            }
        }
        return false;
    }
}

/// Freezes `state` in place (mirrors `immer.freeze`), preventing direct mutations.
pub fn freeze(state: *ImmerState) {
    state.frozen = true;
}

/// Returns `true` if `state` is frozen (`immer.isFrozen`).
pub fn isFrozen(state: ImmerState): bool {
    return state.frozen;
}

pub struct Draft {
    pub state: ImmerState,
    pub patches: [64]StatePatch,
    pub patch_count: usize,

    pub static fn fromState(base: ImmerState): Draft {
        let copy = ImmerState { entries: base.entries, len: base.len, frozen: false };
        let p: [64]StatePatch = undefined;
        let i: usize = 0;
        while (i < IMMER_MAX_PATCHES) : (i += 1) {
            p[i] = immpEmptyPatch();
        }
        return Draft { state: copy, patches: p, patch_count: 0 };
    }

    pub fn get(key: string): string? {
        return _.state.get(key);
    }

    pub fn has(key: string): bool {
        return _.state.has(key);
    }

    pub fn set(*_, key: string, value: string): bool {
        const prev = _.state.get(key);
        if (prev != null) {
            if (immpStrEq(prev.?, value)) { return true; }
            if (!_.state.setUnfrozen(key, value)) { return false; }
            if (_.patch_count < IMMER_MAX_PATCHES) {
                _.patches[_.patch_count] = StatePatch {
                    op: .Replace,
                    path: key,
                    value: value,
                    prev_value: prev.?,
                };
                _.patch_count += 1;
            }
            return true;
        }
        if (!_.state.setUnfrozen(key, value)) { return false; }
        if (_.patch_count < IMMER_MAX_PATCHES) {
            _.patches[_.patch_count] = StatePatch {
                op: .Add,
                path: key,
                value: value,
                prev_value: "",
            };
            _.patch_count += 1;
        }
        return true;
    }

    pub fn remove(*_, key: string): bool {
        const prev = _.state.get(key);
        if (prev == null) { return false; }
        if (!_.state.removeUnfrozen(key)) { return false; }
        if (_.patch_count < IMMER_MAX_PATCHES) {
            _.patches[_.patch_count] = StatePatch {
                op: .Remove,
                path: key,
                value: "",
                prev_value: prev.?,
            };
            _.patch_count += 1;
        }
        return true;
    }

    pub fn finish(): ImmerState {
        return ImmerState { entries: _.state.entries, len: _.state.len, frozen: true };
    }
}

pub struct ProduceResult {
    pub next_state: ImmerState,
    pub patches: [64]StatePatch,
    pub patch_count: usize,
    pub inverse_patches: [64]StatePatch,
    pub inverse_count: usize,
}

/// Produces the next immutable (frozen) state by running `mutator` on a copy-on-write `Draft` (`immer.produce`).
pub fn produce(base: ImmerState, mutator: fn(*Draft): void): ImmerState {
    let draft = Draft.fromState(base);
    mutator(&draft);
    return draft.finish();
}

/// Produces the next immutable state together with forward `patches` and undo `inverse_patches` (`immer.produceWithPatches`).
pub fn produceWithPatches(base: ImmerState, mutator: fn(*Draft): void): ProduceResult {
    let draft = Draft.fromState(base);
    mutator(&draft);
    let inv: [64]StatePatch = undefined;
    let i: usize = 0;
    while (i < IMMER_MAX_PATCHES) : (i += 1) {
        inv[i] = immpEmptyPatch();
    }
    const inv_count = inversePatches(draft.patches[0..draft.patch_count], inv[0..IMMER_MAX_PATCHES]);
    return ProduceResult {
        next_state: draft.finish(),
        patches: draft.patches,
        patch_count: draft.patch_count,
        inverse_patches: inv,
        inverse_count: inv_count,
    };
}

/// Applies a slice of `StatePatch` operations to `base` and returns a new frozen `ImmerState` (`immer.applyPatches`).
pub fn applyPatches(base: ImmerState, patches: []const StatePatch): ImmerState {
    let next = ImmerState { entries: base.entries, len: base.len, frozen: false };
    let i: usize = 0;
    while (i < patches.len) : (i += 1) {
        const p = patches[i];
        const key = immpNormalizeKey(p.path);
        switch (p.op) {
            .Add => { * = next.setUnfrozen(key, p.value); },
            .Replace => { * = next.setUnfrozen(key, p.value); },
            .Remove => { * = next.removeUnfrozen(key); },
        }
    }
    next.frozen = true;
    return next;
}
