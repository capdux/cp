// Source: Absorbed from /data/dev/bun/src/runtime/node/util/parse_args_utils.cp | Synced: 2026-11-22T15:13:33Z
// Zero-allocation Node/Bun `util.parseArgs` token classifier, short/long option lookup & tokenizer.

/// Default recommended capacity for caller-provided token buffers.
pub const PARSE_ARGS_MAX_TOKENS: usize = 128;

/// Error set returned by strict/validated `parseArgsTokens`.
pub err ParseArgsError {
    UnknownOption,
    UnexpectedArgument,
    MissingArgument,
    OptionLikeValue,
    UnexpectedPositional,
    NegativeNotAllowed,
    OutputBufferTooSmall,
}

/// Declared value type of a CLI option (mirrors `util.parseArgs` `{ type: "boolean" | "string" }`).
pub enum ParseArgsValueType: u8 {
    Boolean = 0,
    String = 1,
}

/// Metadata describing a known option for `parseArgsTokens` and `classifyArgToken`.
pub struct ParseArgsOptionDef {
    /// Long option name without leading dashes (e.g. `"verbose"` for `--verbose`).
    long_name: []const u8 = "",
    /// Optional single-byte short name without leading dash (e.g. `"v"` for `-v`). Empty if none.
    short_name: []const u8 = "",
    /// Value kind (`Boolean` or `String`).
    value_type: ParseArgsValueType = .Boolean,
    /// Whether the option accepts multiple occurrences.
    multiple: bool = false,
    /// Optional default string value when `has_default` is true and `value_type == .String`.
    default_str: []const u8 = "",
    /// Optional default boolean value when `has_default` is true and `value_type == .Boolean`.
    default_bool: bool = false,
    /// Whether a default value is configured.
    has_default: bool = false,
}

/// Low-level lexical classification of a single `argv` element (matches Bun `TokenSubtype`).
pub enum ParseArgsTokenSubtype: u8 {
    /// Exact `"--"` option terminator.
    OptionTerminator = 0,
    /// Single short option, e.g. `"-f"`.
    LoneShortOption = 1,
    /// Clustered short options, e.g. `"-abc"`.
    ShortOptionGroup = 2,
    /// Short string option with inline attached value, e.g. `"-fFILE"`.
    ShortOptionAndValue = 3,
    /// Long option without inline value, e.g. `"--foo"`.
    LoneLongOption = 4,
    /// Long option with inline `=value`, e.g. `"--foo=bar"`.
    LongOptionAndValue = 5,
    /// Positional argument (including `""` and `"-"`).
    Positional = 6,
}

/// High-level parsed token category (matches Bun `TokenKind`).
pub enum ParseArgsTokenKind: u8 {
    Positional = 0,
    Option = 1,
    OptionTerminator = 2,
}

/// Result of splitting a long option argument (`--name` or `--name=value`).
pub struct ParseArgsLongSplit {
    /// Option name without leading `"--"` and without `"=value"`.
    name: []const u8 = "",
    /// Raw option prefix including `"--"` (excluding `"=value"`).
    raw_name: []const u8 = "",
    /// Inline value slice after `'='` (empty if `has_value` is false).
    value: []const u8 = "",
    /// True when `'='` was present at index >= 3.
    has_value: bool = false,
}

/// Zero-allocation parsed token emitted by `parseArgsTokens`.
pub struct ParseArgsToken {
    kind: ParseArgsTokenKind = .Positional,
    /// Index in the input `args` slice.
    index: u32 = 0,
    /// Resolved canonical option name (long name if known, otherwise raw stripped name). Empty for positionals.
    name: []const u8 = "",
    /// Raw option name without inline value (e.g. `"--port"`, `"-p"`, or single char slice for groups).
    raw_name: []const u8 = "",
    /// Positional text or string option value.
    value: []const u8 = "",
    /// True if `value` carries a string value.
    has_value: bool = false,
    /// True if the option value came inline (`--foo=bar` or `-fFILE`).
    inline_value: bool = false,
    /// True if parsed from `--no-<flag>` with `allow_negative`.
    negative: bool = false,
    /// Matched index into the caller's `options` table, if any.
    option_idx: usize? = null,
}

/// Configuration for `parseArgsTokens` (mirrors Node/Bun `util.parseArgs` options).
pub struct ParseArgsConfig {
    /// Reject unknown options, wrong value types, and option-like values.
    strict: bool = true,
    /// Allow positional arguments (both before and after `"--"`).
    allow_positionals: bool = false,
    /// Allow `--no-<flag>` negation for boolean options.
    allow_negative: bool = false,
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn indexOfByte(s: []const u8, needle: u8): usize? {
    for (i of 0..s.len) {
        if (s[i] == needle) return i;
    }
    return null;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    for (i of 0..prefix.len) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

/// Detect whether `value` looks like another CLI option (e.g. `--port --verbose`),
/// which in strict mode indicates a likely omitted string argument.
pub fn isOptionLikeValue(value: []const u8): bool {
    return value.len > 1 && value[0] == '-';
}

/// Find an option index by its short name (1-byte string like `"f"`).
/// Exact `short_name` match takes priority; otherwise falls back to a 1-byte `long_name` match.
pub fn findOptionByShortName(short_name: []const u8, options: []const ParseArgsOptionDef): usize? {
    if (short_name.len == 0) return null;
    let long_option_index: usize? = null;
    for (i of 0..options.len) {
        const option = options[i];
        if (option.short_name.len > 0 && sliceEql(short_name, option.short_name)) {
            return i;
        }
        if (option.long_name.len == 1 && sliceEql(short_name, option.long_name)) {
            long_option_index = i;
        }
    }
    return long_option_index;
}

/// Find an option index by exact `long_name`.
pub fn findOptionByLongName(long_name: []const u8, options: []const ParseArgsOptionDef): usize? {
    if (long_name.len == 0) return null;
    for (i of 0..options.len) {
        if (sliceEql(long_name, options[i].long_name)) {
            return i;
        }
    }
    return null;
}

/// Classify a single raw CLI argument according to Node/Bun `util.parseArgs` rules.
pub fn classifyArgToken(arg: []const u8, options: []const ParseArgsOptionDef): ParseArgsTokenSubtype {
    const len = arg.len;
    if (len == 2) {
        if (arg[0] == '-') {
            if (arg[1] == '-') {
                return .OptionTerminator;
            }
            return .LoneShortOption;
        }
    } else if (len > 2) {
        if (arg[0] == '-' && arg[1] == '-') {
            if (eq_idx := indexOfByte(arg, '=')) {
                if (eq_idx >= 3) {
                    return .LongOptionAndValue;
                }
            }
            return .LoneLongOption;
        } else if (arg[0] == '-') {
            const first_char = arg[1..2];
            if (opt_idx := findOptionByShortName(first_char, options)) {
                if (options[opt_idx].value_type == .String) {
                    return .ShortOptionAndValue;
                }
                return .ShortOptionGroup;
            }
            return .ShortOptionGroup;
        }
    }
    return .Positional;
}

/// Split a long option (`--foo` or `--foo=bar`) into name, raw prefix, and optional inline value.
pub fn splitLongOption(arg: []const u8): ParseArgsLongSplit {
    if (arg.len <= 2 || arg[0] != '-' || arg[1] != '-') {
        return ParseArgsLongSplit{
            name: arg,
            raw_name: arg,
            value: "",
            has_value: false,
        };
    }
    if (eq_idx := indexOfByte(arg, '=')) {
        if (eq_idx >= 3) {
            return ParseArgsLongSplit{
                name: arg[2..eq_idx],
                raw_name: arg[0..eq_idx],
                value: arg[(eq_idx + 1)..],
                has_value: true,
            };
        }
    }
    return ParseArgsLongSplit{
        name: arg[2..],
        raw_name: arg,
        value: "",
        has_value: false,
    };
}

fn emitToken(out_tokens: []ParseArgsToken, count: *usize, tok: ParseArgsToken): usize ! ParseArgsError {
    if (count.* >= out_tokens.len) {
        return err.OutputBufferTooSmall;
    }
    out_tokens[count.*] = tok;
    count.* += 1;
    return count.*;
}

/// Tokenize and validate `args` into caller-provided `out_tokens` with zero heap allocations.
/// Returns the number of tokens written to `out_tokens`.
pub fn parseArgsTokens(
    args: []const ([]const u8),
    options: []const ParseArgsOptionDef,
    cfg: ParseArgsConfig,
    out_tokens: []ParseArgsToken,
): usize ! ParseArgsError {
    let out_count: usize = 0;
    let i: usize = 0;
    let after_terminator = false;

    while (i < args.len) {
        const arg = args[i];
        const arg_index = u32(i);

        if (after_terminator) {
            if (cfg.strict && !cfg.allow_positionals) {
                return err.UnexpectedPositional;
            }
            * = emitToken(out_tokens, *out_count, ParseArgsToken{
                kind: .Positional,
                index: arg_index,
                value: arg,
                has_value: true,
            })?;
            i += 1;
            continue;
        }

        const subtype = classifyArgToken(arg, options);
        switch (subtype) {
            .OptionTerminator => {
                after_terminator = true;
                * = emitToken(out_tokens, *out_count, ParseArgsToken{
                    kind: .OptionTerminator,
                    index: arg_index,
                    raw_name: arg,
                })?;
                i += 1;
            },
            .Positional => {
                if (cfg.strict && !cfg.allow_positionals) {
                    return err.UnexpectedPositional;
                }
                * = emitToken(out_tokens, *out_count, ParseArgsToken{
                    kind: .Positional,
                    index: arg_index,
                    value: arg,
                    has_value: true,
                })?;
                i += 1;
            },
            .LoneLongOption => {
                const split = splitLongOption(arg);
                let opt_idx = findOptionByLongName(split.name, options);
                let is_negative = false;
                let resolved_name = split.name;

                if (opt_idx == null && cfg.allow_negative && startsWith(split.name, "no-") && split.name.len > 3) {
                    const base_name = split.name[3..];
                    if (base_idx := findOptionByLongName(base_name, options)) {
                        if (options[base_idx].value_type != .Boolean) {
                            if (cfg.strict) return err.NegativeNotAllowed;
                        } else {
                            opt_idx = base_idx;
                            resolved_name = options[base_idx].long_name;
                            is_negative = true;
                        }
                    } else if (!cfg.strict) {
                        resolved_name = base_name;
                        is_negative = true;
                    }
                }

                if (idx := opt_idx) {
                    const def = options[idx];
                    if (def.value_type == .String) {
                        if (i + 1 >= args.len) {
                            return err.MissingArgument;
                        }
                        const next_arg = args[i + 1];
                        if (cfg.strict && isOptionLikeValue(next_arg)) {
                            return err.OptionLikeValue;
                        }
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: def.long_name,
                            raw_name: split.raw_name,
                            value: next_arg,
                            has_value: true,
                            inline_value: false,
                            negative: false,
                            option_idx: idx,
                        })?;
                        i += 2;
                    } else {
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: def.long_name,
                            raw_name: split.raw_name,
                            has_value: false,
                            inline_value: false,
                            negative: is_negative,
                            option_idx: idx,
                        })?;
                        i += 1;
                    }
                } else {
                    if (cfg.strict) {
                        return err.UnknownOption;
                    }
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: resolved_name,
                        raw_name: split.raw_name,
                        has_value: false,
                        inline_value: false,
                        negative: is_negative,
                        option_idx: null,
                    })?;
                    i += 1;
                }
            },
            .LongOptionAndValue => {
                const split = splitLongOption(arg);
                const opt_idx = findOptionByLongName(split.name, options);
                if (idx := opt_idx) {
                    const def = options[idx];
                    if (def.value_type == .Boolean && cfg.strict) {
                        return err.UnexpectedArgument;
                    }
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: def.long_name,
                        raw_name: split.raw_name,
                        value: split.value,
                        has_value: true,
                        inline_value: true,
                        negative: false,
                        option_idx: idx,
                    })?;
                } else {
                    if (cfg.strict) {
                        return err.UnknownOption;
                    }
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: split.name,
                        raw_name: split.raw_name,
                        value: split.value,
                        has_value: true,
                        inline_value: true,
                        negative: false,
                        option_idx: null,
                    })?;
                }
                i += 1;
            },
            .LoneShortOption => {
                const short_ch = arg[1..2];
                const opt_idx = findOptionByShortName(short_ch, options);
                if (idx := opt_idx) {
                    const def = options[idx];
                    if (def.value_type == .String) {
                        if (i + 1 >= args.len) {
                            return err.MissingArgument;
                        }
                        const next_arg = args[i + 1];
                        if (cfg.strict && isOptionLikeValue(next_arg)) {
                            return err.OptionLikeValue;
                        }
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: def.long_name,
                            raw_name: arg,
                            value: next_arg,
                            has_value: true,
                            inline_value: false,
                            negative: false,
                            option_idx: idx,
                        })?;
                        i += 2;
                    } else {
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: def.long_name,
                            raw_name: arg,
                            has_value: false,
                            inline_value: false,
                            negative: false,
                            option_idx: idx,
                        })?;
                        i += 1;
                    }
                } else {
                    if (cfg.strict) {
                        return err.UnknownOption;
                    }
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: short_ch,
                        raw_name: arg,
                        has_value: false,
                        inline_value: false,
                        negative: false,
                        option_idx: null,
                    })?;
                    i += 1;
                }
            },
            .ShortOptionAndValue => {
                const short_ch = arg[1..2];
                const opt_idx = findOptionByShortName(short_ch, options);
                const val = arg[2..];
                if (idx := opt_idx) {
                    const def = options[idx];
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: def.long_name,
                        raw_name: arg[0..2],
                        value: val,
                        has_value: true,
                        inline_value: true,
                        negative: false,
                        option_idx: idx,
                    })?;
                } else {
                    if (cfg.strict) {
                        return err.UnknownOption;
                    }
                    * = emitToken(out_tokens, *out_count, ParseArgsToken{
                        kind: .Option,
                        index: arg_index,
                        name: short_ch,
                        raw_name: arg[0..2],
                        value: val,
                        has_value: true,
                        inline_value: true,
                        negative: false,
                        option_idx: null,
                    })?;
                }
                i += 1;
            },
            .ShortOptionGroup => {
                let consumed_next = false;
                let j: usize = 1;
                while (j < arg.len) {
                    const short_ch = arg[j..(j + 1)];
                    const opt_idx = findOptionByShortName(short_ch, options);
                    if (idx := opt_idx) {
                        const def = options[idx];
                        if (def.value_type == .String) {
                            if (j + 1 < arg.len) {
                                // Remaining chars in group form the inline value (e.g. -vfFILE)
                                * = emitToken(out_tokens, *out_count, ParseArgsToken{
                                    kind: .Option,
                                    index: arg_index,
                                    name: def.long_name,
                                    raw_name: short_ch,
                                    value: arg[(j + 1)..],
                                    has_value: true,
                                    inline_value: true,
                                    negative: false,
                                    option_idx: idx,
                                })?;
                                break;
                            }
                            if (i + 1 >= args.len) {
                                return err.MissingArgument;
                            }
                            const next_arg = args[i + 1];
                            if (cfg.strict && isOptionLikeValue(next_arg)) {
                                return err.OptionLikeValue;
                            }
                            * = emitToken(out_tokens, *out_count, ParseArgsToken{
                                kind: .Option,
                                index: arg_index,
                                name: def.long_name,
                                raw_name: short_ch,
                                value: next_arg,
                                has_value: true,
                                inline_value: false,
                                negative: false,
                                option_idx: idx,
                            })?;
                            consumed_next = true;
                            break;
                        }
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: def.long_name,
                            raw_name: short_ch,
                            has_value: false,
                            inline_value: false,
                            negative: false,
                            option_idx: idx,
                        })?;
                    } else {
                        if (cfg.strict) {
                            return err.UnknownOption;
                        }
                        * = emitToken(out_tokens, *out_count, ParseArgsToken{
                            kind: .Option,
                            index: arg_index,
                            name: short_ch,
                            raw_name: short_ch,
                            has_value: false,
                            inline_value: false,
                            negative: false,
                            option_idx: null,
                        })?;
                    }
                    j += 1;
                }
                i += if (consumed_next) 2 else 1;
            },
        }
    }

    return out_count;
}
