// Origin: Pure-CP Stdlib (AI Multi-Model Chat Template Formatter: ChatML, Llama3 & Mistral) | Created: 2026-11-22
// Pure CP zero-allocation multi-model chat prompt formatter supporting ChatML (Qwen/OpenAI/DeepSeek),
// Llama-3, Mistral Instruct, and PlainTranscript formats, with built-in control-token injection
// sanitization (`sanitizeControlTokens`) for untrusted User and Tool payloads.

/// Supported LLM chat template families.
pub enum ChatTemplateKind {
    ChatMl,
    Llama3,
    Mistral,
    PlainTranscript,
}

/// Conversational role of a chat message.
pub enum ChatMsgRole {
    System,
    User,
    Assistant,
    Tool,
}

/// A single message in a chat prompt sequence.
/// `name` is optional and primarily used for `ChatMsgRole.Tool` attribution.
pub struct ChatMessage {
    role: ChatMsgRole = ChatMsgRole.User,
    content: string = "",
    name: string = "",
}

// ============================================================================
// Internal Buffer Writer & Control-Token Matching Helpers
// ============================================================================

struct SliceWriter {
    buf: *[]u8,
    pos: usize = 0,

    pub fn writeBytes(*_, data: string): bool {
        if (_.pos + data.len > _.buf.len) {
            return false;
        }
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            _.buf[_.pos + i] = data[i];
        }
        _.pos += data.len;
        return true;
    }

    pub fn writeByte(*_, b: u8): bool {
        if (_.pos + 1 > _.buf.len) {
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }
}

fn startsWithAt(text: string, pos: usize, needle: string): bool {
    if (needle.len == 0 || pos + needle.len > text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) : (i += 1) {
        if (text[pos + i] != needle[i]) {
            return false;
        }
    }
    return true;
}

/// Matches known special control tokens at `input[pos..]` and writes their neutralized
/// form into `w`. Returns the number of input bytes consumed (> 0) if a control token
/// matched, `0` if no control token starts at `pos`, or `null` if `w` overflowed.
fn tryWriteSanitizedControlAt(input: string, pos: usize, w: *SliceWriter): usize? {
    // 1. `<|...|>` family (ChatML, Llama3, DeepSeek, Qwen)
    const pipe_tokens: [12]string = [
        "<|im_start|>",
        "<|im_end|>",
        "<|endoftext|>",
        "<|begin_of_text|>",
        "<|end_of_text|>",
        "<|start_header_id|>",
        "<|end_header_id|>",
        "<|eot_id|>",
        "<|eom_id|>",
        "<|system|>",
        "<|user|>",
        "<|assistant|>",
    ];

    let k: usize = 0;
    while (k < pipe_tokens.len) : (k += 1) {
        const tok = pipe_tokens[k];
        if (startsWithAt(input, pos, tok)) {
            // Neutralize `<|inner|>` -> `<\|inner\|>` so no special token sequence survives
            const inner = tok[2..tok.len - 2];
            if (!w.writeBytes("<\\|")) return null;
            if (!w.writeBytes(inner)) return null;
            if (!w.writeBytes("\\|>")) return null;
            return tok.len;
        }
    }

    // 2. Mistral control markers
    if (startsWithAt(input, pos, "[INST]")) {
        if (!w.writeBytes("[\\INST]")) return null;
        return 6;
    }
    if (startsWithAt(input, pos, "[/INST]")) {
        if (!w.writeBytes("[\\/INST]")) return null;
        return 7;
    }
    if (startsWithAt(input, pos, "<s>")) {
        if (!w.writeBytes("<\\s>")) return null;
        return 3;
    }
    if (startsWithAt(input, pos, "</s>")) {
        if (!w.writeBytes("<\\/s>")) return null;
        return 4;
    }

    return 0;
}

fn writeContent(text: string, sanitize: bool, w: *SliceWriter): bool {
    if (!sanitize) {
        return w.writeBytes(text);
    }
    let pos: usize = 0;
    while (pos < text.len) {
        if (consumed := tryWriteSanitizedControlAt(text, pos, w)) {
            if (consumed > 0) {
                pos += consumed;
            } else {
                if (!w.writeByte(text[pos])) {
                    return false;
                }
                pos += 1;
            }
        } else {
            return false;
        }
    }
    return true;
}

fn shouldSanitizeRole(role: ChatMsgRole, sanitize_user_content: bool): bool {
    if (!sanitize_user_content) {
        return false;
    }
    return role == ChatMsgRole.User || role == ChatMsgRole.Tool;
}

fn writeRoleHeader(role: ChatMsgRole, name: string, sanitize_name: bool, w: *SliceWriter): bool {
    if (role == ChatMsgRole.System) {
        return w.writeBytes("system");
    }
    if (role == ChatMsgRole.User) {
        return w.writeBytes("user");
    }
    if (role == ChatMsgRole.Assistant) {
        return w.writeBytes("assistant");
    }
    if (!w.writeBytes("tool")) {
        return false;
    }
    if (name.len > 0) {
        if (!w.writeBytes(" name=")) {
            return false;
        }
        if (!writeContent(name, sanitize_name, w)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Public Sanitization & Multi-Model Chat Prompt Formatting APIs
// ============================================================================

/// Escapes/neutralizes forged model control tokens (such as `<|im_start|>`, `<|im_end|>`,
/// `<|eot_id|>`, `<|start_header_id|>`, `<|end_header_id|>`, `[INST]`, `[/INST]`, `<s>`, `</s>`)
/// in untrusted `input` into `out_buf` to prevent prompt-boundary injection.
/// Returns total bytes written, or `null` if `out_buf` is too small.
pub fn sanitizeControlTokens(input: string, out_buf: *[]u8): usize? {
    if (input.len == 0) {
        return 0;
    }
    let w = SliceWriter{ buf: out_buf, pos: 0 };
    if (!writeContent(input, true, &w)) {
        return null;
    }
    return w.pos;
}

fn formatChatMl(
    messages: []const ChatMessage,
    add_generation_prompt: bool,
    sanitize_user_content: bool,
    w: *SliceWriter,
): bool {
    let i: usize = 0;
    while (i < messages.len) : (i += 1) {
        const msg = messages[i];
        const do_san = shouldSanitizeRole(msg.role, sanitize_user_content);
        if (!w.writeBytes("<|im_start|>")) return false;
        if (!writeRoleHeader(msg.role, msg.name, do_san, w)) return false;
        if (!w.writeByte('\n')) return false;
        if (!writeContent(msg.content, do_san, w)) return false;
        if (!w.writeBytes("<|im_end|>\n")) return false;
    }
    if (add_generation_prompt) {
        if (!w.writeBytes("<|im_start|>assistant\n")) return false;
    }
    return true;
}

fn formatLlama3(
    messages: []const ChatMessage,
    add_generation_prompt: bool,
    sanitize_user_content: bool,
    w: *SliceWriter,
): bool {
    if (messages.len == 0 && !add_generation_prompt) {
        return true;
    }
    if (!w.writeBytes("<|begin_of_text|>")) return false;

    let i: usize = 0;
    while (i < messages.len) : (i += 1) {
        const msg = messages[i];
        const do_san = shouldSanitizeRole(msg.role, sanitize_user_content);
        if (!w.writeBytes("<|start_header_id|>")) return false;
        if (!writeRoleHeader(msg.role, msg.name, do_san, w)) return false;
        if (!w.writeBytes("<|end_header_id|>\n\n")) return false;
        if (!writeContent(msg.content, do_san, w)) return false;
        if (!w.writeBytes("<|eot_id|>")) return false;
    }
    if (add_generation_prompt) {
        if (!w.writeBytes("<|start_header_id|>assistant<|end_header_id|>\n\n")) return false;
    }
    return true;
}

fn formatMistral(
    messages: []const ChatMessage,
    add_generation_prompt: bool,
    sanitize_user_content: bool,
    w: *SliceWriter,
): bool {
    if (messages.len == 0) {
        if (add_generation_prompt) {
            return w.writeBytes("<s>");
        }
        return true;
    }

    let first_turn: bool = true;
    let has_pending_sys: bool = false;
    let pending_sys: string = "";

    let i: usize = 0;
    while (i < messages.len) : (i += 1) {
        const msg = messages[i];
        if (msg.role == ChatMsgRole.System) {
            if (!has_pending_sys) {
                has_pending_sys = true;
                pending_sys = msg.content;
            } else {
                // Flush earlier system block if multiple consecutive system messages occur
                if (first_turn) {
                    if (!w.writeBytes("<s>[INST] ")) return false;
                    first_turn = false;
                } else {
                    if (!w.writeBytes("[INST] ")) return false;
                }
                if (!writeContent(pending_sys, false, w)) return false;
                if (!w.writeBytes(" [/INST]")) return false;
                pending_sys = msg.content;
            }
            continue;
        }

        if (msg.role == ChatMsgRole.User || msg.role == ChatMsgRole.Tool) {
            const do_san = shouldSanitizeRole(msg.role, sanitize_user_content);
            if (first_turn) {
                if (!w.writeBytes("<s>[INST] ")) return false;
                first_turn = false;
            } else {
                if (!w.writeBytes("[INST] ")) return false;
            }
            if (has_pending_sys) {
                if (!writeContent(pending_sys, false, w)) return false;
                if (!w.writeBytes("\n\n")) return false;
                has_pending_sys = false;
            }
            if (msg.role == ChatMsgRole.Tool) {
                if (msg.name.len > 0) {
                    if (!w.writeBytes("[tool:")) return false;
                    if (!writeContent(msg.name, do_san, w)) return false;
                    if (!w.writeBytes("] ")) return false;
                } else {
                    if (!w.writeBytes("[tool] ")) return false;
                }
            }
            if (!writeContent(msg.content, do_san, w)) return false;
            if (!w.writeBytes(" [/INST]")) return false;
            continue;
        }

        // Assistant turn
        if (has_pending_sys) {
            if (first_turn) {
                if (!w.writeBytes("<s>[INST] ")) return false;
                first_turn = false;
            } else {
                if (!w.writeBytes("[INST] ")) return false;
            }
            if (!writeContent(pending_sys, false, w)) return false;
            if (!w.writeBytes(" [/INST]")) return false;
            has_pending_sys = false;
        } else if (first_turn) {
            if (!w.writeBytes("<s>")) return false;
            first_turn = false;
        }
        if (!w.writeByte(' ')) return false;
        if (!writeContent(msg.content, false, w)) return false;
        if (!w.writeBytes("</s>")) return false;
    }

    if (has_pending_sys) {
        if (first_turn) {
            if (!w.writeBytes("<s>[INST] ")) return false;
        } else {
            if (!w.writeBytes("[INST] ")) return false;
        }
        if (!writeContent(pending_sys, false, w)) return false;
        if (!w.writeBytes(" [/INST]")) return false;
    }

    * = add_generation_prompt;
    return true;
}

fn formatPlainTranscript(
    messages: []const ChatMessage,
    add_generation_prompt: bool,
    sanitize_user_content: bool,
    w: *SliceWriter,
): bool {
    let i: usize = 0;
    while (i < messages.len) : (i += 1) {
        const msg = messages[i];
        const do_san = shouldSanitizeRole(msg.role, sanitize_user_content);
        if (msg.role == ChatMsgRole.System) {
            if (!w.writeBytes("System: ")) return false;
        } else if (msg.role == ChatMsgRole.User) {
            if (!w.writeBytes("User: ")) return false;
        } else if (msg.role == ChatMsgRole.Assistant) {
            if (!w.writeBytes("Assistant: ")) return false;
        } else {
            if (msg.name.len > 0) {
                if (!w.writeBytes("Tool(")) return false;
                if (!writeContent(msg.name, do_san, w)) return false;
                if (!w.writeBytes("): ")) return false;
            } else {
                if (!w.writeBytes("Tool: ")) return false;
            }
        }
        if (!writeContent(msg.content, do_san, w)) return false;
        if (!w.writeByte('\n')) return false;
    }
    if (add_generation_prompt) {
        if (!w.writeBytes("Assistant:")) return false;
    }
    return true;
}

/// Formats `messages` into `out_buf` according to the selected LLM `kind` template.
/// When `sanitize_user_content` is `true`, untrusted `User` and `Tool` contents (and tool names)
/// are sanitized via `sanitizeControlTokens` while trusted `System` and `Assistant` messages
/// and template-generated control tags remain intact.
/// Returns total bytes written into `out_buf`, or `null` if `out_buf` is too small.
pub fn formatChatPrompt(
    kind: ChatTemplateKind,
    messages: []const ChatMessage,
    add_generation_prompt: bool,
    sanitize_user_content: bool,
    out_buf: *[]u8,
): usize? {
    if (messages.len == 0 && !add_generation_prompt) {
        return 0;
    }

    let w = SliceWriter{ buf: out_buf, pos: 0 };
    let ok: bool = false;

    if (kind == ChatTemplateKind.ChatMl) {
        ok = formatChatMl(messages, add_generation_prompt, sanitize_user_content, &w);
    } else if (kind == ChatTemplateKind.Llama3) {
        ok = formatLlama3(messages, add_generation_prompt, sanitize_user_content, &w);
    } else if (kind == ChatTemplateKind.Mistral) {
        ok = formatMistral(messages, add_generation_prompt, sanitize_user_content, &w);
    } else {
        ok = formatPlainTranscript(messages, add_generation_prompt, sanitize_user_content, &w);
    }

    if (!ok) {
        return null;
    }
    return w.pos;
}
