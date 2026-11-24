// Source: Absorbed from /data/dev/bun/src/shell/states/Expansion.cp | Synced: 2026-11-24T06:13:09Z
// Pure-CP zero-allocation POSIX / Bun Shell Word Expansion State Machine:
// - Handles literals, quoted literals, `$VAR`/`"$VAR"`, leading tilde (`~` / `~/...`),
//   command substitutions `$(...)` / `"$(...)"`, and unquoted glob trigger detection
// - Preserves quoted empty strings (`has_quoted_empty`, e.g. `""` -> 1 empty arg, unquoted `$EMPTY` -> 0 args)
// - Implements Bun `postSubshellExpansion` (newline-to-space folding, whitespace trimming, and word splitting)
// - Supports both `Single` (assignment/redirect target) and `Array` (command argv) output modes
// - Zero heap allocation with step-by-step `step()` / `onCmdSubstDone()` / `onGlobDone()` driving.

/// 单次词展开支持的最大 Atom 片段数
pub const WORD_EXPAND_MAX_ATOMS: usize = 32;
/// 单次词展开支持的最大输出词条数
pub const WORD_EXPAND_MAX_WORDS: usize = 32;
/// 单次词展开内联字符缓冲容量（字节）
pub const WORD_EXPAND_ARENA_CAP: usize = 2048;
/// 内置环境变量查找表最大条目数
pub const WORD_EXPAND_MAX_VARS: usize = 32;

/// Shell 词展开错误集
pub err WordExpandError {
    TooManyAtoms,
    TooManyWords,
    TooManyVars,
    ArenaFull,
    AwaitingCmdSubst,
    NotAwaitingCmdSubst,
    AwaitingGlob,
    NotAwaitingGlob,
    BufferTooSmall,
}

/// 词展开输出模式（对齐 Bun `Expansion.Result`: `single` vs `array`）
pub enum WordExpandMode: u8 {
    Single = 0,
    Array = 1,
}

/// 组成单个 Shell 词的原子片段类型
pub enum WordAtomKind: u8 {
    Literal = 0,
    QuotedLiteral = 1,
    Var = 2,
    QuotedVar = 3,
    Tilde = 4,
    CmdSubst = 5,
    QuotedCmdSubst = 6,
    GlobPattern = 7,
}

/// 步进状态机返回的下一步动作
pub enum WordExpandStepKind: u8 {
    NeedCmdSubst = 0,
    NeedGlobWalk = 1,
    Done = 2,
}

/// 环境变量键值绑定
pub struct WordVarBinding {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// 单个待展开词片段
pub struct WordAtom {
    kind: WordAtomKind = .Literal,
    text: []const u8 = "",
    cmd_id: u32 = 0,
}

/// `step()` 返回的单步控制描述符
pub struct WordExpandStep {
    kind: WordExpandStepKind = .Done,
    atom_index: usize = 0,
    cmd_id: u32 = 0,
    quoted: bool = false,
    pattern_offset: usize = 0,
    pattern_len: usize = 0,
}

/// 输出缓冲中的单个词切片偏移
pub struct WordSpan {
    offset: usize = 0,
    len: usize = 0,
}

/// 零堆分配词展开输出容器
pub struct WordExpandOutput {
    arena: [WORD_EXPAND_ARENA_CAP]u8 = [0; WORD_EXPAND_ARENA_CAP],
    arena_len: usize = 0,
    words: [WORD_EXPAND_MAX_WORDS]WordSpan = [WordSpan{}; WORD_EXPAND_MAX_WORDS],
    word_count: usize = 0,

    static fn init(): WordExpandOutput {
        return WordExpandOutput{};
    }

    fn reset(*_) {
        _.arena_len = 0;
        _.word_count = 0;
    }

    fn wordCount(): usize {
        return _.word_count;
    }

    fn wordAt(idx: usize): []const u8 {
        if (idx >= _.word_count) return "";
        const sp = _.words[idx];
        return _.arena[sp.offset .. sp.offset + sp.len];
    }

    fn pushWord(*_, s: []const u8): usize! {
        if (_.word_count >= WORD_EXPAND_MAX_WORDS) {
            return err.TooManyWords;
        }
        if (s.len > WORD_EXPAND_ARENA_CAP - _.arena_len) {
            return err.ArenaFull;
        }
        const off = _.arena_len;
        copyBytes(_.arena[off .. off + s.len], s);
        _.arena_len += s.len;
        _.words[_.word_count] = WordSpan{ offset: off, len: s.len };
        _.word_count += 1;
        return _.word_count;
    }
}

/// POSIX 命令替换规范：剥离输出末尾所有连续 `\n`
pub fn stripTrailingNewlines(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == '\n') {
        end -= 1;
    }
    return s[0..end];
}

/// 检测字符串是否包含未转义的 Glob 元字符（`*` / `?` / `[`）
pub fn hasUnquotedGlobMeta(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '*' || c == '?' || c == '[') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 对齐 Bun `Expansion.convertNewlinesToSpaces` + `trim(" \n\r\t")`：
/// 将非尾随 `\n` 替换为空格并修剪首尾空白，写入 `out` 返回长度
pub fn normalizeUnquotedSubshellStdout(raw: []const u8, out: []u8): usize! {
    const stripped = stripTrailingNewlines(raw);
    if (stripped.len == 0) return 0;

    // 先找到修剪首尾 " \n\r\t" 后的有效范围
    let begin: usize = 0;
    let end: usize = stripped.len;
    while (begin < end && isSubshellTrimWs(stripped[begin])) {
        begin += 1;
    }
    while (end > begin && isSubshellTrimWs(stripped[end - 1])) {
        end -= 1;
    }
    const len = end - begin;
    if (len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        const c = stripped[begin + i];
        out[i] = if (c == '\n') ' ' else c;
        i += 1;
    }
    return len;
}

/// 对齐 Bun `Expansion.cp` 波浪号展开规则：
/// - `rest` 为空或以 `/` / `\` 开头时展开为 `home_dir + rest`
/// - 否则（如 `~user`）原样保留前导 `~` + `rest`
pub fn expandLeadingTilde(home_dir: []const u8, rest: []const u8, out: []u8): usize! {
    const should_expand = rest.len == 0 || rest[0] == '/' || rest[0] == '\\';
    if (should_expand) {
        if (home_dir.len + rest.len > out.len) {
            return err.BufferTooSmall;
        }
        copyBytes(out[0..home_dir.len], home_dir);
        copyBytes(out[home_dir.len .. home_dir.len + rest.len], rest);
        return home_dir.len + rest.len;
    }
    if (1 + rest.len > out.len) {
        return err.BufferTooSmall;
    }
    out[0] = '~';
    copyBytes(out[1 .. 1 + rest.len], rest);
    return 1 + rest.len;
}

/// 零堆分配 POSIX / Bun Shell 词展开状态机
pub struct ShellWordExpander {
    mode: WordExpandMode = .Array,
    home_dir: []const u8 = "/root",
    vars: [WORD_EXPAND_MAX_VARS]WordVarBinding = [WordVarBinding{}; WORD_EXPAND_MAX_VARS],
    var_count: usize = 0,

    atoms: [WORD_EXPAND_MAX_ATOMS]WordAtom = [WordAtom{}; WORD_EXPAND_MAX_ATOMS],
    atom_count: usize = 0,
    cursor: usize = 0,

    current: [WORD_EXPAND_ARENA_CAP]u8 = [0; WORD_EXPAND_ARENA_CAP],
    current_len: usize = 0,

    has_tilde: bool = false,
    has_quoted_empty: bool = false,
    has_unquoted_glob: bool = false,
    awaiting_cmd_subst: bool = false,
    awaiting_cmd_quoted: bool = false,
    awaiting_glob: bool = false,
    done: bool = false,

    static fn init(mode: WordExpandMode, home_dir: []const u8): ShellWordExpander {
        return ShellWordExpander{
            mode: mode,
            home_dir: home_dir,
        };
    }

    fn setVar(*_, key: []const u8, value: []const u8): usize! {
        let i: usize = 0;
        while (i < _.var_count) {
            if (bytesEql(_.vars[i].key, key)) {
                _.vars[i].value = value;
                return i;
            }
            i += 1;
        }
        if (_.var_count >= WORD_EXPAND_MAX_VARS) {
            return err.TooManyVars;
        }
        const idx = _.var_count;
        _.vars[idx] = WordVarBinding{ key: key, value: value };
        _.var_count += 1;
        return idx;
    }

    fn lookupVar(key: []const u8): []const u8 {
        let i: usize = 0;
        while (i < _.var_count) {
            if (bytesEql(_.vars[i].key, key)) {
                return _.vars[i].value;
            }
            i += 1;
        }
        return "";
    }

    fn pushAtom(*_, atom: WordAtom): usize! {
        if (_.atom_count >= WORD_EXPAND_MAX_ATOMS) {
            return err.TooManyAtoms;
        }
        const idx = _.atom_count;
        _.atoms[idx] = atom;
        _.atom_count += 1;
        return idx;
    }

    fn isDone(): bool {
        return _.done;
    }

    fn pendingGlobPattern(): []const u8 {
        return _.current[0.._.current_len];
    }

    /// 推进词展开状态机一步
    fn step(*_, out: *WordExpandOutput): WordExpandStep! {
        if (_.awaiting_cmd_subst) return err.AwaitingCmdSubst;
        if (_.awaiting_glob) return err.AwaitingGlob;
        if (_.done) return WordExpandStep{ kind: .Done };

        while (_.cursor < _.atom_count) {
            const idx = _.cursor;
            const atom = _.atoms[idx];
            switch (atom.kind) {
                .Tilde => {
                    if (idx == 0) {
                        _.has_tilde = true;
                    } else {
                        * = _.appendCurrent("~")?;
                    }
                    _.cursor += 1;
                },
                .Literal => {
                    if (hasUnquotedGlobMeta(atom.text)) {
                        _.has_unquoted_glob = true;
                    }
                    * = _.appendCurrent(atom.text)?;
                    _.cursor += 1;
                },
                .QuotedLiteral => {
                    if (atom.text.len == 0) {
                        _.has_quoted_empty = true;
                    } else {
                        * = _.appendCurrent(atom.text)?;
                    }
                    _.cursor += 1;
                },
                .Var => {
                    const val = _.lookupVar(atom.text);
                    if (val.len > 0) {
                        if (hasUnquotedGlobMeta(val)) {
                            _.has_unquoted_glob = true;
                        }
                        if (_.mode == .Array) {
                            * = _.appendWithWordSplit(val, out)?;
                        } else {
                            * = _.appendCurrent(val)?;
                        }
                    }
                    _.cursor += 1;
                },
                .QuotedVar => {
                    const val = _.lookupVar(atom.text);
                    if (val.len == 0) {
                        _.has_quoted_empty = true;
                    } else {
                        * = _.appendCurrent(val)?;
                    }
                    _.cursor += 1;
                },
                .GlobPattern => {
                    _.has_unquoted_glob = true;
                    * = _.appendCurrent(atom.text)?;
                    _.cursor += 1;
                },
                .CmdSubst => {
                    _.awaiting_cmd_subst = true;
                    _.awaiting_cmd_quoted = false;
                    _.cursor += 1;
                    return WordExpandStep{
                        kind: .NeedCmdSubst,
                        atom_index: idx,
                        cmd_id: atom.cmd_id,
                        quoted: false,
                    };
                },
                .QuotedCmdSubst => {
                    _.awaiting_cmd_subst = true;
                    _.awaiting_cmd_quoted = true;
                    _.cursor += 1;
                    return WordExpandStep{
                        kind: .NeedCmdSubst,
                        atom_index: idx,
                        cmd_id: atom.cmd_id,
                        quoted: true,
                    };
                },
            }
        }

        // 所有 atom 处理完成，执行前导波浪号展开
        if (_.has_tilde) {
            let tmp: [WORD_EXPAND_ARENA_CAP]u8 = [0; WORD_EXPAND_ARENA_CAP];
            const n = expandLeadingTilde(_.home_dir, _.current[0.._.current_len], tmp[0..])?;
            copyBytes(_.current[0..n], tmp[0..n]);
            _.current_len = n;
            _.has_tilde = false;
        }

        // Array 模式下包含未引号 Glob 元字符时挂起等待宿主目录遍历
        if (_.mode == .Array && _.has_unquoted_glob && _.current_len > 0) {
            _.awaiting_glob = true;
            return WordExpandStep{
                kind: .NeedGlobWalk,
                pattern_offset: 0,
                pattern_len: _.current_len,
            };
        }

        // 刷出最终词（注意保留 quoted empty，忽略未引号空串）
        if (_.current_len > 0 || _.has_quoted_empty) {
            * = out.pushWord(_.current[0.._.current_len])?;
            _.current_len = 0;
            _.has_quoted_empty = false;
        }

        _.done = true;
        return WordExpandStep{ kind: .Done };
    }

    /// 命令替换执行完毕后回填 stdout（对齐 Bun `postSubshellExpansion`）
    fn onCmdSubstDone(*_, stdout: []const u8, out: *WordExpandOutput): usize! {
        if (!_.awaiting_cmd_subst) return err.NotAwaitingCmdSubst;
        const quoted = _.awaiting_cmd_quoted;
        _.awaiting_cmd_subst = false;
        _.awaiting_cmd_quoted = false;

        if (quoted) {
            const stripped = stripTrailingNewlines(stdout);
            if (stripped.len == 0) {
                _.has_quoted_empty = true;
            } else {
                * = _.appendCurrent(stripped)?;
            }
            return out.word_count;
        }

        if (_.mode == .Single) {
            const stripped = stripTrailingNewlines(stdout);
            * = _.appendCurrent(stripped)?;
            return out.word_count;
        }

        // 未引号 Array 模式：换行转空格 + 首尾修剪 + 空白分词
        let norm_buf: [WORD_EXPAND_ARENA_CAP]u8 = [0; WORD_EXPAND_ARENA_CAP];
        const norm_len = normalizeUnquotedSubshellStdout(stdout, norm_buf[0..])?;
        if (norm_len == 0) {
            return out.word_count;
        }
        const norm = norm_buf[0..norm_len];
        if (hasUnquotedGlobMeta(norm)) {
            _.has_unquoted_glob = true;
        }
        return _.appendWithWordSplit(norm, out);
    }

    /// Glob 遍历完成后回填匹配路径（无匹配时按 POSIX 回退为原始模式字面量）
    fn onGlobDone(*_, matches: []const ([]const u8), out: *WordExpandOutput): usize! {
        if (!_.awaiting_glob) return err.NotAwaitingGlob;
        _.awaiting_glob = false;
        _.has_unquoted_glob = false;

        if (matches.len == 0) {
            if (_.current_len > 0) {
                * = out.pushWord(_.current[0.._.current_len])?;
            }
        } else {
            let i: usize = 0;
            while (i < matches.len) {
                * = out.pushWord(matches[i])?;
                i += 1;
            }
        }
        _.current_len = 0;
        _.done = true;
        return out.word_count;
    }

    fn appendCurrent(*_, s: []const u8): usize! {
        if (s.len > WORD_EXPAND_ARENA_CAP - _.current_len) {
            return err.ArenaFull;
        }
        copyBytes(_.current[_.current_len .. _.current_len + s.len], s);
        _.current_len += s.len;
        return _.current_len;
    }

    fn appendWithWordSplit(*_, text: []const u8, out: *WordExpandOutput): usize! {
        let prev_ws = false;
        let seg_start: usize = 0;
        let seg_end: usize = 0;
        let has_seg = false;
        let i: usize = 0;

        while (i < text.len) {
            const c = text[i];
            if (isSplitWs(c)) {
                if (has_seg) {
                    * = _.appendCurrent(text[seg_start..seg_end])?;
                    * = out.pushWord(_.current[0.._.current_len])?;
                    _.current_len = 0;
                    has_seg = false;
                } else if (!prev_ws && _.current_len > 0) {
                    * = out.pushWord(_.current[0.._.current_len])?;
                    _.current_len = 0;
                }
                prev_ws = true;
            } else {
                if (!has_seg) {
                    seg_start = i;
                    has_seg = true;
                }
                seg_end = i + 1;
                prev_ws = false;
            }
            i += 1;
        }

        if (has_seg) {
            * = _.appendCurrent(text[seg_start..seg_end])?;
        }
        return out.word_count;
    }
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn isSplitWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n';
}

fn isSubshellTrimWs(c: u8): bool {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}
