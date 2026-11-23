// Source: Absorbed from /data/dev/bun/src/cli/add_completions.cp | Synced: 2026-11-22T01:22:09Z
// ============================================================================
// lib/cli/shellCompletions.cp — Cross-Shell Dynamic Completions & RC Installer
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven shell completion generator, dynamic
// context completion evaluator, and idempotent RC file installer for `cap`:
//   1. Declarative command/flag/enum/file-extension rules and full completion
//      script generator (`generateCapShellCompletion`) for Bash, Zsh, Fish,
//      and PowerShell;
//   2. Real-time context-aware dynamic completion evaluator
//      (`computeDynamicCompletions`) over CLI tokens, cursor word index, and
//      project context (`cap.json` scripts, installed packages, candidate files)
//      with `cap:simd` (`Simd.toLowerAscii`) case-folding and relevance ranking;
//   3. Idempotent RC file injection/update/uninstall planner
//      (`planRcCompletionInstall`) with `# >>> cap completions >>>` markers
//      and `Simd.indexOfByte` marker scanning.

import { Simd } from "cap:simd";

/// Opening marker line delimiting a managed `cap` completion block in shell RC files.
pub const SHELL_COMP_MARKER_BEGIN: []const u8 = "# >>> cap completions >>>";

/// Closing marker line delimiting a managed `cap` completion block in shell RC files.
pub const SHELL_COMP_MARKER_END: []const u8 = "# <<< cap completions <<<";

/// Maximum prefix length folded with SIMD stack buffer during scoring.
pub const SHELL_COMP_MAX_PREFIX_LEN: usize = 128;

/// Sentinel subcommand index indicating no active subcommand matched.
pub const SHELL_COMP_NO_SUBCOMMAND: usize = 0xFFFFFFFF;

/// Error set for shell completion script generation, evaluation, and RC planning.
pub err ShellCompError {
    UnsupportedShell,
    InvalidCursorIndex,
    SliceCapacityExceeded,
    BufferTooSmall,
    EmptyProgramName,
}

// ============================================================================
// Internal Zero-Alloc Byte, SIMD Case-Fold, and Formatting Helpers
// ============================================================================

fn scBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn scBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn scEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return scBytesEql(s[s.len - suffix.len..], suffix);
}

fn scAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn scAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn scAppendSingleQuoted(out: []u8, pos: *usize, s: []const u8)! {
    scAppendByte(out, pos, '\'')?;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\'') {
            scAppendBytes(out, pos, "'\\''")?;
        } else {
            scAppendByte(out, pos, s[i])?;
        }
        i += 1;
    }
    scAppendByte(out, pos, '\'')?;
}

/// Computes relevance score for `candidate` against `prefix` using `cap:simd`
/// `Simd.toLowerAscii` for fast ASCII case-folding:
///   - Empty prefix -> 50
///   - Exact case-sensitive prefix -> 100
///   - Case-insensitive prefix -> 80
///   - Case-insensitive substring -> 40
///   - No match -> 0
fn scScoreCandidate(candidate: []const u8, prefix: []const u8): u32 {
    if (prefix.len == 0) return 50;
    if (prefix.len > candidate.len) return 0;

    // Fast exact case-sensitive prefix check
    if (scBytesEql(candidate[0..prefix.len], prefix)) {
        return if (candidate.len == prefix.len) 120 else 100;
    }

    // SIMD ASCII lowercase folding into bounded stack buffers
    let low_pref: [SHELL_COMP_MAX_PREFIX_LEN]u8 = [0; SHELL_COMP_MAX_PREFIX_LEN];
    let low_cand: [SHELL_COMP_MAX_PREFIX_LEN]u8 = [0; SHELL_COMP_MAX_PREFIX_LEN];
    const plen = if (prefix.len <= SHELL_COMP_MAX_PREFIX_LEN) prefix.len else SHELL_COMP_MAX_PREFIX_LEN;
    const clen = if (candidate.len <= SHELL_COMP_MAX_PREFIX_LEN) candidate.len else SHELL_COMP_MAX_PREFIX_LEN;

    * = Simd.toLowerAscii(prefix[0..plen], low_pref[0..plen]);
    * = Simd.toLowerAscii(candidate[0..clen], low_cand[0..clen]);

    if (plen <= clen && scBytesEql(low_cand[0..plen], low_pref[0..plen])) {
        return 80;
    }

    // Substring match
    if (plen <= clen) {
        let i: usize = 1;
        while (i + plen <= clen) {
            if (scBytesEql(low_cand[i..i + plen], low_pref[0..plen])) {
                return 40;
            }
            i += 1;
        }
    }

    return 0;
}

fn scMatchesFileExtCsv(path: []const u8, ext_csv: []const u8): bool {
    if (ext_csv.len == 0) return true;
    let start: usize = 0;
    while (start <= ext_csv.len) {
        let end = start;
        while (end < ext_csv.len && ext_csv[end] != ',') {
            end += 1;
        }
        const ext = ext_csv[start..end];
        if (ext.len > 0 && scEndsWith(path, ext)) return true;
        if (end >= ext_csv.len) break;
        start = end + 1;
    }
    return false;
}

// ============================================================================
// Section 1: Declarative Completion Rules & 4-Shell Script Generator
// ============================================================================

/// Target shell dialect for completion script generation and RC installation.
pub enum ShellCompDialect: u8 {
    Bash = 0,
    Zsh = 1,
    Fish = 2,
    PowerShell = 3,
}

/// Parses a shell name (`bash`, `zsh`, `fish`, `pwsh`, `powershell`) into `ShellCompDialect`.
pub fn parseShellCompDialect(name: []const u8): ShellCompDialect? {
    let low: [16]u8 = [0; 16];
    if (name.len == 0 || name.len > low.len) return null;
    * = Simd.toLowerAscii(name, low[0..name.len]);
    const s = low[0..name.len];
    if (scBytesEql(s, "bash")) return ShellCompDialect.Bash;
    if (scBytesEql(s, "zsh")) return ShellCompDialect.Zsh;
    if (scBytesEql(s, "fish")) return ShellCompDialect.Fish;
    if (scBytesEql(s, "pwsh") || scBytesEql(s, "powershell")) return ShellCompDialect.PowerShell;
    return null;
}

/// Returns the canonical lowercase shell name for `dialect`.
pub fn shellCompDialectName(dialect: ShellCompDialect): []const u8 {
    if (dialect == ShellCompDialect.Bash) return "bash";
    if (dialect == ShellCompDialect.Zsh) return "zsh";
    if (dialect == ShellCompDialect.Fish) return "fish";
    return "powershell";
}

/// Expected argument value hint for a command positional or flag parameter.
pub enum ShellCompArgHint: u8 {
    None = 0,
    FilePath = 1,
    Directory = 2,
    ScriptName = 3,
    PackageName = 4,
    EnumValues = 5,
}

/// Declarative flag rule for a CLI subcommand.
pub struct ShellCompFlagRule {
    long_name: []const u8 = "",
    short_name: []const u8 = "",
    description: []const u8 = "",
    arg_hint: ShellCompArgHint = ShellCompArgHint.None,
    enum_values_csv: []const u8 = "",
    file_ext_filter: []const u8 = "",

    static fn empty(): ShellCompFlagRule {
        return ShellCompFlagRule{};
    }
}

/// Declarative subcommand rule referencing `flags[flags_start..flags_start + flags_len]`.
pub struct ShellCompCommandRule {
    name: []const u8 = "",
    description: []const u8 = "",
    arg_hint: ShellCompArgHint = ShellCompArgHint.None,
    file_ext_filter: []const u8 = "",
    flags_start: usize = 0,
    flags_len: usize = 0,

    static fn empty(): ShellCompCommandRule {
        return ShellCompCommandRule{};
    }
}

fn scAppendSpaceSeparatedCsv(out: []u8, pos: *usize, csv: []const u8)! {
    let start: usize = 0;
    let first = true;
    while (start <= csv.len) {
        let end = start;
        while (end < csv.len && csv[end] != ',') {
            end += 1;
        }
        const tok = csv[start..end];
        if (tok.len > 0) {
            if (!first) scAppendByte(out, pos, ' ')?;
            scAppendBytes(out, pos, tok)?;
            first = false;
        }
        if (end >= csv.len) break;
        start = end + 1;
    }
}

fn generateBashScript(
    prog: []const u8,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    out: []u8,
): ShellCompError!usize {
    let pos: usize = 0;
    scAppendBytes(out, &pos, "# bash completion for ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "\n_")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "_completions() {\n")?;
    scAppendBytes(out, &pos, "  local cur prev cmd\n")?;
    scAppendBytes(out, &pos, "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n")?;
    scAppendBytes(out, &pos, "  prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n")?;
    scAppendBytes(out, &pos, "  cmd=\"${COMP_WORDS[1]}\"\n")?;
    scAppendBytes(out, &pos, "  if [[ $COMP_CWORD -eq 1 ]]; then\n")?;
    scAppendBytes(out, &pos, "    COMPREPLY=( $(compgen -W \"")?;

    let i: usize = 0;
    while (i < commands.len) {
        if (i > 0) scAppendByte(out, &pos, ' ')?;
        scAppendBytes(out, &pos, commands[i].name)?;
        i += 1;
    }
    scAppendBytes(out, &pos, "\" -- \"$cur\") )\n    return 0\n  fi\n")?;
    scAppendBytes(out, &pos, "  case \"$cmd\" in\n")?;

    let c: usize = 0;
    while (c < commands.len) {
        const cmd = commands[c];
        scAppendBytes(out, &pos, "    ")?;
        scAppendBytes(out, &pos, cmd.name)?;
        scAppendBytes(out, &pos, ")\n")?;
        scAppendBytes(out, &pos, "      COMPREPLY=( $(compgen -W \"")?;
        let f: usize = 0;
        let first_f = true;
        while (f < cmd.flags_len && cmd.flags_start + f < flags.len) {
            const fl = flags[cmd.flags_start + f];
            if (fl.long_name.len > 0) {
                if (!first_f) scAppendByte(out, &pos, ' ')?;
                scAppendBytes(out, &pos, fl.long_name)?;
                first_f = false;
            }
            if (fl.short_name.len > 0) {
                if (!first_f) scAppendByte(out, &pos, ' ')?;
                scAppendBytes(out, &pos, fl.short_name)?;
                first_f = false;
            }
            f += 1;
        }
        scAppendBytes(out, &pos, "\" -- \"$cur\") )\n")?;
        if (cmd.arg_hint == ShellCompArgHint.FilePath) {
            scAppendBytes(out, &pos, "      [[ \"$cur\" != -* ]] && COMPREPLY+=( $(compgen -f -- \"$cur\") )\n")?;
        }
        scAppendBytes(out, &pos, "      ;;\n")?;
        c += 1;
    }

    scAppendBytes(out, &pos, "  esac\n}\ncomplete -F _")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "_completions ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendByte(out, &pos, '\n')?;
    return pos;
}

fn generateZshScript(
    prog: []const u8,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    out: []u8,
): ShellCompError!usize {
    let pos: usize = 0;
    scAppendBytes(out, &pos, "#compdef ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "\n_")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "() {\n  local -a commands\n  commands=(\n")?;

    let i: usize = 0;
    while (i < commands.len) {
        const cmd = commands[i];
        scAppendBytes(out, &pos, "    '")?;
        scAppendBytes(out, &pos, cmd.name)?;
        scAppendByte(out, &pos, ':')?;
        scAppendBytes(out, &pos, cmd.description)?;
        scAppendBytes(out, &pos, "'\n")?;
        i += 1;
    }
    scAppendBytes(out, &pos, "  )\n  _arguments -C '1: :->cmd' '*::arg:->args'\n")?;
    scAppendBytes(out, &pos, "  case \"$state\" in\n    cmd)\n      _describe 'command' commands\n      ;;\n    args)\n      case \"$line[1]\" in\n")?;

    let c: usize = 0;
    while (c < commands.len) {
        const cmd = commands[c];
        scAppendBytes(out, &pos, "        ")?;
        scAppendBytes(out, &pos, cmd.name)?;
        scAppendBytes(out, &pos, ")\n          _arguments \\\n")?;
        let f: usize = 0;
        while (f < cmd.flags_len && cmd.flags_start + f < flags.len) {
            const fl = flags[cmd.flags_start + f];
            scAppendBytes(out, &pos, "            '")?;
            scAppendBytes(out, &pos, fl.long_name)?;
            scAppendByte(out, &pos, '[')?;
            scAppendBytes(out, &pos, fl.description)?;
            scAppendByte(out, &pos, ']')?;
            if (fl.arg_hint == ShellCompArgHint.EnumValues && fl.enum_values_csv.len > 0) {
                scAppendBytes(out, &pos, ":value:(")?;
                scAppendSpaceSeparatedCsv(out, &pos, fl.enum_values_csv)?;
                scAppendByte(out, &pos, ')')?;
            }
            scAppendBytes(out, &pos, "' \\\n")?;
            f += 1;
        }
        scAppendBytes(out, &pos, "            '*:file:_files'\n          ;;\n")?;
        c += 1;
    }

    scAppendBytes(out, &pos, "      esac\n      ;;\n  esac\n}\ncompdef _")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendByte(out, &pos, ' ')?;
    scAppendBytes(out, &pos, prog)?;
    scAppendByte(out, &pos, '\n')?;
    return pos;
}

fn generateFishScript(
    prog: []const u8,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    out: []u8,
): ShellCompError!usize {
    let pos: usize = 0;
    scAppendBytes(out, &pos, "# fish completion for ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendByte(out, &pos, '\n')?;

    let i: usize = 0;
    while (i < commands.len) {
        const cmd = commands[i];
        scAppendBytes(out, &pos, "complete -c ")?;
        scAppendBytes(out, &pos, prog)?;
        scAppendBytes(out, &pos, " -n '__fish_use_subcommand' -a ")?;
        scAppendBytes(out, &pos, cmd.name)?;
        scAppendBytes(out, &pos, " -d ")?;
        scAppendSingleQuoted(out, &pos, cmd.description)?;
        scAppendByte(out, &pos, '\n')?;

        let f: usize = 0;
        while (f < cmd.flags_len && cmd.flags_start + f < flags.len) {
            const fl = flags[cmd.flags_start + f];
            scAppendBytes(out, &pos, "complete -c ")?;
            scAppendBytes(out, &pos, prog)?;
            scAppendBytes(out, &pos, " -n '__fish_seen_subcommand_from ")?;
            scAppendBytes(out, &pos, cmd.name)?;
            scAppendByte(out, &pos, '\'')?;
            if (fl.long_name.len > 2 && fl.long_name[0] == '-' && fl.long_name[1] == '-') {
                scAppendBytes(out, &pos, " -l ")?;
                scAppendBytes(out, &pos, fl.long_name[2..])?;
            }
            if (fl.short_name.len == 2 && fl.short_name[0] == '-') {
                scAppendBytes(out, &pos, " -s ")?;
                scAppendByte(out, &pos, fl.short_name[1])?;
            }
            if (fl.arg_hint == ShellCompArgHint.EnumValues && fl.enum_values_csv.len > 0) {
                scAppendBytes(out, &pos, " -r -a '")?;
                scAppendSpaceSeparatedCsv(out, &pos, fl.enum_values_csv)?;
                scAppendByte(out, &pos, '\'')?;
            }
            scAppendBytes(out, &pos, " -d ")?;
            scAppendSingleQuoted(out, &pos, fl.description)?;
            scAppendByte(out, &pos, '\n')?;
            f += 1;
        }
        i += 1;
    }

    return pos;
}

fn generatePowerShellScript(
    prog: []const u8,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    out: []u8,
): ShellCompError!usize {
    let pos: usize = 0;
    scAppendBytes(out, &pos, "# PowerShell completion for ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, "\nRegister-ArgumentCompleter -Native -CommandName ")?;
    scAppendBytes(out, &pos, prog)?;
    scAppendBytes(out, &pos, " -ScriptBlock {\n  param($wordToComplete, $commandAst, $cursorPosition)\n")?;
    scAppendBytes(out, &pos, "  $commands = @(\n")?;

    let i: usize = 0;
    while (i < commands.len) {
        const cmd = commands[i];
        scAppendBytes(out, &pos, "    @{ Name = '")?;
        scAppendBytes(out, &pos, cmd.name)?;
        scAppendBytes(out, &pos, "'; Desc = '")?;
        scAppendBytes(out, &pos, cmd.description)?;
        scAppendBytes(out, &pos, "' }\n")?;
        i += 1;
    }
    scAppendBytes(out, &pos, "  )\n  $commands | Where-Object { $_.Name -like \"$wordToComplete*\" } | ForEach-Object {\n")?;
    scAppendBytes(out, &pos, "    [System.Management.Automation.CompletionResult]::new($_.Name, $_.Name, 'ParameterValue', $_.Desc)\n")?;
    scAppendBytes(out, &pos, "  }\n}\n")?;
    * = flags;
    return pos;
}

/// Generates a complete, ready-to-source shell completion script for `dialect`
/// (Bash, Zsh, Fish, or PowerShell) into caller-owned `out_buf`.
pub fn generateCapShellCompletion(
    dialect: ShellCompDialect,
    program_name: []const u8,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    out_buf: []u8,
): ShellCompError!usize {
    if (program_name.len == 0) return err.EmptyProgramName;
    if (dialect == ShellCompDialect.Bash) {
        return generateBashScript(program_name, commands, flags, out_buf);
    }
    if (dialect == ShellCompDialect.Zsh) {
        return generateZshScript(program_name, commands, flags, out_buf);
    }
    if (dialect == ShellCompDialect.Fish) {
        return generateFishScript(program_name, commands, flags, out_buf);
    }
    return generatePowerShellScript(program_name, commands, flags, out_buf);
}

// ============================================================================
// Section 2: Real-Time Dynamic Context Completion Evaluator
// ============================================================================

/// Semantic category of a dynamic completion candidate.
pub enum ShellCompCandidateKind: u8 {
    Subcommand = 0,
    Flag = 1,
    EnumValue = 2,
    Script = 3,
    Package = 4,
    FilePath = 5,
}

/// Ranked completion suggestion emitted by `computeDynamicCompletions`.
pub struct ShellCompCandidate {
    value: []const u8 = "",
    description: []const u8 = "",
    kind: ShellCompCandidateKind = ShellCompCandidateKind.Subcommand,
    score: u32 = 0,

    static fn empty(): ShellCompCandidate {
        return ShellCompCandidate{};
    }
}

/// Live project context fed into `computeDynamicCompletions`.
pub struct ShellCompProjectContext {
    scripts: []const []const u8 = [],
    packages: []const []const u8 = [],
    files: []const []const u8 = [],
}

/// Summary returned by `computeDynamicCompletions`.
pub struct ShellCompEvalSummary {
    candidate_count: usize = 0,
    truncated: bool = false,
    active_subcommand_index: usize = SHELL_COMP_NO_SUBCOMMAND,
}

fn pushCandidateIfMatched(
    value: []const u8,
    desc: []const u8,
    kind: ShellCompCandidateKind,
    prefix: []const u8,
    out_candidates: []ShellCompCandidate,
    count: *usize,
    truncated: *bool,
) {
    if (value.len == 0) return;
    const score = scScoreCandidate(value, prefix);
    if (score == 0) return;
    if (count.* >= out_candidates.len) {
        truncated.* = true;
        return;
    }
    out_candidates[count.*] = ShellCompCandidate{
        value: value,
        description: desc,
        kind: kind,
        score: score,
    };
    count.* += 1;
}

fn pushEnumCsvCandidates(
    csv: []const u8,
    prefix: []const u8,
    out_candidates: []ShellCompCandidate,
    count: *usize,
    truncated: *bool,
) {
    let start: usize = 0;
    while (start <= csv.len) {
        let end = start;
        while (end < csv.len && csv[end] != ',') {
            end += 1;
        }
        const tok = csv[start..end];
        if (tok.len > 0) {
            pushCandidateIfMatched(tok, "enum value", ShellCompCandidateKind.EnumValue, prefix, out_candidates, count, truncated);
        }
        if (end >= csv.len) break;
        start = end + 1;
    }
}

fn findFlagInCommand(
    cmd: ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    flag_token: []const u8,
): usize? {
    let f: usize = 0;
    while (f < cmd.flags_len && cmd.flags_start + f < flags.len) {
        const fl = flags[cmd.flags_start + f];
        if ((fl.long_name.len > 0 && scBytesEql(fl.long_name, flag_token)) ||
            (fl.short_name.len > 0 && scBytesEql(fl.short_name, flag_token)))
        {
            return cmd.flags_start + f;
        }
        f += 1;
    }
    return null;
}

fn pushContextByHint(
    hint: ShellCompArgHint,
    enum_csv: []const u8,
    ext_filter: []const u8,
    prefix: []const u8,
    ctx: ShellCompProjectContext,
    out_candidates: []ShellCompCandidate,
    count: *usize,
    truncated: *bool,
) {
    if (hint == ShellCompArgHint.EnumValues) {
        pushEnumCsvCandidates(enum_csv, prefix, out_candidates, count, truncated);
        return;
    }
    if (hint == ShellCompArgHint.ScriptName) {
        let i: usize = 0;
        while (i < ctx.scripts.len) {
            pushCandidateIfMatched(ctx.scripts[i], "script", ShellCompCandidateKind.Script, prefix, out_candidates, count, truncated);
            i += 1;
        }
        return;
    }
    if (hint == ShellCompArgHint.PackageName) {
        let i: usize = 0;
        while (i < ctx.packages.len) {
            pushCandidateIfMatched(ctx.packages[i], "package", ShellCompCandidateKind.Package, prefix, out_candidates, count, truncated);
            i += 1;
        }
        return;
    }
    if (hint == ShellCompArgHint.FilePath || hint == ShellCompArgHint.Directory) {
        let i: usize = 0;
        while (i < ctx.files.len) {
            const fp = ctx.files[i];
            if (scMatchesFileExtCsv(fp, ext_filter)) {
                pushCandidateIfMatched(fp, "file", ShellCompCandidateKind.FilePath, prefix, out_candidates, count, truncated);
            }
            i += 1;
        }
        return;
    }
}

/// Evaluates real-time dynamic completions for `tokens` at `cursor_index` (0 = program
/// name, 1 = subcommand, >=2 = subcommand args/flags) using `cap:simd` case-folding
/// and deterministic relevance + lexicographical ranking.
pub fn computeDynamicCompletions(
    tokens: []const []const u8,
    cursor_index: usize,
    commands: []const ShellCompCommandRule,
    flags: []const ShellCompFlagRule,
    ctx: ShellCompProjectContext,
    out_candidates: []ShellCompCandidate,
): ShellCompError!ShellCompEvalSummary {
    let summary = ShellCompEvalSummary{};
    if (tokens.len == 0 || cursor_index > tokens.len) {
        return err.InvalidCursorIndex;
    }

    const cur_prefix: []const u8 = if (cursor_index < tokens.len) tokens[cursor_index] else "";
    let count: usize = 0;
    let truncated = false;

    // Top-level subcommand completion (cursor at index 0 or 1)
    if (cursor_index <= 1) {
        let i: usize = 0;
        while (i < commands.len) {
            pushCandidateIfMatched(
                commands[i].name,
                commands[i].description,
                ShellCompCandidateKind.Subcommand,
                cur_prefix,
                out_candidates,
                &count,
                &truncated,
            );
            i += 1;
        }
    } else {
        // Resolve active subcommand from tokens[1]
        const cmd_tok = tokens[1];
        let cmd_idx: usize = SHELL_COMP_NO_SUBCOMMAND;
        let i: usize = 0;
        while (i < commands.len) {
            if (scBytesEql(commands[i].name, cmd_tok)) {
                cmd_idx = i;
                break;
            }
            i += 1;
        }
        summary.active_subcommand_index = cmd_idx;

        if (cmd_idx != SHELL_COMP_NO_SUBCOMMAND) {
            const cmd = commands[cmd_idx];

            // Case A: `--flag=value` inline value completion
            let eq_idx_opt: usize? = null;
            if (cur_prefix.len > 2 && cur_prefix[0] == '-') {
                eq_idx_opt = Simd.indexOfByte(cur_prefix, '=');
            }

            if (eq_pos := eq_idx_opt) {
                const flag_part = cur_prefix[0..eq_pos];
                const val_part = cur_prefix[eq_pos + 1..];
                if (fidx := findFlagInCommand(cmd, flags, flag_part)) {
                    const fl = flags[fidx];
                    pushContextByHint(fl.arg_hint, fl.enum_values_csv, fl.file_ext_filter, val_part, ctx, out_candidates, &count, &truncated);
                }
            } else {
                // Case B: Previous token is a value-taking flag (e.g. `--target <tab>`)
                const prev_tok = tokens[cursor_index - 1];
                let handled_prev_flag = false;
                if (prev_tok.len > 1 && prev_tok[0] == '-') {
                    if (fidx := findFlagInCommand(cmd, flags, prev_tok)) {
                        const fl = flags[fidx];
                        if (fl.arg_hint != ShellCompArgHint.None) {
                            pushContextByHint(fl.arg_hint, fl.enum_values_csv, fl.file_ext_filter, cur_prefix, ctx, out_candidates, &count, &truncated);
                            handled_prev_flag = true;
                        }
                    }
                }

                if (!handled_prev_flag) {
                    if (cur_prefix.len > 0 && cur_prefix[0] == '-') {
                        // Case C: Flag completion for active subcommand
                        let f: usize = 0;
                        while (f < cmd.flags_len && cmd.flags_start + f < flags.len) {
                            const fl = flags[cmd.flags_start + f];
                            if (fl.long_name.len > 0) {
                                pushCandidateIfMatched(fl.long_name, fl.description, ShellCompCandidateKind.Flag, cur_prefix, out_candidates, &count, &truncated);
                            }
                            if (fl.short_name.len > 0) {
                                pushCandidateIfMatched(fl.short_name, fl.description, ShellCompCandidateKind.Flag, cur_prefix, out_candidates, &count, &truncated);
                            }
                            f += 1;
                        }
                    } else {
                        // Case D: Subcommand positional argument completion (scripts/packages/files)
                        pushContextByHint(cmd.arg_hint, "", cmd.file_ext_filter, cur_prefix, ctx, out_candidates, &count, &truncated);
                    }
                }
            }
        }
    }

    // Sort candidates by score descending, then value lexicographically ascending
    let s: usize = 1;
    while (s < count) {
        const key = out_candidates[s];
        let j = s;
        while (j > 0) {
            const prev = out_candidates[j - 1];
            const should_swap = if (key.score != prev.score)
                key.score > prev.score
            else
                scBytesLessThan(key.value, prev.value);
            if (!should_swap) break;
            out_candidates[j] = prev;
            j -= 1;
        }
        out_candidates[j] = key;
        s += 1;
    }

    summary.candidate_count = count;
    summary.truncated = truncated;
    return summary;
}

// ============================================================================
// Section 3: Idempotent RC File Completion Block Installer / Uninstaller
// ============================================================================

/// Requested RC file modification operation.
pub enum RcCompOperation: u8 {
    InstallOrUpdate = 0,
    Uninstall = 1,
}

/// Resulting edit action planned by `planRcCompletionInstall`.
pub enum RcCompAction: u8 {
    NoChange = 0,
    AppendBlock = 1,
    ReplaceBlock = 2,
    RemoveBlock = 3,
}

/// Outcome of `planRcCompletionInstall`.
pub struct RcCompPlanResult {
    action: RcCompAction = RcCompAction.NoChange,
    rc_path_hint: []const u8 = "",
    existing_block_found: bool = false,
    bytes_written: usize = 0,
}

fn scRcPathHint(dialect: ShellCompDialect): []const u8 {
    if (dialect == ShellCompDialect.Bash) return "~/.bashrc";
    if (dialect == ShellCompDialect.Zsh) return "~/.zshrc";
    if (dialect == ShellCompDialect.Fish) return "~/.config/fish/config.fish";
    return "$PROFILE";
}

fn scFindMarkerLine(src: []const u8, start_from: usize, marker: []const u8): usize? {
    let pos = start_from;
    while (pos < src.len) {
        const rel_opt = Simd.indexOfByte(src[pos..], '#');
        let rel: usize = 0;
        if (v := rel_opt) {
            rel = v;
        } else {
            return null;
        }
        const idx = pos + rel;
        const at_line_start = (idx == 0) || (src[idx - 1] == '\n');
        if (at_line_start && idx + marker.len <= src.len && scBytesEql(src[idx..idx + marker.len], marker)) {
            return idx;
        }
        pos = idx + 1;
    }
    return null;
}

fn scFormatManagedBlock(
    dialect: ShellCompDialect,
    prog: []const u8,
    out_buf: []u8,
): ShellCompError!usize {
    let pos: usize = 0;
    scAppendBytes(out_buf, &pos, SHELL_COMP_MARKER_BEGIN)?;
    scAppendByte(out_buf, &pos, '\n')?;

    if (dialect == ShellCompDialect.Bash) {
        scAppendBytes(out_buf, &pos, "eval \"$(")?;
        scAppendBytes(out_buf, &pos, prog)?;
        scAppendBytes(out_buf, &pos, " completions bash)\"\n")?;
    } else if (dialect == ShellCompDialect.Zsh) {
        scAppendBytes(out_buf, &pos, "eval \"$(")?;
        scAppendBytes(out_buf, &pos, prog)?;
        scAppendBytes(out_buf, &pos, " completions zsh)\"\n")?;
    } else if (dialect == ShellCompDialect.Fish) {
        scAppendBytes(out_buf, &pos, prog)?;
        scAppendBytes(out_buf, &pos, " completions fish | source\n")?;
    } else {
        scAppendBytes(out_buf, &pos, "Invoke-Expression (& ")?;
        scAppendBytes(out_buf, &pos, prog)?;
        scAppendBytes(out_buf, &pos, " completions powershell | Out-String)\n")?;
    }

    scAppendBytes(out_buf, &pos, SHELL_COMP_MARKER_END)?;
    scAppendByte(out_buf, &pos, '\n')?;
    return pos;
}

/// Plans an idempotent RC file update for `dialect` (`~/.bashrc`, `~/.zshrc`,
/// `~/.config/fish/config.fish`, `$PROFILE`).
///   - Detects existing `# >>> cap completions >>> ... # <<< cap completions <<<` blocks;
///   - `InstallOrUpdate`: appends block if absent, returns `NoChange` if identical,
///     or replaces in-place if outdated;
///   - `Uninstall`: removes existing block cleanly or returns `NoChange` if absent.
pub fn planRcCompletionInstall(
    dialect: ShellCompDialect,
    program_name: []const u8,
    existing_rc_content: []const u8,
    op: RcCompOperation,
    out_rc_buf: []u8,
): ShellCompError!RcCompPlanResult {
    if (program_name.len == 0) return err.EmptyProgramName;

    let res = RcCompPlanResult{
        action: RcCompAction.NoChange,
        rc_path_hint: scRcPathHint(dialect),
        existing_block_found: false,
        bytes_written: 0,
    };

    let block_buf: [256]u8 = [0; 256];
    const block_len = scFormatManagedBlock(dialect, program_name, block_buf[0..])?;
    const desired_block = block_buf[0..block_len];

    // Locate existing managed block if present
    let block_start: usize = 0;
    let block_end: usize = 0;
    if (b_start := scFindMarkerLine(existing_rc_content, 0, SHELL_COMP_MARKER_BEGIN)) {
        if (b_end_line := scFindMarkerLine(existing_rc_content, b_start + SHELL_COMP_MARKER_BEGIN.len, SHELL_COMP_MARKER_END)) {
            let end_pos = b_end_line + SHELL_COMP_MARKER_END.len;
            if (end_pos < existing_rc_content.len && existing_rc_content[end_pos] == '\r') end_pos += 1;
            if (end_pos < existing_rc_content.len && existing_rc_content[end_pos] == '\n') end_pos += 1;
            block_start = b_start;
            block_end = end_pos;
            res.existing_block_found = true;
        }
    }

    let pos: usize = 0;

    if (op == RcCompOperation.Uninstall) {
        if (!res.existing_block_found) {
            scAppendBytes(out_rc_buf, &pos, existing_rc_content)?;
            res.action = RcCompAction.NoChange;
            res.bytes_written = pos;
            return res;
        }
        scAppendBytes(out_rc_buf, &pos, existing_rc_content[0..block_start])?;
        scAppendBytes(out_rc_buf, &pos, existing_rc_content[block_end..])?;
        res.action = RcCompAction.RemoveBlock;
        res.bytes_written = pos;
        return res;
    }

    // InstallOrUpdate
    if (res.existing_block_found) {
        const existing_block = existing_rc_content[block_start..block_end];
        if (scBytesEql(existing_block, desired_block)) {
            scAppendBytes(out_rc_buf, &pos, existing_rc_content)?;
            res.action = RcCompAction.NoChange;
            res.bytes_written = pos;
            return res;
        }
        scAppendBytes(out_rc_buf, &pos, existing_rc_content[0..block_start])?;
        scAppendBytes(out_rc_buf, &pos, desired_block)?;
        scAppendBytes(out_rc_buf, &pos, existing_rc_content[block_end..])?;
        res.action = RcCompAction.ReplaceBlock;
        res.bytes_written = pos;
        return res;
    }

    // Append new block (ensuring a leading newline if existing file is non-empty without trailing '\n')
    scAppendBytes(out_rc_buf, &pos, existing_rc_content)?;
    if (existing_rc_content.len > 0 && existing_rc_content[existing_rc_content.len - 1] != '\n') {
        scAppendByte(out_rc_buf, &pos, '\n')?;
    }
    scAppendBytes(out_rc_buf, &pos, desired_block)?;
    res.action = RcCompAction.AppendBlock;
    res.bytes_written = pos;
    return res;
}
