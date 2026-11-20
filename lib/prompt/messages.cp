// Source: cp/lib/prompt/messages.cp (LLM Context Window Manager & Protocol Serializer)
// Pure CP LLM context window manager with TokenEstimator-backed budget accounting,
// pinned system protection, orphaned tool-result pruning, and OpenAI/Anthropic JSON export.

const std = @import("std");
import { TokenEstimator } from "cap:text";

/// Error set shared across `cap:prompt` context window and extraction operations.
pub err PromptError {
    InvalidBudget,
    BudgetExceededByPinned,
    TagNotFound,
    JsonNotFound,
    OutOfMemory,
}

/// Conversation role for an LLM prompt message.
pub enum PromptRole: u8 {
    System = 0,
    Developer = 1,
    User = 2,
    Assistant = 3,
    Tool = 4,

    /// Returns the lowercase wire protocol role string (`"system"`, `"developer"`, `"user"`, `"assistant"`, `"tool"`).
    fn roleStr(): string {
        switch (_) {
            .System => return "system",
            .Developer => return "developer",
            .User => return "user",
            .Assistant => return "assistant",
            .Tool => return "tool",
        }
    }
}

/// Strategy used by `ContextWindow.fitToBudget` when pruning messages to satisfy token limits.
pub enum TrimStrategy: u8 {
    DropOldest = 0,
    KeepHeadAndTail = 1,
}

/// Owned chat message stored inside a `ContextWindow`.
pub struct PromptMessage {
    role: PromptRole = PromptRole.User,
    content: string = "",
    name: string = "",
    tool_call_id: string = "",
    pinned: bool = false,
    token_count: usize = 0,
}

/// Summary statistics returned by `ContextWindow.fitToBudget`.
pub struct WindowStats {
    total_messages: usize = 0,
    kept_messages: usize = 0,
    dropped_messages: usize = 0,
    used_tokens: usize = 0,
    available_tokens: usize = 0,
}

const MSG_PROTOCOL_OVERHEAD_TOKENS: usize = 4;

fn dupOwnedSlice(alloc: std.mem.Allocator, s: string): string ! PromptError {
    if (s.len == 0) return "";
    const copy = alloc.dupe(u8, s) catch return PromptError.OutOfMemory;
    return copy;
}

fn freeOwnedSlice(alloc: std.mem.Allocator, s: string) {
    if (s.len > 0) {
        alloc.free(s);
    }
}

fn freePromptMessage(alloc: std.mem.Allocator, msg: PromptMessage) {
    freeOwnedSlice(alloc, msg.content);
    freeOwnedSlice(alloc, msg.name);
    freeOwnedSlice(alloc, msg.tool_call_id);
}

fn estimateMessageTokens(content: string, name: string): usize {
    let total: usize = MSG_PROTOCOL_OVERHEAD_TOKENS + TokenEstimator.estimateTokens(content);
    if (name.len > 0) {
        total += TokenEstimator.estimateTokens(name);
    }
    return total;
}

fn appendJsonEscapedString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: string): void ! PromptError {
    out.append(alloc, '"') catch return PromptError.OutOfMemory;
    let i: usize = 0;
    let flush_start: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c < 0x20) {
            if (i > flush_start) {
                out.appendSlice(alloc, s[flush_start..i]) catch return PromptError.OutOfMemory;
            }
            switch (c) {
                '"' => out.appendSlice(alloc, "\\\"") catch return PromptError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return PromptError.OutOfMemory,
                0x08 => out.appendSlice(alloc, "\\b") catch return PromptError.OutOfMemory,
                0x0C => out.appendSlice(alloc, "\\f") catch return PromptError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return PromptError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return PromptError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return PromptError.OutOfMemory,
                else => {
                    const hex = "0123456789abcdef";
                    let seq: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
                    out.appendSlice(alloc, seq[0..6]) catch return PromptError.OutOfMemory;
                },
            }
            flush_start = i + 1;
        }
    }
    if (i > flush_start) {
        out.appendSlice(alloc, s[flush_start..i]) catch return PromptError.OutOfMemory;
    }
    out.append(alloc, '"') catch return PromptError.OutOfMemory;
}

/// Manages an LLM conversation window under a strict token budget with pinned-message
/// protection, orphaned tool-result cleanup, and OpenAI / Anthropic JSON serialization.
pub struct ContextWindow {
    allocator: std.mem.Allocator = undefined,
    max_tokens: usize = 0,
    reserve_output_tokens: usize = 0,
    messages: std.ArrayList(PromptMessage) = .empty,

    /// Initializes a `ContextWindow` with `max_tokens` total window and `reserve_output_tokens` output reserve.
    /// Returns `PromptError.InvalidBudget` if `max_tokens == 0` or `reserve_output_tokens >= max_tokens`.
    static fn init(
        alloc: std.mem.Allocator,
        max_tokens: usize,
        reserve_output_tokens: usize,
    ): ContextWindow ! PromptError {
        if (max_tokens == 0 || reserve_output_tokens >= max_tokens) {
            return PromptError.InvalidBudget;
        }
        return ContextWindow{
            allocator: alloc,
            max_tokens: max_tokens,
            reserve_output_tokens: reserve_output_tokens,
            messages: .empty,
        };
    }

    /// Releases all owned message strings and backing buffer.
    fn deinit(*_) {
        _.clear();
        _.messages.deinit(_.allocator);
    }

    /// Removes and frees all messages currently stored in the window.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.messages.items.len) : (i += 1) {
            freePromptMessage(_.allocator, _.messages.items[i]);
        }
        _.messages.items.len = 0;
    }

    /// Returns the maximum input token budget (`max_tokens - reserve_output_tokens`).
    fn inputBudget(): usize {
        if (_.max_tokens <= _.reserve_output_tokens) return 0;
        return _.max_tokens - _.reserve_output_tokens;
    }

    /// Returns the sum of `token_count` across all messages currently in the window.
    fn usedTokens(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.messages.items.len) : (i += 1) {
            total += _.messages.items[i].token_count;
        }
        return total;
    }

    /// Returns the number of messages currently in the window.
    fn messageCount(): usize {
        return _.messages.items.len;
    }

    /// Returns a read-only slice view of all messages currently in the window.
    fn items(): []const PromptMessage {
        return _.messages.items;
    }

    fn appendOwnedMessage(
        *_,
        role: PromptRole,
        content: string,
        name: string,
        tool_call_id: string,
        pinned: bool,
    ): void ! PromptError {
        const owned_content = dupOwnedSlice(_.allocator, content)?;
        errdefer freeOwnedSlice(_.allocator, owned_content);
        const owned_name = dupOwnedSlice(_.allocator, name)?;
        errdefer freeOwnedSlice(_.allocator, owned_name);
        const owned_call_id = dupOwnedSlice(_.allocator, tool_call_id)?;
        errdefer freeOwnedSlice(_.allocator, owned_call_id);

        const tokens = estimateMessageTokens(owned_content, owned_name);
        _.messages.append(_.allocator, PromptMessage{
            role: role,
            content: owned_content,
            name: owned_name,
            tool_call_id: owned_call_id,
            pinned: pinned,
            token_count: tokens,
        }) catch return PromptError.OutOfMemory;
    }

    /// Appends a `System` message with `pinned = true` by default.
    fn addSystem(*_, content: string): void ! PromptError {
        return _.appendOwnedMessage(PromptRole.System, content, "", "", true);
    }

    /// Appends a `Developer` message with `pinned = true` by default.
    fn addDeveloper(*_, content: string): void ! PromptError {
        return _.appendOwnedMessage(PromptRole.Developer, content, "", "", true);
    }

    /// Appends a `User` message (`pinned = false`).
    fn addUser(*_, content: string): void ! PromptError {
        return _.appendOwnedMessage(PromptRole.User, content, "", "", false);
    }

    /// Appends an `Assistant` message (`pinned = false`).
    fn addAssistant(*_, content: string): void ! PromptError {
        return _.appendOwnedMessage(PromptRole.Assistant, content, "", "", false);
    }

    /// Appends a `Tool` result message with `tool_call_id` and tool `name` (`pinned = false`).
    fn addToolResult(*_, tool_call_id: string, name: string, content: string): void ! PromptError {
        return _.appendOwnedMessage(PromptRole.Tool, content, name, tool_call_id, false);
    }

    /// Appends a message with explicit `role`, `content`, and `pinned` protection flag.
    fn addMessage(*_, role: PromptRole, content: string, pinned: bool): void ! PromptError {
        return _.appendOwnedMessage(role, content, "", "", pinned);
    }

    /// Marks `idx` as dropped and cascades to any immediately following unpinned `Tool`
    /// results when an `Assistant` message is dropped.
    fn dropWithToolCascade(
        *_,
        keep: []bool,
        idx: usize,
        cur_tokens: *usize,
    ) {
        if (!keep[idx]) return;
        keep[idx] = false;
        const msg = _.messages.items[idx];
        if (cur_tokens.* >= msg.token_count) {
            cur_tokens.* -= msg.token_count;
        } else {
            cur_tokens.* = 0;
        }
        if (msg.role == PromptRole.Assistant) {
            let j: usize = idx + 1;
            while (j < _.messages.items.len) : (j += 1) {
                const next_msg = _.messages.items[j];
                if (next_msg.role != PromptRole.Tool) break;
                if (keep[j] && !next_msg.pinned) {
                    keep[j] = false;
                    if (cur_tokens.* >= next_msg.token_count) {
                        cur_tokens.* -= next_msg.token_count;
                    } else {
                        cur_tokens.* = 0;
                    }
                }
            }
        }
    }

    /// Removes any unpinned `Tool` message whose immediate non-Tool predecessor among kept
    /// messages is not an `Assistant` message.
    fn pruneOrphanedTools(*_, keep: []bool, cur_tokens: *usize) {
        let expecting_tool = false;
        let i: usize = 0;
        while (i < _.messages.items.len) : (i += 1) {
            if (!keep[i]) continue;
            const msg = _.messages.items[i];
            switch (msg.role) {
                .Assistant => {
                    expecting_tool = true;
                },
                .Tool => {
                    if (!expecting_tool && !msg.pinned) {
                        keep[i] = false;
                        if (cur_tokens.* >= msg.token_count) {
                            cur_tokens.* -= msg.token_count;
                        } else {
                            cur_tokens.* = 0;
                        }
                    }
                },
                else => {
                    expecting_tool = false;
                },
            }
        }
    }

    /// Trims unpinned messages according to `strategy` so `used_tokens <= max_tokens - reserve_output_tokens`.
    /// Pinned messages are never dropped; returns `PromptError.BudgetExceededByPinned` if pinned messages alone
    /// exceed the available input budget. Automatically removes orphaned `Tool` messages whose preceding
    /// `Assistant` turn was trimmed.
    fn fitToBudget(*_, strategy: TrimStrategy): WindowStats ! PromptError {
        const budget = _.inputBudget();
        const total = _.messages.items.len;

        let pinned_tokens: usize = 0;
        let cur_tokens: usize = 0;
        let i: usize = 0;
        while (i < total) : (i += 1) {
            const tc = _.messages.items[i].token_count;
            cur_tokens += tc;
            if (_.messages.items[i].pinned) {
                pinned_tokens += tc;
            }
        }

        if (pinned_tokens > budget) {
            return PromptError.BudgetExceededByPinned;
        }

        if (total == 0) {
            return WindowStats{
                total_messages: 0,
                kept_messages: 0,
                dropped_messages: 0,
                used_tokens: 0,
                available_tokens: budget,
            };
        }

        let keep = _.allocator.alloc(bool, total) catch return PromptError.OutOfMemory;
        defer _.allocator.free(keep);
        i = 0;
        while (i < total) : (i += 1) {
            keep[i] = true;
        }

        if (cur_tokens > budget) {
            switch (strategy) {
                .DropOldest => {
                    let idx: usize = 0;
                    while (idx < total && cur_tokens > budget) : (idx += 1) {
                        if (!keep[idx] || _.messages.items[idx].pinned) continue;
                        _.dropWithToolCascade(keep, idx, *cur_tokens);
                    }
                },
                .KeepHeadAndTail => {
                    // Locate unpinned message indices in chronological order.
                    let unpinned_indices: std.ArrayList(usize) = .empty;
                    defer unpinned_indices.deinit(_.allocator);
                    let k: usize = 0;
                    while (k < total) : (k += 1) {
                        if (!_.messages.items[k].pinned) {
                            unpinned_indices.append(_.allocator, k) catch return PromptError.OutOfMemory;
                        }
                    }

                    const u_len = unpinned_indices.items.len;
                    if (u_len >= 3) {
                        // Drop middle turns (u[1..u_len-1]) oldest-first, preserving head u[0] and recent tail.
                        let pos: usize = 1;
                        while (pos + 1 < u_len && cur_tokens > budget) : (pos += 1) {
                            const msg_idx = unpinned_indices.items[pos];
                            if (!keep[msg_idx]) continue;
                            _.dropWithToolCascade(keep, msg_idx, *cur_tokens);
                        }
                    }
                    // If still over budget, drop head (u[0]) before touching the newest tail.
                    if (cur_tokens > budget && u_len >= 1) {
                        const head_idx = unpinned_indices.items[0];
                        if (keep[head_idx]) {
                            _.dropWithToolCascade(keep, head_idx, *cur_tokens);
                        }
                    }
                    // Finally, if the single newest tail message still overflows, drop it too.
                    if (cur_tokens > budget && u_len >= 2) {
                        const tail_idx = unpinned_indices.items[u_len - 1];
                        if (keep[tail_idx]) {
                            _.dropWithToolCascade(keep, tail_idx, *cur_tokens);
                        }
                    }
                },
            }

            // Final structural pass to guarantee zero orphaned Tool messages remain.
            _.pruneOrphanedTools(keep, *cur_tokens);
        }

        // Compact kept messages in-place and free dropped messages.
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        let final_tokens: usize = 0;
        while (read_idx < total) : (read_idx += 1) {
            if (keep[read_idx]) {
                _.messages.items[write_idx] = _.messages.items[read_idx];
                final_tokens += _.messages.items[write_idx].token_count;
                write_idx += 1;
            } else {
                freePromptMessage(_.allocator, _.messages.items[read_idx]);
            }
        }
        _.messages.items.len = write_idx;

        const avail: usize = if (budget >= final_tokens) budget - final_tokens else 0;
        return WindowStats{
            total_messages: total,
            kept_messages: write_idx,
            dropped_messages: total - write_idx,
            used_tokens: final_tokens,
            available_tokens: avail,
        };
    }

    /// Exports messages as standard OpenAI Chat Completions JSON: `{"messages":[...]}`.
    /// Caller owns the returned slice and must free it with the window's allocator.
    fn toOpenAiJson(): ([]u8) ! PromptError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        out.appendSlice(_.allocator, "{\"messages\":[") catch return PromptError.OutOfMemory;
        let i: usize = 0;
        while (i < _.messages.items.len) : (i += 1) {
            if (i > 0) {
                out.append(_.allocator, ',') catch return PromptError.OutOfMemory;
            }
            const msg = _.messages.items[i];
            out.appendSlice(_.allocator, "{\"role\":") catch return PromptError.OutOfMemory;
            appendJsonEscapedString(*out, _.allocator, msg.role.roleStr())?;
            out.appendSlice(_.allocator, ",\"content\":") catch return PromptError.OutOfMemory;
            appendJsonEscapedString(*out, _.allocator, msg.content)?;
            if (msg.name.len > 0) {
                out.appendSlice(_.allocator, ",\"name\":") catch return PromptError.OutOfMemory;
                appendJsonEscapedString(*out, _.allocator, msg.name)?;
            }
            if (msg.tool_call_id.len > 0) {
                out.appendSlice(_.allocator, ",\"tool_call_id\":") catch return PromptError.OutOfMemory;
                appendJsonEscapedString(*out, _.allocator, msg.tool_call_id)?;
            }
            out.append(_.allocator, '}') catch return PromptError.OutOfMemory;
        }
        out.appendSlice(_.allocator, "]}") catch return PromptError.OutOfMemory;

        const slice = out.toOwnedSlice(_.allocator) catch return PromptError.OutOfMemory;
        return slice;
    }

    /// Exports messages as standard Anthropic Messages API JSON:
    /// aggregates `System` / `Developer` messages into top-level `"system"` (when non-empty)
    /// and exports remaining turns into `"messages":[...]`.
    /// Caller owns the returned slice and must free it with the window's allocator.
    fn toAnthropicJson(): ([]u8) ! PromptError {
        let sys_buf: std.ArrayList(u8) = .empty;
        defer sys_buf.deinit(_.allocator);

        let sys_count: usize = 0;
        let i: usize = 0;
        while (i < _.messages.items.len) : (i += 1) {
            const msg = _.messages.items[i];
            if (msg.role == PromptRole.System || msg.role == PromptRole.Developer) {
                if (sys_count > 0) {
                    sys_buf.appendSlice(_.allocator, "\n\n") catch return PromptError.OutOfMemory;
                }
                sys_buf.appendSlice(_.allocator, msg.content) catch return PromptError.OutOfMemory;
                sys_count += 1;
            }
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        out.append(_.allocator, '{') catch return PromptError.OutOfMemory;
        if (sys_count > 0) {
            out.appendSlice(_.allocator, "\"system\":") catch return PromptError.OutOfMemory;
            appendJsonEscapedString(*out, _.allocator, sys_buf.items)?;
            out.append(_.allocator, ',') catch return PromptError.OutOfMemory;
        }

        out.appendSlice(_.allocator, "\"messages\":[") catch return PromptError.OutOfMemory;
        let emitted: usize = 0;
        i = 0;
        while (i < _.messages.items.len) : (i += 1) {
            const msg = _.messages.items[i];
            if (msg.role == PromptRole.System || msg.role == PromptRole.Developer) {
                continue;
            }
            if (emitted > 0) {
                out.append(_.allocator, ',') catch return PromptError.OutOfMemory;
            }
            emitted += 1;

            if (msg.role == PromptRole.Tool) {
                if (msg.tool_call_id.len > 0) {
                    out.appendSlice(_.allocator, "{\"role\":\"user\",\"content\":[{\"type\":\"tool_result\",\"tool_use_id\":") catch return PromptError.OutOfMemory;
                    appendJsonEscapedString(*out, _.allocator, msg.tool_call_id)?;
                    out.appendSlice(_.allocator, ",\"content\":") catch return PromptError.OutOfMemory;
                    appendJsonEscapedString(*out, _.allocator, msg.content)?;
                    out.appendSlice(_.allocator, "}]}") catch return PromptError.OutOfMemory;
                } else {
                    out.appendSlice(_.allocator, "{\"role\":\"user\",\"content\":") catch return PromptError.OutOfMemory;
                    appendJsonEscapedString(*out, _.allocator, msg.content)?;
                    out.append(_.allocator, '}') catch return PromptError.OutOfMemory;
                }
            } else {
                const role_str = if (msg.role == PromptRole.Assistant) "assistant" else "user";
                out.appendSlice(_.allocator, "{\"role\":") catch return PromptError.OutOfMemory;
                appendJsonEscapedString(*out, _.allocator, role_str)?;
                out.appendSlice(_.allocator, ",\"content\":") catch return PromptError.OutOfMemory;
                appendJsonEscapedString(*out, _.allocator, msg.content)?;
                out.append(_.allocator, '}') catch return PromptError.OutOfMemory;
            }
        }
        out.appendSlice(_.allocator, "]}") catch return PromptError.OutOfMemory;

        const slice = out.toOwnedSlice(_.allocator) catch return PromptError.OutOfMemory;
        return slice;
    }
}
