// Source: Absorbed from /data/dev/bun/src/test_runner/debug.cp | Synced: 2026-11-22T03:19:19Z
// Pure-CP zero-allocation hierarchical test execution debug tree tracer
// (mirroring Bun test_runner/debug.cp group.begin/log/end, dumpTest, dumpDescribe, dumpOrder).

/// 调试树最大嵌套深度上限（防止畸形嵌套测试结构导致栈或缓冲异常）
pub const DEBUG_TREE_MAX_DEPTH: usize = 32;

/// 默认单层树形缩进前缀（UTF-8 制表竖线 + 空格）
pub const DEBUG_TREE_DEFAULT_INDENT: []const u8 = "│ ";

/// 调试树写入器错误集
pub err DebugTreeError {
    BufferTooSmall,
    MaxDepthExceeded,
    UnbalancedEnd,
}

/// 测试条目运行模式（对齐 Bun `TestMode` / `DescribeScope.mode`）
pub enum DebugRunMode: u8 {
    Normal = 0,
    Skip = 1,
    Todo = 2,
    Only = 3,
    Fail = 4,

    fn asStr(): []const u8 {
        return switch (_) {
            .Normal => "normal",
            .Skip => "skip",
            .Todo => "todo",
            .Only => "only",
            .Fail => "fail",
        };
    }
}

/// 生命周期钩子与测试执行阶段（对齐 Bun `docs/test/lifecycle.mdx`）
pub enum DebugHookPhase: u8 {
    BeforeAll = 0,
    BeforeEach = 1,
    Test = 2,
    AfterEach = 3,
    AfterAll = 4,
    OnTestFinished = 5,

    fn asStr(): []const u8 {
        return switch (_) {
            .BeforeAll => "beforeAll",
            .BeforeEach => "beforeEach",
            .Test => "test",
            .AfterEach => "afterEach",
            .AfterAll => "afterAll",
            .OnTestFinished => "onTestFinished",
        };
    }
}

/// 单个测试或 `describe` 作用域的调试元信息
pub struct DebugEntryMeta {
    name: []const u8 = "",
    label: []const u8 = "test",
    concurrent: bool = false,
    only: bool = false,
    mode: DebugRunMode = .Normal,
    has_callback: bool = true,
}

/// 并发分组与序列调度元信息
pub struct DebugSequenceMeta {
    index: usize = 0,
    repeat_count: u32 = 1,
    concurrent_group_id: usize = 0,
    sequence_start: usize = 0,
    sequence_end: usize = 0,
}

const ANSI_GRAY: []const u8 = "\x1b[90m";
const ANSI_GREEN: []const u8 = "\x1b[32m";
const ANSI_CYAN: []const u8 = "\x1b[36m";
const ANSI_WHITE: []const u8 = "\x1b[37m";
const ANSI_BRIGHT_YELLOW: []const u8 = "\x1b[93m";
const ANSI_YELLOW: []const u8 = "\x1b[33m";
const ANSI_MAGENTA: []const u8 = "\x1b[35m";
const ANSI_RESET: []const u8 = "\x1b[m";

/// 零动态堆分配的树形缩进测试调试追踪器
///
/// 精确复刻 Bun `test_runner/debug.cp` 的 `group.begin` / `group.log` / `group.end` 状态机：
/// - 每进入一层分组，`depth` 自增并置 `last_was_start = true`；
/// - 若分组内未产生任何 `log`（即空分组），`end()` 自动抑制冗余的 `--` 闭合行；
/// - 支持 `ansi` 彩色模式与纯 ASCII/UTF-8 无色模式，全部写入调用方提供的定长切片缓冲。
pub struct DebugTreeWriter {
    buf: []u8,
    pos: usize = 0,
    indent: usize = 0,
    last_was_start: bool = false,
    enabled: bool = true,
    ansi: bool = false,

    static fn init(out: []u8, enabled: bool, ansi: bool): DebugTreeWriter {
        return DebugTreeWriter{
            buf: out,
            pos: 0,
            indent: 0,
            last_was_start: false,
            enabled: enabled,
            ansi: ansi,
        };
    }

    fn isEnabled(): bool {
        return _.enabled;
    }

    fn depth(): usize {
        return _.indent;
    }

    fn len(): usize {
        return _.pos;
    }

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn slice(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn reset(*_) {
        _.pos = 0;
        _.indent = 0;
        _.last_was_start = false;
    }

    fn writeBytes(*_, data: []const u8): bool! {
        if (_.buf.len - _.pos < data.len) {
            return err.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i += 1;
        }
        _.pos += data.len;
        return true;
    }

    fn writeByte(*_, b: u8): bool! {
        if (_.pos >= _.buf.len) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeUsize(*_, val: usize): bool! {
        let tmp: [24]u8 = [0; 24];
        let n: usize = 0;
        if (val == 0) {
            return _.writeByte('0');
        }
        let v: usize = val;
        while (v > 0) {
            tmp[n] = @as(u8, @intCast(v % 10)) + '0';
            v = v / 10;
            n += 1;
        }
        if (_.buf.len - _.pos < n) {
            return err.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < n) {
            _.buf[_.pos + i] = tmp[n - 1 - i];
            i += 1;
        }
        _.pos += n;
        return true;
    }

    fn writeBool(*_, val: bool): bool! {
        if (val) {
            return _.writeBytes("true");
        }
        return _.writeBytes("false");
    }

    fn writeEscapedQuoted(*_, raw: []const u8): bool! {
        const text = if (raw.len == 0) "(unnamed)" else raw;
        * = _.writeByte('"')?;
        let i: usize = 0;
        while (i < text.len) {
            const c = text[i];
            if (c == '"') {
                * = _.writeBytes("\\\"")?;
            } else if (c == '\\') {
                * = _.writeBytes("\\\\")?;
            } else if (c == '\n') {
                * = _.writeBytes("\\n")?;
            } else if (c == '\r') {
                * = _.writeBytes("\\r")?;
            } else if (c == '\t') {
                * = _.writeBytes("\\t")?;
            } else {
                * = _.writeByte(c)?;
            }
            i += 1;
        }
        * = _.writeByte('"')?;
        return true;
    }

    fn writeIndent(*_): bool! {
        if (_.indent == 0) {
            return true;
        }
        if (_.ansi) {
            * = _.writeBytes(ANSI_GRAY)?;
        }
        let i: usize = 0;
        while (i < _.indent) {
            * = _.writeBytes(DEBUG_TREE_DEFAULT_INDENT)?;
            i += 1;
        }
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET)?;
        }
        return true;
    }

    /// 打开一个新的缩进分组：`++ <title>`
    fn begin(*_, title: []const u8): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes(title) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeByte('\n') catch |e| {
            _.pos = saved;
            return e;
        };
        _.indent += 1;
        _.last_was_start = true;
        return true;
    }

    /// 以源码位置打开新分组（对齐 Bun `group.begin(pos)`）：`file:line:col: fn_name`
    fn beginLocation(*_, file: []const u8, line: u32, col: u32, fn_name: []const u8): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_CYAN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes(file) catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_WHITE) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeByte(':') catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_BRIGHT_YELLOW) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeUsize(@as(usize, line)) catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_WHITE) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeByte(':') catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_YELLOW) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeUsize(@as(usize, col)) catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_WHITE) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes(": ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_MAGENTA) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes(fn_name) catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeByte('\n') catch |e| {
            _.pos = saved;
            return e;
        };
        _.indent += 1;
        _.last_was_start = true;
        return true;
    }

    /// 在当前缩进层级记录一条日志
    fn log(*_, msg: []const u8): bool! {
        if (!_.enabled) {
            return false;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(msg) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeByte('\n') catch |e| {
            _.pos = saved;
            return e;
        };
        _.last_was_start = false;
        return true;
    }

    /// 结束当前缩进分组：若分组内无子日志，仅回退缩进而不打印冗余 `--`
    fn end(*_): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent == 0) {
            return err.UnbalancedEnd;
        }
        const was_start = _.last_was_start;
        _.indent -= 1;
        _.last_was_start = false;
        if (was_start) {
            return true;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            _.indent += 1;
            _.last_was_start = was_start;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                _.indent += 1;
                _.last_was_start = was_start;
                return e;
            };
        }
        * = _.writeBytes("--") catch |e| {
            _.pos = saved;
            _.indent += 1;
            _.last_was_start = was_start;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                _.indent += 1;
                _.last_was_start = was_start;
                return e;
            };
        }
        * = _.writeByte('\n') catch |e| {
            _.pos = saved;
            _.indent += 1;
            _.last_was_start = was_start;
            return e;
        };
        return true;
    }

    /// 格式化单条测试或钩子条目（对齐 Bun `dumpTest`）
    fn dumpTest(*_, entry: DebugEntryMeta): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        const lbl = if (entry.label.len == 0) "test" else entry.label;
        * = _.writeBytes(lbl) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeByte(' ') catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeEscapedQuoted(entry.name) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(" (concurrent=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.concurrent) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", only=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.only) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(")\n") catch |e| {
            _.pos = saved;
            return e;
        };
        // dumpTest 在 Bun 中立即 begin + defer end（空分组，不打印 `--`，但标记父分组已有输出）
        _.last_was_start = false;
        return true;
    }

    /// 打开一个 `describe` 作用域分组（对齐 Bun `dumpDescribe`，调用方需配对 `end()`）
    fn dumpDescribeHeader(*_, entry: DebugEntryMeta): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("describe ") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeEscapedQuoted(entry.name) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(" (concurrent=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.concurrent) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", mode=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(entry.mode.asStr()) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", only=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.only) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", has_callback=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.has_callback) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(")\n") catch |e| {
            _.pos = saved;
            return e;
        };
        _.indent += 1;
        _.last_was_start = true;
        return true;
    }

    /// 输出调度队列中的单条 `ExecutionEntry`（对齐 Bun `dumpOrder`）
    fn dumpExecutionEntry(*_, entry: DebugEntryMeta): bool! {
        if (!_.enabled) {
            return false;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes("ExecutionEntry ") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeEscapedQuoted(entry.name) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(" (concurrent=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.concurrent) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", mode=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(entry.mode.asStr()) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", only=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.only) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(", has_callback=") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBool(entry.has_callback) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(")\n") catch |e| {
            _.pos = saved;
            return e;
        };
        _.last_was_start = false;
        return true;
    }

    /// 打开并发分组头（对齐 Bun `dumpOrder`）：`<id> ConcurrentGroup (<start>-<end>)`
    fn dumpConcurrentGroupHeader(*_, meta: DebugSequenceMeta): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeUsize(meta.concurrent_group_id) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(" ConcurrentGroup (") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeUsize(meta.sequence_start) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeByte('-') catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeUsize(meta.sequence_end) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(")\n") catch |e| {
            _.pos = saved;
            return e;
        };
        _.indent += 1;
        _.last_was_start = true;
        return true;
    }

    /// 打开执行序列头（对齐 Bun `dumpOrder`）：`<index> Sequence (<repeat>x)`
    fn dumpSequenceHeader(*_, meta: DebugSequenceMeta): bool! {
        if (!_.enabled) {
            return false;
        }
        if (_.indent >= DEBUG_TREE_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const saved = _.pos;
        * = _.writeIndent() catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_GREEN) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeBytes("++ ") catch |e| {
            _.pos = saved;
            return e;
        };
        if (_.ansi) {
            * = _.writeBytes(ANSI_RESET) catch |e| {
                _.pos = saved;
                return e;
            };
        }
        * = _.writeUsize(meta.index) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes(" Sequence (") catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeUsize(@as(usize, meta.repeat_count)) catch |e| {
            _.pos = saved;
            return e;
        };
        * = _.writeBytes("x)\n") catch |e| {
            _.pos = saved;
            return e;
        };
        _.indent += 1;
        _.last_was_start = true;
        return true;
    }
}
