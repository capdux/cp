// @cap/is-plain-obj — Zero-heap plain object & prototype-safety validator.
// Clean-room MIT port of npm `is-plain-obj` v4 (140M+/week).

pub const PROTO_NULL: u8 = 0;
pub const PROTO_OBJECT: u8 = 1;
pub const PROTO_CUSTOM_CLASS: u8 = 2;
pub const PROTO_ARRAY: u8 = 3;
pub const PROTO_PRIMITIVE: u8 = 4;
pub const PROTO_FUNCTION: u8 = 5;

/// Lightweight descriptor of a runtime value's prototype and special traits.
pub struct ValueMeta {
    pub proto_kind: u8,
    pub has_to_string_tag: bool,
    pub is_iterator: bool,
    pub is_null: bool,

    pub static fn plain(): ValueMeta {
        return ValueMeta {
            proto_kind: PROTO_OBJECT,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
    }

    pub static fn nullProto(): ValueMeta {
        return ValueMeta {
            proto_kind: PROTO_NULL,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
    }
}

fn ipoIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn ipoSliceEq(s: string, start: usize, end: usize, target: string): bool {
    let seg_len = end - start;
    if (seg_len != target.len) {
        return false;
    }
    let i: usize = 0;
    while (i < seg_len) {
        if (s[start + i] != target[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ipoIsDangerousSlice(s: string, start: usize, end: usize): bool {
    if (ipoSliceEq(s, start, end, "__proto__")) {
        return true;
    }
    if (ipoSliceEq(s, start, end, "constructor")) {
        return true;
    }
    if (ipoSliceEq(s, start, end, "prototype")) {
        return true;
    }
    return false;
}

/// Returns `true` only when `m` represents a plain object:
/// prototype is `PROTO_NULL` or `PROTO_OBJECT`, and it is neither `null`,
/// nor tagged with `Symbol.toStringTag`, nor an iterator.
pub fn isPlainObjectMeta(m: ValueMeta): bool {
    if (m.is_null || m.has_to_string_tag || m.is_iterator) {
        return false;
    }
    return m.proto_kind == PROTO_NULL || m.proto_kind == PROTO_OBJECT;
}

/// Zero-heap check that `json_text` (after trimming ASCII whitespace) is a single
/// balanced JSON object literal `{...}`, excluding arrays, strings, primitives,
/// malformed braces/strings, and concatenated objects like `{}{}`.
pub fn isPlainJsonObject(json_text: string): bool {
    let start: usize = 0;
    let end: usize = json_text.len;

    while (start < end && ipoIsSpace(json_text[start])) {
        start = start + 1;
    }
    while (end > start && ipoIsSpace(json_text[end - 1])) {
        end = end - 1;
    }

    if (end - start < 2) {
        return false;
    }
    if (json_text[start] != 123 || json_text[end - 1] != 125) {
        return false;
    }

    // Verify first non-space token inside '{' is either '}' (empty object) or '"' (JSON key)
    let first_inner = start + 1;
    while (first_inner < end - 1 && ipoIsSpace(json_text[first_inner])) {
        first_inner = first_inner + 1;
    }
    if (first_inner < end - 1 && json_text[first_inner] != 34) {
        return false;
    }

    let in_str = false;
    let escaped = false;
    let brace_depth: i32 = 0;
    let bracket_depth: i32 = 0;
    let i = start;

    while (i < end) {
        let ch = json_text[i];
        if (in_str) {
            if (escaped) {
                escaped = false;
            } else if (ch == 92) {
                escaped = true;
            } else if (ch == 34) {
                in_str = false;
            }
        } else {
            if (ch == 34) {
                in_str = true;
            } else if (ch == 123) {
                brace_depth = brace_depth + 1;
            } else if (ch == 125) {
                brace_depth = brace_depth - 1;
                if (brace_depth < 0) {
                    return false;
                }
                if (brace_depth == 0 && i != end - 1) {
                    return false;
                }
            } else if (ch == 91) {
                bracket_depth = bracket_depth + 1;
            } else if (ch == 93) {
                bracket_depth = bracket_depth - 1;
                if (bracket_depth < 0) {
                    return false;
                }
            }
        }
        i = i + 1;
    }

    return !in_str && !escaped && brace_depth == 0 && bracket_depth == 0;
}

/// Returns `false` for prototype-pollution keys (`__proto__`, `constructor`, `prototype`),
/// and `true` for all other property keys.
pub fn isPlainObjectKey(key: string): bool {
    return !ipoIsDangerousSlice(key, 0, key.len);
}

/// Zero-heap linear scan of `json_text` detecting whether any JSON object key
/// (top-level or nested) is `__proto__`, `constructor`, or `prototype`.
/// String values matching those names are not flagged.
pub fn hasDangerousProtoKey(json_text: string): bool {
    let n = json_text.len;
    let i: usize = 0;

    while (i < n) {
        if (json_text[i] == 34) {
            let key_start = i + 1;
            let pos = key_start;
            let escaped = false;
            let closed = false;

            while (pos < n) {
                let ch = json_text[pos];
                if (escaped) {
                    escaped = false;
                } else if (ch == 92) {
                    escaped = true;
                } else if (ch == 34) {
                    closed = true;
                    break;
                }
                pos = pos + 1;
            }

            if (!closed) {
                return false;
            }

            let key_end = pos;
            let after = pos + 1;
            while (after < n && ipoIsSpace(json_text[after])) {
                after = after + 1;
            }

            if (after < n && json_text[after] == 58) {
                if (ipoIsDangerousSlice(json_text, key_start, key_end)) {
                    return true;
                }
            }

            i = pos + 1;
        } else {
            i = i + 1;
        }
    }

    return false;
}
