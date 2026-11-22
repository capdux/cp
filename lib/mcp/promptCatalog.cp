// Origin: Pure-CP Stdlib (Zero-Alloc MCP Prompt Catalog, Argument Contract Validator & Prefix Completion Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation implementation of MCP 2025-03-26 Prompt lifecycle primitives:
//   - `prompts/list`        : Cursor-paginated catalog listing (`listPrompts`) with `next_cursor` & `has_more`
//   - `prompts/get`         : Argument contract validation (`validatePromptGet`) checking required args,
//                             unknown/duplicate args, value length limits, and enum membership
//   - `completion/complete` : Case-insensitive prefix argument auto-completion (`completePromptArg`)
//                             with bounded output slice truncation and `has_more` indicator

// ============================================================================
// Constants & Internal String Helpers
// ============================================================================

pub const MCP_PROMPT_MAX_ENTRIES: usize = 32;
pub const MCP_PROMPT_MAX_ARGS: usize = 8;
pub const MCP_PROMPT_MAX_ENUM_CHOICES: usize = 8;
pub const MCP_PROMPT_NAME_MAX_LEN: usize = 64;
pub const MCP_PROMPT_DESC_MAX_LEN: usize = 128;
pub const MCP_PROMPT_ARG_NAME_MAX_LEN: usize = 48;
pub const MCP_PROMPT_CHOICE_MAX_LEN: usize = 32;
pub const MCP_PROMPT_DEFAULT_MAX_VALUE_LEN: usize = 1024;

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn startsWithIgnoreCaseAscii(text: []const u8, prefix: []const u8): bool {
    if (prefix.len > text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (toLowerAscii(text[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
    }
    return true;
}

fn copyIntoFixed(dst: *[]u8, src: []const u8): usize {
    const n: usize = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

// ============================================================================
// 1. Registration, Argument Contract, Pagination & Completion Models
// ============================================================================

/// Result status of registering a prompt definition into `McpPromptCatalog`.
pub enum McpPromptRegisterStatus {
    ok,
    empty_name,
    name_too_long,
    desc_too_long,
    duplicate_name,
    capacity_full,
    too_many_args,
    invalid_arg_spec,
}

/// Declarative specification for a single prompt argument (used at registration time).
pub struct McpPromptArgSpec {
    name: []const u8 = "";
    required: bool = false;
    max_value_len: usize = MCP_PROMPT_DEFAULT_MAX_VALUE_LEN;
    enforce_enum: bool = false;
    choices: [MCP_PROMPT_MAX_ENUM_CHOICES][]const u8 = [""; MCP_PROMPT_MAX_ENUM_CHOICES];
    choice_count: usize = 0;

    /// Convenience constructor for a free-text argument without enum choices.
    fn plain(name: []const u8, required: bool): McpPromptArgSpec {
        return McpPromptArgSpec {
            name: name,
            required: required,
        };
    }

    /// Convenience constructor for an argument with completion/enum choices.
    fn withChoices(
        name: []const u8,
        required: bool,
        enforce_enum: bool,
        choice_list: []const []const u8,
    ): McpPromptArgSpec {
        let spec = McpPromptArgSpec {
            name: name,
            required: required,
            enforce_enum: enforce_enum,
            choice_count: choice_list.len,
        };
        const limit: usize = if (choice_list.len < MCP_PROMPT_MAX_ENUM_CHOICES) {
            choice_list.len
        } else {
            MCP_PROMPT_MAX_ENUM_CHOICES
        };
        let i: usize = 0;
        while (i < limit) : (i += 1) {
            spec.choices[i] = choice_list[i];
        }
        return spec;
    }
}

/// Self-contained inline storage for a single registered prompt argument.
pub struct McpStoredPromptArg {
    name_buf: [MCP_PROMPT_ARG_NAME_MAX_LEN]u8 = [0; MCP_PROMPT_ARG_NAME_MAX_LEN];
    name_len: usize = 0;
    required: bool = false;
    max_value_len: usize = MCP_PROMPT_DEFAULT_MAX_VALUE_LEN;
    enforce_enum: bool = false;
    choice_bufs: [MCP_PROMPT_MAX_ENUM_CHOICES][MCP_PROMPT_CHOICE_MAX_LEN]u8 = [[0; MCP_PROMPT_CHOICE_MAX_LEN]; MCP_PROMPT_MAX_ENUM_CHOICES];
    choice_lens: [MCP_PROMPT_MAX_ENUM_CHOICES]usize = [0; MCP_PROMPT_MAX_ENUM_CHOICES];
    choice_count: usize = 0;

    fn nameSlice(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn choiceSlice(idx: usize): []const u8 {
        if (idx >= _.choice_count) {
            return "";
        }
        return _.choice_bufs[idx][0.._.choice_lens[idx]];
    }
}

/// Self-contained inline prompt entry stored in `McpPromptCatalog`.
pub struct McpPromptEntry {
    name_buf: [MCP_PROMPT_NAME_MAX_LEN]u8 = [0; MCP_PROMPT_NAME_MAX_LEN];
    name_len: usize = 0;
    desc_buf: [MCP_PROMPT_DESC_MAX_LEN]u8 = [0; MCP_PROMPT_DESC_MAX_LEN];
    desc_len: usize = 0;
    args: [MCP_PROMPT_MAX_ARGS]McpStoredPromptArg = [McpStoredPromptArg {}; MCP_PROMPT_MAX_ARGS];
    arg_count: usize = 0;
    occupied: bool = false;

    fn nameSlice(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn descSlice(): []const u8 {
        return _.desc_buf[0.._.desc_len];
    }

    fn argCount(): usize {
        return _.arg_count;
    }

    fn findArgIndex(arg_name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.arg_count) : (i += 1) {
            if (bytesEqual(_.args[i].nameSlice(), arg_name)) {
                return i;
            }
        }
        return null;
    }
}

/// Cursor-paginated `prompts/list` page metadata.
pub struct McpPromptListPage {
    valid: bool = false;
    count: usize = 0;
    total: usize = 0;
    has_more: bool = false;
    next_cursor: usize = 0;
}

/// Caller-supplied argument `(name, value)` pair for `prompts/get` validation.
pub struct McpPromptArgValue {
    name: []const u8 = "";
    value: []const u8 = "";
}

/// Primary contract violation kind detected during `validatePromptGet`.
pub enum McpPromptViolationKind {
    none,
    prompt_not_found,
    missing_required_arg,
    unknown_arg,
    duplicate_arg,
    value_too_long,
    invalid_enum_value,
}

/// Validation report returned by `McpPromptCatalog.validatePromptGet`.
pub struct McpPromptGetValidation {
    ok: bool = false;
    violation: McpPromptViolationKind = McpPromptViolationKind.none;
    violation_count: usize = 0;
    first_bad_arg_buf: [MCP_PROMPT_ARG_NAME_MAX_LEN]u8 = [0; MCP_PROMPT_ARG_NAME_MAX_LEN];
    first_bad_arg_len: usize = 0;
    prompt_index: usize = 0;

    fn firstBadArg(): []const u8 {
        return _.first_bad_arg_buf[0.._.first_bad_arg_len];
    }
}

/// Single completion candidate returned by `completePromptArg`.
pub struct McpCompletionCandidate {
    value_buf: [MCP_PROMPT_CHOICE_MAX_LEN]u8 = [0; MCP_PROMPT_CHOICE_MAX_LEN];
    value_len: usize = 0;

    fn valueSlice(): []const u8 {
        return _.value_buf[0.._.value_len];
    }
}

/// Result summary returned by `McpPromptCatalog.completePromptArg`.
pub struct McpCompletionResult {
    found: bool = false;
    count: usize = 0;
    total_matches: usize = 0;
    has_more: bool = false;
}

// ============================================================================
// 2. Zero-Alloc MCP Prompt Catalog & Argument Completion Engine
// ============================================================================

/// Zero-heap-allocation MCP Prompt Catalog (`MCP_PROMPT_MAX_ENTRIES = 32`) supporting
/// cursor pagination (`prompts/list`), contract validation (`prompts/get`), and
/// case-insensitive prefix completion (`completion/complete`).
pub struct McpPromptCatalog {
    entries: [MCP_PROMPT_MAX_ENTRIES]McpPromptEntry = [McpPromptEntry {}; MCP_PROMPT_MAX_ENTRIES];
    entry_count: usize = 0;

    fn init(): McpPromptCatalog {
        return McpPromptCatalog {};
    }

    fn count(): usize {
        return _.entry_count;
    }

    /// Finds the catalog index of a registered prompt by exact name.
    fn findPromptIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            if (_.entries[i].occupied && bytesEqual(_.entries[i].nameSlice(), name)) {
                return i;
            }
        }
        return null;
    }

    /// Returns a copy of the registered prompt entry at `index`, or `null` if out of bounds.
    fn getPrompt(index: usize): McpPromptEntry? {
        if (index >= _.entry_count || !_.entries[index].occupied) {
            return null;
        }
        return _.entries[index];
    }

    /// Registers a prompt with up to `MCP_PROMPT_MAX_ARGS` (8) validated argument specs.
    /// Rejects empty/overlong names, duplicates, full catalog, >8 args, or invalid arg specs.
    fn registerPrompt(
        *_,
        name: []const u8,
        description: []const u8,
        args: []const McpPromptArgSpec,
    ): McpPromptRegisterStatus {
        if (name.len == 0) {
            return McpPromptRegisterStatus.empty_name;
        }
        if (name.len > MCP_PROMPT_NAME_MAX_LEN) {
            return McpPromptRegisterStatus.name_too_long;
        }
        if (description.len > MCP_PROMPT_DESC_MAX_LEN) {
            return McpPromptRegisterStatus.desc_too_long;
        }
        if (_.findPromptIndex(name) != null) {
            return McpPromptRegisterStatus.duplicate_name;
        }
        if (_.entry_count >= MCP_PROMPT_MAX_ENTRIES) {
            return McpPromptRegisterStatus.capacity_full;
        }
        if (args.len > MCP_PROMPT_MAX_ARGS) {
            return McpPromptRegisterStatus.too_many_args;
        }

        // Validate all argument specs atomically before committing entry.
        let i: usize = 0;
        while (i < args.len) : (i += 1) {
            const spec = args[i];
            if (spec.name.len == 0 || spec.name.len > MCP_PROMPT_ARG_NAME_MAX_LEN) {
                return McpPromptRegisterStatus.invalid_arg_spec;
            }
            if (spec.choice_count > MCP_PROMPT_MAX_ENUM_CHOICES) {
                return McpPromptRegisterStatus.invalid_arg_spec;
            }
            if (spec.enforce_enum && spec.choice_count == 0) {
                return McpPromptRegisterStatus.invalid_arg_spec;
            }
            // Check duplicate argument names within the same prompt.
            let prev: usize = 0;
            while (prev < i) : (prev += 1) {
                if (bytesEqual(args[prev].name, spec.name)) {
                    return McpPromptRegisterStatus.invalid_arg_spec;
                }
            }
            // Validate every choice string length.
            let c: usize = 0;
            while (c < spec.choice_count) : (c += 1) {
                const ch = spec.choices[c];
                if (ch.len == 0 || ch.len > MCP_PROMPT_CHOICE_MAX_LEN) {
                    return McpPromptRegisterStatus.invalid_arg_spec;
                }
            }
        }

        // Commit entry into inline storage.
        let entry = McpPromptEntry {
            arg_count: args.len,
            occupied: true,
        };
        let name_s: []u8 = entry.name_buf[0..];
        entry.name_len = copyIntoFixed(*name_s, name);
        let desc_s: []u8 = entry.desc_buf[0..];
        entry.desc_len = copyIntoFixed(*desc_s, description);

        i = 0;
        while (i < args.len) : (i += 1) {
            const spec = args[i];
            let stored = McpStoredPromptArg {
                required: spec.required,
                max_value_len: if (spec.max_value_len == 0) MCP_PROMPT_DEFAULT_MAX_VALUE_LEN else spec.max_value_len,
                enforce_enum: spec.enforce_enum,
                choice_count: spec.choice_count,
            };
            let aname_s: []u8 = stored.name_buf[0..];
            stored.name_len = copyIntoFixed(*aname_s, spec.name);

            let c: usize = 0;
            while (c < spec.choice_count) : (c += 1) {
                let c_s: []u8 = stored.choice_bufs[c][0..];
                stored.choice_lens[c] = copyIntoFixed(*c_s, spec.choices[c]);
            }
            entry.args[i] = stored;
        }

        _.entries[_.entry_count] = entry;
        _.entry_count += 1;
        return McpPromptRegisterStatus.ok;
    }

    /// Cursor-paginated `prompts/list` query writing catalog indices into `out_indices`.
    /// Returns `valid = false` if `limit == 0` or `cursor > _.count()`.
    fn listPrompts(cursor: usize, limit: usize, out_indices: *[]usize): McpPromptListPage {
        if (limit == 0 || cursor > _.entry_count) {
            return McpPromptListPage {
                valid: false,
                total: _.entry_count,
            };
        }

        const remaining: usize = _.entry_count - cursor;
        let page_cap: usize = if (limit < remaining) limit else remaining;
        if (out_indices.len < page_cap) {
            page_cap = out_indices.len;
        }

        let i: usize = 0;
        while (i < page_cap) : (i += 1) {
            out_indices[i] = cursor + i;
        }

        const next_pos: usize = cursor + page_cap;
        const more: bool = next_pos < _.entry_count;
        return McpPromptListPage {
            valid: true,
            count: page_cap,
            total: _.entry_count,
            has_more: more,
            next_cursor: if (more) next_pos else 0,
        };
    }

    /// Validates a `prompts/get` invocation against the registered prompt's argument contract:
    /// checks prompt existence, unknown args, duplicate args, max value length, enum membership,
    /// and all required arguments.
    fn validatePromptGet(
        prompt_name: []const u8,
        provided_args: []const McpPromptArgValue,
    ): McpPromptGetValidation {
        const p_idx = _.findPromptIndex(prompt_name) ?? {
            return McpPromptGetValidation {
                ok: false,
                violation: McpPromptViolationKind.prompt_not_found,
                violation_count: 1,
            };
        };

        const entry = _.entries[p_idx];
        let seen: [MCP_PROMPT_MAX_ARGS]bool = [false; MCP_PROMPT_MAX_ARGS];
        let report = McpPromptGetValidation {
            ok: true,
            violation: McpPromptViolationKind.none,
            prompt_index: p_idx,
        };

        // 1. Validate each provided argument against declared contract.
        let i: usize = 0;
        while (i < provided_args.len) : (i += 1) {
            const pv = provided_args[i];
            if (arg_idx := entry.findArgIndex(pv.name)) {
                if (seen[arg_idx]) {
                    if (report.violation_count == 0) {
                        report.violation = McpPromptViolationKind.duplicate_arg;
                        let dst_s: []u8 = report.first_bad_arg_buf[0..];
                        report.first_bad_arg_len = copyIntoFixed(*dst_s, pv.name);
                    }
                    report.violation_count += 1;
                    report.ok = false;
                    continue;
                }
                seen[arg_idx] = true;

                const spec = entry.args[arg_idx];
                if (pv.value.len > spec.max_value_len) {
                    if (report.violation_count == 0) {
                        report.violation = McpPromptViolationKind.value_too_long;
                        let dst_s: []u8 = report.first_bad_arg_buf[0..];
                        report.first_bad_arg_len = copyIntoFixed(*dst_s, pv.name);
                    }
                    report.violation_count += 1;
                    report.ok = false;
                    continue;
                }

                if (spec.enforce_enum) {
                    let matched: bool = false;
                    let c: usize = 0;
                    while (c < spec.choice_count) : (c += 1) {
                        if (bytesEqual(spec.choiceSlice(c), pv.value)) {
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        if (report.violation_count == 0) {
                            report.violation = McpPromptViolationKind.invalid_enum_value;
                            let dst_s: []u8 = report.first_bad_arg_buf[0..];
                            report.first_bad_arg_len = copyIntoFixed(*dst_s, pv.name);
                        }
                        report.violation_count += 1;
                        report.ok = false;
                    }
                }
            } else {
                if (report.violation_count == 0) {
                    report.violation = McpPromptViolationKind.unknown_arg;
                    let dst_s: []u8 = report.first_bad_arg_buf[0..];
                    report.first_bad_arg_len = copyIntoFixed(*dst_s, pv.name);
                }
                report.violation_count += 1;
                report.ok = false;
            }
        }

        // 2. Verify all required arguments were supplied.
        let a: usize = 0;
        while (a < entry.arg_count) : (a += 1) {
            if (entry.args[a].required && !seen[a]) {
                if (report.violation_count == 0) {
                    report.violation = McpPromptViolationKind.missing_required_arg;
                    let dst_s: []u8 = report.first_bad_arg_buf[0..];
                    report.first_bad_arg_len = copyIntoFixed(*dst_s, entry.args[a].nameSlice());
                }
                report.violation_count += 1;
                report.ok = false;
            }
        }

        return report;
    }

    /// Case-insensitive ASCII prefix auto-completion (`completion/complete`) for a prompt argument.
    /// Writes matching candidates into `out_candidates` and sets `has_more` if truncated.
    fn completePromptArg(
        prompt_name: []const u8,
        arg_name: []const u8,
        prefix: []const u8,
        out_candidates: *[]McpCompletionCandidate,
    ): McpCompletionResult {
        const p_idx = _.findPromptIndex(prompt_name) ?? return McpCompletionResult {};
        const entry = _.entries[p_idx];
        const a_idx = entry.findArgIndex(arg_name) ?? return McpCompletionResult {};
        const spec = entry.args[a_idx];

        let written: usize = 0;
        let matched: usize = 0;
        let c: usize = 0;
        while (c < spec.choice_count) : (c += 1) {
            const choice = spec.choiceSlice(c);
            if (startsWithIgnoreCaseAscii(choice, prefix)) {
                matched += 1;
                if (written < out_candidates.len) {
                    let cand = McpCompletionCandidate {};
                    let dst_s: []u8 = cand.value_buf[0..];
                    cand.value_len = copyIntoFixed(*dst_s, choice);
                    out_candidates[written] = cand;
                    written += 1;
                }
            }
        }

        return McpCompletionResult {
            found: true,
            count: written,
            total_matches: matched,
            has_more: matched > written,
        };
    }

    /// Clears all registered prompts.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            _.entries[i] = McpPromptEntry {};
        }
        _.entry_count = 0;
    }
}
