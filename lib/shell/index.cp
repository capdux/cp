// Source: Absorbed from /data/dev/bun/src/shell/ | Synced: 2026-11-21T17:30:39Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/ & /data/dev/bun/src/shell_parser/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/index.cp
// Unified Public Exports for CP Shell Engine (cap:shell)

const std = @import("std");
import { Parser } from "./parser.cp";
import { Executor, ShellResult, ShellSession } from "./exec.cp";

export * from "./lexer.cp";
export * from "./braces.cp";
export * from "./env.cp";
export * from "./arith.cp";
export * from "./parser.cp";
export * from "./builtin_fs.cp";
export * from "./builtin_text.cp";
export * from "./builtin.cp";
export * from "./exec.cp";

export {
    ShellEnvEntry,
    ShellEnvIter,
    getShellEnv,
} from "./envparse.cp";

export {
    needsQuoting,
    quotePosix,
    joinArgs,
    ShellArgIter,
} from "./quote.cp";

export {
    BraceExpandIter,
} from "./brace.cp";

export {
    ShellError,
    ShellToken,
    deinitTokens,
    tokenize,
} from "./posixlexer.cp";
import plex from "./posixlexer.cp";
pub const PosixTokenKind = plex.TokenKind;

export {
    RedirectKind,
    Assignment,
    SimpleCommand,
    ListOp,
    ListItem,
    ShellScript,
    parseScript,
} from "./posixast.cp";
import past from "./posixast.cp";
pub const PosixRedirect = past.Redirect;
pub const PosixPipeline = past.Pipeline;

export {
    quoteArg,
    joinCommand,
} from "./posixquote.cp";
import pquote from "./posixquote.cp";
pub const posixNeedsQuoting = pquote.needsQuoting;

export {
    EnvAssignment,
    isSafeUnquoted,
    escapeDoubleQuoted,
    posixBasename,
    posixDirname,
    isValidEnvVarName,
    splitEnvAssignment,
} from "./util.cp";
import sutil from "./util.cp";
pub const quotePosixBuf = sutil.quotePosix;
pub const joinCommandBuf = sutil.joinCommand;

export {
    MAX_COND_DEPTH,
    POSIX_S_IFMT,
    POSIX_S_IFREG,
    POSIX_S_IFDIR,
    POSIX_S_IFCHR,
    POSIX_S_IFLNK,
    POSIX_S_IRUSR,
    POSIX_S_IWUSR,
    POSIX_S_IXUSR,
    CondExprError,
    CondOp,
    CondFileStat,
    CondStatFn,
    condToExitCode,
    CondExprEvaluator,
} from "./cond_expr.cp";

export {
    EchoError,
    EchoOptions,
    EchoEscapeResult,
    expandEchoEscapes,
    formatEchoArgs,
} from "./echo.cp";

export {
    MAX_TRAMPOLINE_REENTRY,
    MAX_PIPELINE_STACK_DEPTH,
    DEFAULT_MAX_TRAMPOLINE_STEPS,
    TrampolineError,
    YieldKind,
    PipelinePhase,
    ShellYield,
    PipelineStack,
    TrampolineResult,
    ShellTrampoline,
} from "./trampoline.cp";

export {
    DEFAULT_MKDIR_MODE,
    MkdirParseError,
    parseOctalMode,
    MkdirOpts,
    ParentDirIter,
} from "./mkdir_opts.cp";

export {
    DEFAULT_SEQ_MAX_STEPS,
    SeqError,
    SeqOptions,
    SeqIter,
    formatSeq,
    formatSeqArgs,
} from "./seq.cp";

export {
    SHELL_EXIT_SUCCESS,
    SHELL_EXIT_GENERAL_ERROR,
    SHELL_EXIT_MISUSE,
    SHELL_EXIT_NOT_EXECUTABLE,
    SHELL_EXIT_NOT_FOUND,
    SHELL_EXIT_SIGNAL_BASE,
    ShellExitExportError,
    ExportEntry,
    isValidShellIdentifier,
    parseExitCodeStr,
    parseExitArgs,
    exitCodeFromSignal,
    isSignalExitCode,
    parseExportArg,
    formatExportLine,
} from "./exit_export.cp";

export {
    SHELL_CD_MAX_PATH,
    SHELL_CD_MAX_SEGMENTS,
    ShellCdError,
    CdTargetKind,
    CdTarget,
    normalizePosixPath,
    resolveCdTarget,
    formatCdError,
    ShellDirState,
} from "./cd_path.cp";

export {
    SHELL_MV_MAX_SOURCES,
    ShellMvError,
    MvOverwritePolicy,
    MvOpts,
    MvPlan,
} from "./mv_opts.cp";

export {
    SHELL_TOUCH_MAX_PATHS,
    ShellTouchError,
    TouchTimeTarget,
    TouchTimestamp,
    TouchOpts,
} from "./touch_opts.cp";

export {
    SHELL_CP_MAX_SOURCES,
    ShellCpError,
    CpOverwritePolicy,
    CpSymlinkPolicy,
    CpOpts,
    CpPlan,
} from "./cp_opts.cp";

export {
    SHELL_RM_MAX_TARGETS,
    ShellRmError,
    RmPromptBehaviour,
    RmPathKind,
    RmJoinStyle,
    RmTargetDecision,
    RmOpts,
    RmPlan,
    isRmRootPath,
    isRmDotOrDotDotTarget,
} from "./rm_opts.cp";

export {
    SHELL_CAT_MAX_INPUTS,
    ShellCatError,
    CatInputKind,
    CatInputEntry,
    CatOpts,
    CatPlan,
    CatLineFormatter,
    formatCatBytes,
} from "./cat_opts.cp";

export {
    SHELL_LS_MAX_PATHS,
    ShellLsError,
    LsSortKey,
    LsEntryKind,
    LsOpts,
    LsPlan,
    LsEntryMeta,
    formatLsMode,
    formatLsHumanSize,
    sortLsEntries,
} from "./ls_opts.cp";

export {
    SHELL_WHICH_MAX_ARGS,
    SHELL_WHICH_MAX_PATH_LEN,
    ShellWhichError,
    WhichPlatform,
    WhichProbeFn,
    PathEnvIter,
    WhichResolveOutcome,
    WhichPlan,
    isDirectCommandPath,
    joinDirAndCommand,
    formatWhichNotFound,
    resolveWhichSingle,
    evaluateWhichExitCode,
} from "./which_plan.cp";

export {
    SHELL_PIPELINE_MAX_STAGES,
    SHELL_PIPELINE_NO_PIPE,
    ShellPipelineError,
    PipeStageKind,
    PipeIoSource,
    PipeIoSink,
    PipeMachineState,
    PipeExitPolicy,
    PipeChannelSlot,
    PipeStagePlan,
    PipeCleanupAction,
    ShellPipelinePlanner,
} from "./pipeline_plan.cp";

export {
    YES_BUFSIZ,
    YES_HALF_BUFSIZ,
    YES_CHUNKS_PER_TICK,
    ShellYesError,
    YesState,
    YesTickAction,
    YesLayout,
    computeYesLineLen,
    computeYesLayout,
    fillYesChunk,
    YesStreamState,
} from "./yes_stream.cp";

export {
    SHELL_IF_MAX_STMTS,
    SHELL_IF_MAX_ELSE_PARTS,
    ShellIfError,
    IfBranchPhase,
    IfStepKind,
    IfStepResult,
    ShellIfStateMachine,
} from "./if_state.cp";

// Quick synchronous shell execution helper: sh("echo hello | cat")
pub fn sh(command_str: []const u8): ShellResult {
    let parser = Parser.init(command_str);
    let script_ptr = parser.parseScriptHeap();
    defer {
        script_ptr.destroy();
        std.heap.page_allocator.destroy(script_ptr);
    }
    let sess = ShellSession.init();
    defer sess.~init();
    return Executor.executeScriptPtrWithSession(script_ptr, &sess);
}

// Compatibility alias for exec("...")
pub fn exec(command_str: []const u8): ShellResult {
    return sh(command_str);
}

pub struct Shell {
    pub static fn run(command_str: []const u8): ShellResult {
        return sh(command_str);
    }

    pub static fn session(): ShellSession {
        return ShellSession.init();
    }
}
