// Origin: Pure-CP Stdlib (AI Streaming Tool-Call JSON Delta Assembler & Partial Closure) | Created: 2026-11-22
// Zero-heap-allocation streaming JSON tool-call delta assembler, partial JSON auto-closer,
// and incremental top-level string field extractor.

const MAX_JSON_NESTING: usize = 64;

/// Lifecycle state of `ToolStreamAssembler`.
pub enum ToolStreamState {
    Idle,
    Streaming,
    Complete,
    Overflow,
    Malformed,
}

fn isJsonSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// Zero-heap-allocation incremental assembler for streaming LLM tool-call JSON arguments.
pub struct ToolStreamAssembler {
    buf: []u8;
    len: usize = 0;
    stack: [64]u8 = [0; 64];
    depth: usize = 0;
    cur_state: ToolStreamState = ToolStreamState.Idle;
    in_string: bool = false;
    escape_next: bool = false;

    /// Initializes an assembler backed by caller-owned `buf`.
    static fn init(buf: *[]u8): ToolStreamAssembler {
        return ToolStreamAssembler{
            buf: buf,
            len: 0,
            stack: [0; 64],
            depth: 0,
            cur_state: ToolStreamState.Idle,
            in_string: false,
            escape_next: false,
        };
    }

    /// Returns the current assembler state.
    fn state(): ToolStreamState {
        return _.cur_state;
    }

    /// Returns `true` when a complete balanced JSON object or array has been assembled.
    fn isComplete(): bool {
        return _.cur_state == ToolStreamState.Complete;
    }

    /// Returns the assembled bytes collected so far as a zero-copy slice.
    fn payload(): string {
        return _.buf[0.._.len];
    }

    /// Returns the current active `{}`/`[]` nesting depth.
    fn nestingDepth(): usize {
        return _.depth;
    }

    /// Resets the assembler to `Idle` for reuse with the same backing buffer.
    fn reset(*_) {
        _.len = 0;
        _.depth = 0;
        _.cur_state = ToolStreamState.Idle;
        _.in_string = false;
        _.escape_next = false;
    }

    /// Feeds an arbitrary-length delta chunk into the assembler and returns the updated state.
    fn feedDelta(*_, chunk: []const u8): ToolStreamState {
        if (_.cur_state == ToolStreamState.Overflow || _.cur_state == ToolStreamState.Malformed) {
            return _.cur_state;
        }
        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            const b = chunk[i];

            if (_.cur_state == ToolStreamState.Complete) {
                if (isJsonSpace(b)) continue;
                _.cur_state = ToolStreamState.Malformed;
                return _.cur_state;
            }

            if (_.cur_state == ToolStreamState.Idle) {
                if (isJsonSpace(b)) continue;
                if (b != '{' && b != '[') {
                    _.cur_state = ToolStreamState.Malformed;
                    return _.cur_state;
                }
                if (_.len >= _.buf.len) {
                    _.cur_state = ToolStreamState.Overflow;
                    return _.cur_state;
                }
                _.buf[_.len] = b;
                _.len += 1;
                _.stack[0] = if (b == '{') '}' else ']';
                _.depth = 1;
                _.cur_state = ToolStreamState.Streaming;
                continue;
            }

            // Streaming state
            if (_.len >= _.buf.len) {
                _.cur_state = ToolStreamState.Overflow;
                return _.cur_state;
            }
            _.buf[_.len] = b;
            _.len += 1;

            if (_.in_string) {
                if (_.escape_next) {
                    _.escape_next = false;
                } else if (b == '\\') {
                    _.escape_next = true;
                } else if (b == '"') {
                    _.in_string = false;
                }
                continue;
            }

            if (b == '"') {
                _.in_string = true;
            } else if (b == '{' || b == '[') {
                if (_.depth >= MAX_JSON_NESTING) {
                    _.cur_state = ToolStreamState.Overflow;
                    return _.cur_state;
                }
                _.stack[_.depth] = if (b == '{') '}' else ']';
                _.depth += 1;
            } else if (b == '}' || b == ']') {
                if (_.depth == 0 || _.stack[_.depth - 1] != b) {
                    _.cur_state = ToolStreamState.Malformed;
                    return _.cur_state;
                }
                _.depth -= 1;
                if (_.depth == 0) {
                    _.cur_state = ToolStreamState.Complete;
                }
            }
        }
        return _.cur_state;
    }
}

fn appendBytes(out_buf: *[]u8, out_pos: *usize, text: string): bool {
    if (out_pos.* + text.len > out_buf.len) return false;
    let k: usize = 0;
    while (k < text.len) : (k += 1) {
        out_buf[out_pos.* + k] = text[k];
    }
    out_pos.* += text.len;
    return true;
}

fn appendByte(out_buf: *[]u8, out_pos: *usize, b: u8): bool {
    if (out_pos.* >= out_buf.len) return false;
    out_buf[out_pos.*] = b;
    out_pos.* += 1;
    return true;
}

fn isKeywordPrefix(slice: string, target: string): bool {
    if (slice.len == 0 || slice.len > target.len) return false;
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] != target[i]) return false;
    }
    return true;
}

/// Container sub-state for tracking whether an object is expecting a key, colon, or value.
const OBJ_EXPECT_KEY: u8 = 0;
const OBJ_EXPECT_COLON: u8 = 1;
const OBJ_EXPECT_VAL: u8 = 2;
const ARR_MODE: u8 = 3;

/// Auto-closes a truncated streaming JSON object or array into syntactically valid JSON.
/// Handles unclosed strings (including dangling trailing `\`), missing `:null` after object keys,
/// dangling commas/colons, truncated keyword literals (`true`/`false`/`null`), and nested `{}`/`[]`.
/// Returns the number of bytes written to `out_buf`, or `null` if invalid or `out_buf` is too small.
pub fn closePartialJson(partial: string, out_buf: *[]u8): usize? {
    let start: usize = 0;
    while (start < partial.len && isJsonSpace(partial[start])) : (start += 1) {}
    if (start >= partial.len) return null;
    if (partial[start] != '{' && partial[start] != '[') return null;

    let stack: [64]u8 = [0; 64];
    let mode_stack: [64]u8 = [0; 64];
    let depth: usize = 0;
    let in_string = false;
    let escape_next = false;
    let out_pos: usize = 0;

    let i = start;
    while (i < partial.len) : (i += 1) {
        const b = partial[i];
        if (depth == 0 && out_pos > 0) {
            if (isJsonSpace(b)) continue;
            return null;
        }
        if (!appendByte(out_buf, *out_pos, b)) return null;

        if (in_string) {
            if (escape_next) {
                escape_next = false;
            } else if (b == '\\') {
                escape_next = true;
            } else if (b == '"') {
                in_string = false;
                if (depth > 0 && mode_stack[depth - 1] == OBJ_EXPECT_KEY) {
                    mode_stack[depth - 1] = OBJ_EXPECT_COLON;
                }
            }
            continue;
        }

        if (b == '"') {
            in_string = true;
        } else if (b == '{') {
            if (depth >= MAX_JSON_NESTING) return null;
            stack[depth] = '}';
            mode_stack[depth] = OBJ_EXPECT_KEY;
            depth += 1;
        } else if (b == '[') {
            if (depth >= MAX_JSON_NESTING) return null;
            stack[depth] = ']';
            mode_stack[depth] = ARR_MODE;
            depth += 1;
        } else if (b == '}' || b == ']') {
            if (depth == 0 || stack[depth - 1] != b) return null;
            depth -= 1;
        } else if (b == ':') {
            if (depth > 0 && mode_stack[depth - 1] == OBJ_EXPECT_COLON) {
                mode_stack[depth - 1] = OBJ_EXPECT_VAL;
            }
        } else if (b == ',') {
            if (depth > 0 && stack[depth - 1] == '}') {
                mode_stack[depth - 1] = OBJ_EXPECT_KEY;
            }
        }
    }

    if (depth == 0 && !in_string) {
        return out_pos;
    }

    // 1. Close unclosed string literal
    if (in_string) {
        if (escape_next && out_pos > 0) {
            // Strip dangling backslash so closing quote is not escaped
            out_pos -= 1;
        }
        if (!appendByte(out_buf, *out_pos, '"')) return null;
        if (depth > 0 && mode_stack[depth - 1] == OBJ_EXPECT_KEY) {
            mode_stack[depth - 1] = OBJ_EXPECT_COLON;
        }
    } else {
        // 2. Clean up non-string trailing artifacts (whitespace, trailing comma, colon, partial token)
        while (out_pos > 0 && isJsonSpace(out_buf[out_pos - 1])) : (out_pos -= 1) {}
        if (out_pos > 0 && out_buf[out_pos - 1] == ',') {
            out_pos -= 1;
            while (out_pos > 0 && isJsonSpace(out_buf[out_pos - 1])) : (out_pos -= 1) {}
        } else if (out_pos > 0 && out_buf[out_pos - 1] == ':') {
            if (!appendBytes(out_buf, *out_pos, "null")) return null;
        } else if (out_pos > 0) {
            const last = out_buf[out_pos - 1];
            if (last == '.') {
                if (!appendByte(out_buf, *out_pos, '0')) return null;
            } else if (last == '-') {
                if (!appendByte(out_buf, *out_pos, '0')) return null;
            } else if ((last >= 'a' && last <= 'z')) {
                // Find start of trailing lowercase token
                let tok_start = out_pos;
                while (tok_start > 0 && out_buf[tok_start - 1] >= 'a' && out_buf[tok_start - 1] <= 'z') : (tok_start -= 1) {}
                const tok = out_buf[tok_start..out_pos];
                if (isKeywordPrefix(tok, "true") && tok.len < 4) {
                    const rem = "true"[tok.len..4];
                    if (!appendBytes(out_buf, *out_pos, rem)) return null;
                } else if (isKeywordPrefix(tok, "false") && tok.len < 5) {
                    const rem = "false"[tok.len..5];
                    if (!appendBytes(out_buf, *out_pos, rem)) return null;
                } else if (isKeywordPrefix(tok, "null") && tok.len < 4) {
                    const rem = "null"[tok.len..4];
                    if (!appendBytes(out_buf, *out_pos, rem)) return null;
                }
            }
        }
    }

    // 3. If top container is an object that just finished a key without ':value', append ':null'
    if (depth > 0 && stack[depth - 1] == '}' && mode_stack[depth - 1] == OBJ_EXPECT_COLON) {
        if (!appendBytes(out_buf, *out_pos, ":null")) return null;
    }

    // 4. Close all remaining open containers in LIFO order
    while (depth > 0) {
        depth -= 1;
        if (!appendByte(out_buf, *out_pos, stack[depth])) return null;
    }

    return out_pos;
}

fn matchJsonKeyAt(s: string, quote_pos: usize, target_key: string): usize? {
    if (quote_pos >= s.len || s[quote_pos] != '"') return null;
    let i = quote_pos + 1;
    let k: usize = 0;
    let matched = true;
    let esc = false;

    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (esc) {
            esc = false;
            if (k >= target_key.len || target_key[k] != b) matched = false;
            k += 1;
            continue;
        }
        if (b == '\\') {
            esc = true;
            continue;
        }
        if (b == '"') {
            if (matched && k == target_key.len) {
                return i + 1;
            }
            return null;
        }
        if (k >= target_key.len || target_key[k] != b) matched = false;
        k += 1;
    }
    return null;
}

fn skipJsonString(s: string, quote_pos: usize): usize {
    if (quote_pos >= s.len || s[quote_pos] != '"') return quote_pos;
    let i = quote_pos + 1;
    let esc = false;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (esc) {
            esc = false;
        } else if (b == '\\') {
            esc = true;
        } else if (b == '"') {
            return i + 1;
        }
    }
    return s.len;
}

fn skipJsonValue(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && isJsonSpace(s[i])) : (i += 1) {}
    if (i >= s.len) return s.len;

    if (s[i] == '"') {
        return skipJsonString(s, i);
    }
    if (s[i] == '{' || s[i] == '[') {
        let depth: usize = 0;
        let in_str = false;
        let esc = false;
        while (i < s.len) : (i += 1) {
            const b = s[i];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (b == '\\') {
                    esc = true;
                } else if (b == '"') {
                    in_str = false;
                }
            } else {
                if (b == '"') {
                    in_str = true;
                } else if (b == '{' || b == '[') {
                    depth += 1;
                } else if (b == '}' || b == ']') {
                    if (depth > 0) depth -= 1;
                    if (depth == 0) return i + 1;
                }
            }
        }
        return s.len;
    }
    // Primitive value (number, true, false, null)
    while (i < s.len && !isJsonSpace(s[i]) && s[i] != ',' && s[i] != '}' && s[i] != ']') : (i += 1) {}
    return i;
}

/// Incrementally extracts the decoded prefix of a top-level string field `key` from a partial
/// or complete streaming JSON object. Ignores nested fields with the same name.
/// Returns the number of decoded bytes written to `out_val`, or `null` if absent, non-string,
/// or `out_val` is too small.
pub fn peekStreamingStringField(partial: string, key: string, out_val: *[]u8): usize? {
    if (key.len == 0) return null;
    let i: usize = 0;
    while (i < partial.len && isJsonSpace(partial[i])) : (i += 1) {}
    if (i >= partial.len || partial[i] != '{') return null;
    i += 1;

    while (i < partial.len) {
        while (i < partial.len && (isJsonSpace(partial[i]) || partial[i] == ',')) : (i += 1) {}
        if (i >= partial.len || partial[i] == '}') return null;
        if (partial[i] != '"') return null;

        const key_matched_after = matchJsonKeyAt(partial, i, key);
        const after_key = key_matched_after ?? skipJsonString(partial, i);
        i = after_key;
        while (i < partial.len && isJsonSpace(partial[i])) : (i += 1) {}
        if (i >= partial.len || partial[i] != ':') return null;
        i += 1;
        while (i < partial.len && isJsonSpace(partial[i])) : (i += 1) {}
        if (i >= partial.len) return null;

        if (key_matched_after != null) {
            if (partial[i] != '"') return null;
            i += 1;
            let out_pos: usize = 0;
            let esc = false;
            while (i < partial.len) : (i += 1) {
                const b = partial[i];
                if (esc) {
                    esc = false;
                    const decoded: u8 = switch (b) {
                        'n' => '\n',
                        'r' => '\r',
                        't' => '\t',
                        '"' => '"',
                        '\\' => '\\',
                        '/' => '/',
                        'b' => 0x08,
                        'f' => 0x0C,
                        else => b,
                    };
                    if (!appendByte(out_val, *out_pos, decoded)) return null;
                } else if (b == '\\') {
                    esc = true;
                } else if (b == '"') {
                    return out_pos;
                } else {
                    if (!appendByte(out_val, *out_pos, b)) return null;
                }
            }
            // Partial string truncated mid-stream: return all decoded bytes arrived so far
            return out_pos;
        }

        // Skip non-matching value cleanly (including nested objects/arrays)
        i = skipJsonValue(partial, i);
    }
    return null;
}
