// Origin: Pure-CP Stdlib (AI Agent Prompt Template Renderer & Token-Budgeted Context Packer) | Created: 2026-11-22
// Zero-heap-allocation Prompt template renderer ({{key}}, {{key|fallback}}, \{\{ escape, strict/lenient)
// and token-budgeted conversation context window packer (pinned/System lock + newest-first greedy packing).

const std = @import("std");

/// Key-value variable binding for prompt template substitution.
pub struct TemplateVar {
    key: string = "";
    value: string = "";
}

/// Outcome status of `renderPromptTemplate`.
pub enum TemplateRenderStatus {
    Ok,
    MissingVariable,
    UnclosedPlaceholder,
    BufferTooSmall,
}

/// Result summary returned by `renderPromptTemplate`.
pub struct TemplateRenderResult {
    status: TemplateRenderStatus = TemplateRenderStatus.Ok;
    bytes_written: usize = 0;
    substitutions: usize = 0;
}

/// Message role for token-budgeted context packing.
pub enum PackRole {
    System,
    User,
    Assistant,
    Tool,
}

/// Lightweight message metadata used by `packContextWindow`.
pub struct PackableMessage {
    id: u32 = 0;
    role: PackRole = PackRole.User;
    token_count: u32 = 0;
    pinned: bool = false;
}

/// Packing summary returned by `packContextWindow`.
pub struct PackSummary {
    selected_count: usize = 0;
    dropped_count: usize = 0;
    used_tokens: u32 = 0;
    truncated_pinned: bool = false;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAsciiSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn findClosingBraces(tmpl: string, start_pos: usize): usize? {
    let j = start_pos;
    while (j + 1 < tmpl.len) : (j += 1) {
        if (tmpl[j] == '}' && tmpl[j + 1] == '}') {
            return j;
        }
    }
    return null;
}

fn findPipe(s: string): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '|') return i;
    }
    return null;
}

fn lookupVar(vars: []const TemplateVar, key: string): string? {
    let i: usize = 0;
    while (i < vars.len) : (i += 1) {
        if (std.mem.eql(u8, vars[i].key, key)) {
            return vars[i].value;
        }
    }
    return null;
}

fn appendBytes(out_buf: *[]u8, pos: usize, src: string): usize? {
    if (src.len > out_buf.len || pos > out_buf.len - src.len) {
        return null;
    }
    let k: usize = 0;
    while (k < src.len) : (k += 1) {
        out_buf[pos + k] = src[k];
    }
    return pos + src.len;
}

/// Zero-heap-allocation Prompt template renderer.
///
/// Supports:
/// - `{{key}}`: substituted with matching `TemplateVar.value` (whitespace around `key` trimmed).
/// - `{{key|fallback}}`: uses `fallback` if `key` is not present in `vars`.
/// - `\{\{` or `\{{`: emits literal `{{` without triggering placeholder substitution.
/// - `strict == true`: missing variable without fallback aborts with `MissingVariable`.
/// - `strict == false`: missing variable without fallback preserves original `{{...}}` verbatim.
pub fn renderPromptTemplate(
    tmpl: string,
    vars: []const TemplateVar,
    strict: bool,
    out_buf: *[]u8,
): TemplateRenderResult {
    let i: usize = 0;
    let pos: usize = 0;
    let subs: usize = 0;

    while (i < tmpl.len) {
        // Escape sequence `\{\{` -> literal `{{`
        if (i + 3 < tmpl.len && tmpl[i] == '\\' && tmpl[i + 1] == '{' && tmpl[i + 2] == '\\' && tmpl[i + 3] == '{') {
            if (next_pos := appendBytes(out_buf, pos, "{{")) {
                pos = next_pos;
                i += 4;
                continue;
            }
            return TemplateRenderResult{
                status: TemplateRenderStatus.BufferTooSmall,
                bytes_written: pos,
                substitutions: subs,
            };
        }

        // Escape sequence `\{{` -> literal `{{`
        if (i + 2 < tmpl.len && tmpl[i] == '\\' && tmpl[i + 1] == '{' && tmpl[i + 2] == '{') {
            if (next_pos := appendBytes(out_buf, pos, "{{")) {
                pos = next_pos;
                i += 3;
                continue;
            }
            return TemplateRenderResult{
                status: TemplateRenderStatus.BufferTooSmall,
                bytes_written: pos,
                substitutions: subs,
            };
        }

        // Placeholder `{{...}}`
        if (i + 1 < tmpl.len && tmpl[i] == '{' && tmpl[i + 1] == '{') {
            const close_opt = findClosingBraces(tmpl, i + 2);
            if (close_opt == null) {
                return TemplateRenderResult{
                    status: TemplateRenderStatus.UnclosedPlaceholder,
                    bytes_written: pos,
                    substitutions: subs,
                };
            }
            const close_idx = close_opt ?? 0;
            const inner = tmpl[i + 2 .. close_idx];

            let key: string = "";
            let has_fallback = false;
            let fallback: string = "";
            if (pipe_idx := findPipe(inner)) {
                key = trimAsciiSpaces(inner[0..pipe_idx]);
                has_fallback = true;
                fallback = inner[pipe_idx + 1 .. inner.len];
            } else {
                key = trimAsciiSpaces(inner);
            }

            if (key.len > 0) {
                if (val := lookupVar(vars, key)) {
                    if (next_pos := appendBytes(out_buf, pos, val)) {
                        pos = next_pos;
                        subs += 1;
                        i = close_idx + 2;
                        continue;
                    }
                    return TemplateRenderResult{
                        status: TemplateRenderStatus.BufferTooSmall,
                        bytes_written: pos,
                        substitutions: subs,
                    };
                }
            }

            if (has_fallback) {
                if (next_pos := appendBytes(out_buf, pos, fallback)) {
                    pos = next_pos;
                    subs += 1;
                    i = close_idx + 2;
                    continue;
                }
                return TemplateRenderResult{
                    status: TemplateRenderStatus.BufferTooSmall,
                    bytes_written: pos,
                    substitutions: subs,
                };
            }

            if (strict) {
                return TemplateRenderResult{
                    status: TemplateRenderStatus.MissingVariable,
                    bytes_written: pos,
                    substitutions: subs,
                };
            }

            // Lenient mode: preserve original placeholder slice verbatim.
            const raw_placeholder = tmpl[i .. close_idx + 2];
            if (next_pos := appendBytes(out_buf, pos, raw_placeholder)) {
                pos = next_pos;
                i = close_idx + 2;
                continue;
            }
            return TemplateRenderResult{
                status: TemplateRenderStatus.BufferTooSmall,
                bytes_written: pos,
                substitutions: subs,
            };
        }

        // Plain literal byte
        if (pos >= out_buf.len) {
            return TemplateRenderResult{
                status: TemplateRenderStatus.BufferTooSmall,
                bytes_written: pos,
                substitutions: subs,
            };
        }
        out_buf[pos] = tmpl[i];
        pos += 1;
        i += 1;
    }

    return TemplateRenderResult{
        status: TemplateRenderStatus.Ok,
        bytes_written: pos,
        substitutions: subs,
    };
}

/// Extracts all unique placeholder key names from `tmpl` into `out_keys` (zero-copy slices of `tmpl`).
/// Skips escaped `\{\{` and `\{{` sequences and strips `|fallback` defaults.
/// Returns the number of unique keys written, or `null` on unclosed `{{` or insufficient `out_keys` capacity.
pub fn extractTemplateKeys(tmpl: string, out_keys: *[]string): usize? {
    let i: usize = 0;
    let count: usize = 0;

    while (i < tmpl.len) {
        if (i + 3 < tmpl.len && tmpl[i] == '\\' && tmpl[i + 1] == '{' && tmpl[i + 2] == '\\' && tmpl[i + 3] == '{') {
            i += 4;
            continue;
        }
        if (i + 2 < tmpl.len && tmpl[i] == '\\' && tmpl[i + 1] == '{' && tmpl[i + 2] == '{') {
            i += 3;
            continue;
        }
        if (i + 1 < tmpl.len && tmpl[i] == '{' && tmpl[i + 1] == '{') {
            const close_opt = findClosingBraces(tmpl, i + 2);
            if (close_opt == null) return null;
            const close_idx = close_opt ?? 0;
            const inner = tmpl[i + 2 .. close_idx];

            let key: string = "";
            if (pipe_idx := findPipe(inner)) {
                key = trimAsciiSpaces(inner[0..pipe_idx]);
            } else {
                key = trimAsciiSpaces(inner);
            }

            if (key.len > 0) {
                let exists = false;
                let k: usize = 0;
                while (k < count) : (k += 1) {
                    if (std.mem.eql(u8, out_keys[k], key)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    if (count >= out_keys.len) return null;
                    out_keys[count] = key;
                    count += 1;
                }
            }
            i = close_idx + 2;
            continue;
        }
        i += 1;
    }

    return count;
}

fn siftDownMaxUsize(arr: *[]usize, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let largest = left;
        if (right < len && arr[right] > arr[largest]) {
            largest = right;
        }
        if (arr[idx] >= arr[largest]) break;
        const tmp = arr[idx];
        arr[idx] = arr[largest];
        arr[largest] = tmp;
        idx = largest;
    }
}

fn sortUsizeAsc(arr: *[]usize, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        siftDownMaxUsize(arr, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        siftDownMaxUsize(arr, 0, end);
    }
}

/// Zero-heap-allocation token-budgeted context window packer.
///
/// Packing algorithm:
/// 1. Computes `available_budget = max_tokens - reserve_output_tokens` (returns `null` if `reserve_output_tokens > max_tokens`).
/// 2. Locks and packs all `pinned` or `PackRole.System` messages first (sets `truncated_pinned = true` if any cannot fit).
/// 3. Greedily packs remaining non-pinned messages from newest to oldest within the remaining token budget.
/// 4. Sorts selected indices in-place into original chronological order in `out_indices`.
/// Returns `null` if `out_indices` lacks capacity for the selected messages or if budget is invalid.
pub fn packContextWindow(
    messages: []const PackableMessage,
    max_tokens: u32,
    reserve_output_tokens: u32,
    out_indices: *[]usize,
): PackSummary? {
    if (reserve_output_tokens > max_tokens) return null;
    const budget: u64 = (max_tokens - reserve_output_tokens) as u64;

    if (messages.len == 0) {
        return PackSummary{
            selected_count: 0,
            dropped_count: 0,
            used_tokens: 0,
            truncated_pinned: false,
        };
    }

    let selected: usize = 0;
    let used: u64 = 0;
    let truncated_pinned = false;

    // Pass 1: lock and include all pinned or System messages in order.
    let i: usize = 0;
    while (i < messages.len) : (i += 1) {
        const msg = messages[i];
        const is_locked = msg.pinned || msg.role == PackRole.System;
        if (!is_locked) continue;

        const tok: u64 = msg.token_count as u64;
        if (used + tok <= budget) {
            if (selected >= out_indices.len) return null;
            out_indices[selected] = i;
            selected += 1;
            used += tok;
        } else {
            truncated_pinned = true;
        }
    }

    // Pass 2: greedily include newest non-locked messages within remaining budget.
    let rev: usize = messages.len;
    while (rev > 0) {
        rev -= 1;
        const msg = messages[rev];
        const is_locked = msg.pinned || msg.role == PackRole.System;
        if (is_locked) continue;

        const tok: u64 = msg.token_count as u64;
        if (used + tok <= budget) {
            if (selected >= out_indices.len) return null;
            out_indices[selected] = rev;
            selected += 1;
            used += tok;
        }
    }

    // Pass 3: restore original chronological order in-place.
    sortUsizeAsc(out_indices, selected);

    return PackSummary{
        selected_count: selected,
        dropped_count: messages.len - selected,
        used_tokens: used as u32,
        truncated_pinned: truncated_pinned,
    };
}
