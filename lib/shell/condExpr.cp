// Source: Absorbed from /data/dev/bun/src/shell/states/CondExpr.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation Bash `[[ ... ]]` conditional expression parser &
// evaluator supporting file tests (-e/-f/-d/-c/-L/-s/-r/-w/-x), string tests
// (-z/-n/==/!=/</>), signed 64-bit integer comparisons (-eq/-ne/-lt/-le/-gt/-ge),
// logical connectives (&&, ||, !), and parenthesized grouping with depth guard.

pub const MAX_COND_DEPTH: usize = 32;

// Standard POSIX file type & permission masks (pure numeric constants, zero OS import)
pub const POSIX_S_IFMT: u32 = 0o170000;
pub const POSIX_S_IFREG: u32 = 0o100000;
pub const POSIX_S_IFDIR: u32 = 0o040000;
pub const POSIX_S_IFCHR: u32 = 0o020000;
pub const POSIX_S_IFLNK: u32 = 0o120000;
pub const POSIX_S_IRUSR: u32 = 0o000400;
pub const POSIX_S_IWUSR: u32 = 0o000200;
pub const POSIX_S_IXUSR: u32 = 0o000100;

/// Bash 条件表达式求值错误集
pub err CondExprError {
    EmptyExpression,
    UnknownOperator,
    MissingOperand,
    InvalidInteger,
    UnbalancedParens,
    UnexpectedToken,
    ExpressionTooDeep,
}

/// Bash `[[ ... ]]` 条件操作符
pub enum CondOp: u8 {
    // Unary file tests
    file_exists = 0,   // -e
    is_regular = 1,    // -f
    is_dir = 2,        // -d
    is_char_dev = 3,   // -c
    is_symlink = 4,    // -L / -h
    is_nonempty = 5,   // -s
    is_readable = 6,   // -r
    is_writable = 7,   // -w
    is_executable = 8, // -x

    // Unary string tests
    str_empty = 9,     // -z
    str_nonempty = 10, // -n

    // Binary string comparisons
    str_eq = 11,       // == / =
    str_neq = 12,      // !=
    str_lt = 13,       // <
    str_gt = 14,       // >

    // Binary integer comparisons
    int_eq = 15,       // -eq
    int_ne = 16,       // -ne
    int_lt = 17,       // -lt
    int_le = 18,       // -le
    int_gt = 19,       // -gt
    int_ge = 20,       // -ge

    static fn parse(tok: string): CondOp? {
        if (std.mem.eql(u8, tok, "-e")) { return .file_exists; }
        if (std.mem.eql(u8, tok, "-f")) { return .is_regular; }
        if (std.mem.eql(u8, tok, "-d")) { return .is_dir; }
        if (std.mem.eql(u8, tok, "-c")) { return .is_char_dev; }
        if (std.mem.eql(u8, tok, "-L") || std.mem.eql(u8, tok, "-h")) { return .is_symlink; }
        if (std.mem.eql(u8, tok, "-s")) { return .is_nonempty; }
        if (std.mem.eql(u8, tok, "-r")) { return .is_readable; }
        if (std.mem.eql(u8, tok, "-w")) { return .is_writable; }
        if (std.mem.eql(u8, tok, "-x")) { return .is_executable; }
        if (std.mem.eql(u8, tok, "-z")) { return .str_empty; }
        if (std.mem.eql(u8, tok, "-n")) { return .str_nonempty; }
        if (std.mem.eql(u8, tok, "==") || std.mem.eql(u8, tok, "=")) { return .str_eq; }
        if (std.mem.eql(u8, tok, "!=")) { return .str_neq; }
        if (std.mem.eql(u8, tok, "<")) { return .str_lt; }
        if (std.mem.eql(u8, tok, ">")) { return .str_gt; }
        if (std.mem.eql(u8, tok, "-eq")) { return .int_eq; }
        if (std.mem.eql(u8, tok, "-ne")) { return .int_ne; }
        if (std.mem.eql(u8, tok, "-lt")) { return .int_lt; }
        if (std.mem.eql(u8, tok, "-le")) { return .int_le; }
        if (std.mem.eql(u8, tok, "-gt")) { return .int_gt; }
        if (std.mem.eql(u8, tok, "-ge")) { return .int_ge; }
        return null;
    }

    fn isFileTest(): bool {
        return switch (_) {
            .file_exists, .is_regular, .is_dir, .is_char_dev, .is_symlink, .is_nonempty, .is_readable, .is_writable, .is_executable => true,
            else => false,
        };
    }

    fn isUnary(): bool {
        return _.isFileTest() || _ == .str_empty || _ == .str_nonempty;
    }

    fn isBinary(): bool {
        return !_.isUnary();
    }

    fn asStr(): string {
        return switch (_) {
            .file_exists => "-e",
            .is_regular => "-f",
            .is_dir => "-d",
            .is_char_dev => "-c",
            .is_symlink => "-L",
            .is_nonempty => "-s",
            .is_readable => "-r",
            .is_writable => "-w",
            .is_executable => "-x",
            .str_empty => "-z",
            .str_nonempty => "-n",
            .str_eq => "==",
            .str_neq => "!=",
            .str_lt => "<",
            .str_gt => ">",
            .int_eq => "-eq",
            .int_ne => "-ne",
            .int_lt => "-lt",
            .int_le => "-le",
            .int_gt => "-gt",
            .int_ge => "-ge",
        };
    }
}

/// 零依赖纯数据文件元数据快照（解耦 syscall，可离线测试或由上层接入 stat 结果）
pub struct CondFileStat {
    exists: bool = false,
    mode: u32 = 0,
    size: u64 = 0,

    static fn missing(): CondFileStat {
        return CondFileStat{ exists: false, mode: 0, size: 0 };
    }

    static fn regularFile(size: u64, perm_bits: u32): CondFileStat {
        return CondFileStat{
            exists: true,
            mode: POSIX_S_IFREG | (perm_bits & 0o7777),
            size: size,
        };
    }

    static fn directory(perm_bits: u32): CondFileStat {
        return CondFileStat{
            exists: true,
            mode: POSIX_S_IFDIR | (perm_bits & 0o7777),
            size: 0,
        };
    }

    static fn charDevice(perm_bits: u32): CondFileStat {
        return CondFileStat{
            exists: true,
            mode: POSIX_S_IFCHR | (perm_bits & 0o7777),
            size: 0,
        };
    }

    static fn symlink(perm_bits: u32): CondFileStat {
        return CondFileStat{
            exists: true,
            mode: POSIX_S_IFLNK | (perm_bits & 0o7777),
            size: 0,
        };
    }

    fn isRegular(): bool {
        return _.exists && ((_.mode & POSIX_S_IFMT) == POSIX_S_IFREG);
    }

    fn isDir(): bool {
        return _.exists && ((_.mode & POSIX_S_IFMT) == POSIX_S_IFDIR);
    }

    fn isCharDev(): bool {
        return _.exists && ((_.mode & POSIX_S_IFMT) == POSIX_S_IFCHR);
    }

    fn isSymlink(): bool {
        return _.exists && ((_.mode & POSIX_S_IFMT) == POSIX_S_IFLNK);
    }

    fn isReadable(): bool {
        return _.exists && ((_.mode & POSIX_S_IRUSR) != 0);
    }

    fn isWritable(): bool {
        return _.exists && ((_.mode & POSIX_S_IWUSR) != 0);
    }

    fn isExecutable(): bool {
        return _.exists && ((_.mode & POSIX_S_IXUSR) != 0);
    }
}

/// 可选文件元数据查询回调签名
pub type CondStatFn = fn(path: string): CondFileStat?;

fn trimAsciiSpace(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// 解析带符号 64 位整数（支持前导/尾随空白与 +/- 号，拒绝溢位与非法字符）
fn parseSignedI64(raw: string): i64! {
    const s = trimAsciiSpace(raw);
    if (s.len == 0) {
        return err.InvalidInteger;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return err.InvalidInteger;
    }
    let acc: u64 = 0;
    const limit: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return err.InvalidInteger;
        }
        const digit = (c - '0') as u64;
        if (acc > (limit - digit) / 10) {
            return err.InvalidInteger;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    if (neg) {
        if (acc == 9223372036854775808) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

fn lexCompare(a: string, b: string): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
        i += 1;
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

/// 将布尔结果转换为 POSIX Shell 退出码（true => 0, false => 1）
pub fn condToExitCode(result: bool): u8 {
    return if (result) 0 else 1;
}

/// 内部递归下降求值上下文
struct TokenCursor {
    tokens: []const string,
    pos: usize,
    stat_fn: CondStatFn?,

    fn peek(): string? {
        if (_.pos >= _.tokens.len) {
            return null;
        }
        return _.tokens[_.pos];
    }

    fn advance(*_): string? {
        if (_.pos >= _.tokens.len) {
            return null;
        }
        const t = _.tokens[_.pos];
        _.pos += 1;
        return t;
    }

    fn evalOr(*_, depth: usize): bool! {
        if (depth > MAX_COND_DEPTH) {
            return err.ExpressionTooDeep;
        }
        let left = _.evalAnd(depth + 1)?;
        while (tok := _.peek()) {
            if (!std.mem.eql(u8, tok, "||")) {
                break;
            }
            * = _.advance();
            const right = _.evalAnd(depth + 1)?;
            left = left || right;
        }
        return left;
    }

    fn evalAnd(*_, depth: usize): bool! {
        if (depth > MAX_COND_DEPTH) {
            return err.ExpressionTooDeep;
        }
        let left = _.evalNot(depth + 1)?;
        while (tok := _.peek()) {
            if (!std.mem.eql(u8, tok, "&&")) {
                break;
            }
            * = _.advance();
            const right = _.evalNot(depth + 1)?;
            left = left && right;
        }
        return left;
    }

    fn evalNot(*_, depth: usize): bool! {
        if (depth > MAX_COND_DEPTH) {
            return err.ExpressionTooDeep;
        }
        if (tok := _.peek()) {
            if (std.mem.eql(u8, tok, "!")) {
                * = _.advance();
                const inner = _.evalNot(depth + 1)?;
                return !inner;
            }
        }
        return _.evalPrimary(depth + 1);
    }

    fn evalPrimary(*_, depth: usize): bool! {
        if (depth > MAX_COND_DEPTH) {
            return err.ExpressionTooDeep;
        }
        const first = _.peek() ?? return err.MissingOperand;

        // Parenthesized sub-expression `( ... )`
        if (std.mem.eql(u8, first, "(")) {
            * = _.advance();
            const val = _.evalOr(depth + 1)?;
            const closing = _.advance() ?? return err.UnbalancedParens;
            if (!std.mem.eql(u8, closing, ")")) {
                return err.UnbalancedParens;
            }
            return val;
        }

        if (std.mem.eql(u8, first, ")") || std.mem.eql(u8, first, "&&") || std.mem.eql(u8, first, "||")) {
            return err.UnexpectedToken;
        }

        // Unary operator: `-f path`, `-z str`, `-n str`, etc.
        if (uop := CondOp.parse(first)) {
            if (uop.isUnary()) {
                * = _.advance();
                const arg = _.advance() ?? return err.MissingOperand;
                let st: CondFileStat? = null;
                if (uop.isFileTest() && arg.len > 0) {
                    if (f := _.stat_fn) {
                        st = f(arg);
                    }
                }
                return CondExprEvaluator.evalUnary(uop, arg, st);
            }
        }

        // Otherwise consume first operand and check if followed by a binary operator
        const lhs = _.advance() ?? return err.MissingOperand;
        if (next_tok := _.peek()) {
            if (bop := CondOp.parse(next_tok)) {
                if (bop.isBinary()) {
                    * = _.advance();
                    const rhs = _.advance() ?? return err.MissingOperand;
                    return CondExprEvaluator.evalBinary(bop, lhs, rhs);
                }
            }
        }

        // Bare string operand in `[[ str ]]`: true iff non-empty (equivalent to `-n str`)
        return lhs.len > 0;
    }
}

/// 纯逻辑 Bash `[[ ... ]]` 条件表达式求值器（零堆分配）
pub struct CondExprEvaluator {
    /// 单目条件求值（严格遵循 Bun `CondExpr.cp`：空路径对文件测试恒返回 `false`）
    static fn evalUnary(op: CondOp, arg: string, stat_opt: CondFileStat?): bool! {
        if (!op.isUnary()) {
            return err.UnknownOperator;
        }
        if (op == .str_empty) {
            return arg.len == 0;
        }
        if (op == .str_nonempty) {
            return arg.len > 0;
        }
        // Explicit empty path guard (matches Bun CondExpr.cp Windows stat("") protection)
        if (arg.len == 0) {
            return false;
        }
        const st = stat_opt ?? return false;
        if (!st.exists) {
            return false;
        }
        return switch (op) {
            .file_exists => true,
            .is_regular => st.isRegular(),
            .is_dir => st.isDir(),
            .is_char_dev => st.isCharDev(),
            .is_symlink => st.isSymlink(),
            .is_nonempty => st.size > 0,
            .is_readable => st.isReadable(),
            .is_writable => st.isWritable(),
            .is_executable => st.isExecutable(),
            else => err.UnknownOperator,
        };
    }

    /// 双目条件求值（字符串比较与 64 位有符号整数比较）
    static fn evalBinary(op: CondOp, lhs: string, rhs: string): bool! {
        if (!op.isBinary()) {
            return err.UnknownOperator;
        }
        return switch (op) {
            .str_eq => std.mem.eql(u8, lhs, rhs),
            .str_neq => !std.mem.eql(u8, lhs, rhs),
            .str_lt => lexCompare(lhs, rhs) < 0,
            .str_gt => lexCompare(lhs, rhs) > 0,
            .int_eq => (parseSignedI64(lhs)?) == (parseSignedI64(rhs)?),
            .int_ne => (parseSignedI64(lhs)?) != (parseSignedI64(rhs)?),
            .int_lt => (parseSignedI64(lhs)?) < (parseSignedI64(rhs)?),
            .int_le => (parseSignedI64(lhs)?) <= (parseSignedI64(rhs)?),
            .int_gt => (parseSignedI64(lhs)?) > (parseSignedI64(rhs)?),
            .int_ge => (parseSignedI64(lhs)?) >= (parseSignedI64(rhs)?),
            else => err.UnknownOperator,
        };
    }

    /// 对已分词的条件表达式 token 列表执行递归下降求值（不含外层 `[[` / `]]`）
    static fn evalTokens(tokens: []const string, stat_fn: CondStatFn?): bool! {
        if (tokens.len == 0) {
            return err.EmptyExpression;
        }
        let cursor = TokenCursor{
            tokens: tokens,
            pos: 0,
            stat_fn: stat_fn,
        };
        const result = cursor.evalOr(0)?;
        if (cursor.pos != tokens.len) {
            return err.UnexpectedToken;
        }
        return result;
    }
}
