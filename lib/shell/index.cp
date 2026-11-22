// Source: Absorbed from /data/dev/bun/src/shell/ | Synced: 2026-11-22T12:57:27Z
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
export * from "./builtinFs.cp";
export * from "./builtinText.cp";
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
} from "./condExpr.cp";

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
} from "./mkdirOpts.cp";

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
} from "./exitExport.cp";

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
} from "./cdPath.cp";

export {
    SHELL_MV_MAX_SOURCES,
    ShellMvError,
    MvOverwritePolicy,
    MvOpts,
    MvPlan,
} from "./mvOpts.cp";

export {
    SHELL_TOUCH_MAX_PATHS,
    ShellTouchError,
    TouchTimeTarget,
    TouchTimestamp,
    TouchOpts,
} from "./touchOpts.cp";

export {
    SHELL_CP_MAX_SOURCES,
    ShellCpError,
    CpOverwritePolicy,
    CpSymlinkPolicy,
    CpOpts,
    CpPlan,
} from "./cpOpts.cp";

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
} from "./rmOpts.cp";

export {
    SHELL_CAT_MAX_INPUTS,
    ShellCatError,
    CatInputKind,
    CatInputEntry,
    CatOpts,
    CatPlan,
    CatLineFormatter,
    formatCatBytes,
} from "./catOpts.cp";

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
} from "./lsOpts.cp";

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
} from "./which.cp";

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
} from "./pipePlan.cp";

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
} from "./yesStream.cp";

export {
    SHELL_IF_MAX_STMTS,
    SHELL_IF_MAX_ELSE_PARTS,
    ShellIfError,
    IfBranchPhase,
    IfStepKind,
    IfStepResult,
    ShellIfStateMachine,
} from "./ifState.cp";

export {
    SHELL_BINARY_MAX_TERMS,
    ShellBinaryError,
    BinaryLogicOp,
    BinaryStepKind,
    BinaryTerm,
    BinaryStepResult,
    shouldShortCircuitBinary,
    ShellBinaryStateMachine,
} from "./binaryState.cp";

export {
    SHELL_ASSIGNS_MAX_ITEMS,
    SHELL_ASSIGN_MAX_LABEL_LEN,
    SHELL_ASSIGN_MAX_VALUE_LEN,
    ShellAssignsError,
    AssignScopeCtx,
    AssignStepKind,
    AssignStepResult,
    CommittedAssign,
    isValidAssignLabel,
    canSkipPipelineAssigns,
    joinExpandedWords,
    ShellAssignsStateMachine,
} from "./assigns.cp";

export {
    SHELL_SCRIPT_MAX_EXPRS,
    ShellScriptError,
    ScriptHostKind,
    ShellExprKind,
    ScriptStepKind,
    ScriptExprItem,
    ScriptStepResult,
    scriptPropagatesEnvToParent,
    ShellScriptStateMachine,
} from "./scriptState.cp";

export {
    SUBSHELL_MAX_REDIRECT_PATH_LEN,
    SUBSHELL_MAX_ENV_SNAPSHOT,
    SubshellError,
    SubshellParentKind,
    SubshellRunState,
    SubshellRedirectKind,
    SubshellExecPolicy,
    SubshellEnvEntry,
    SubshellStepKind,
    SubshellStepAction,
    SubshellStateMachine,
} from "./subshellState.cp";

export {
    SHELL_ASYNC_MAX_JOBS,
    ShellAsyncError,
    AsyncParentKind,
    AsyncChildKind,
    AsyncRunPhase,
    AsyncStepKind,
    AsyncStepAction,
    ShellAsyncStateMachine,
    ShellAsyncJobTracker,
} from "./asyncState.cp";

export {
    SHELL_IO_MAX_HANDLES,
    SHELL_IO_INVALID_HANDLE,
    ShellIoError,
    ShellInKind,
    ShellOutKind,
    SubprocStdioKind,
    SubprocStdioRoute,
    ShellSubprocStdioPlan,
    ShellInChannel,
    ShellOutChannel,
    ShellIoTriplet,
    ShellIoHandleEntry,
    ShellIoHandleTable,
} from "./ioChannels.cp";

export {
    ENV_STR_MAX_SLOTS,
    ENV_STR_ARENA_CAP,
    ENV_STR_INVALID_SLOT,
    EnvStrError,
    EnvStrTag,
    EnvStr,
    EnvStrSlot,
    EnvStrPool,
} from "./envStr.cp";

export {
    ENV_MAP_MAX_ENTRIES,
    EnvMapError,
    EnvMapEntry,
    ShellEnvMap,
} from "./envMap.cp";

export {
    WORD_EXPAND_MAX_ATOMS,
    WORD_EXPAND_MAX_WORDS,
    WORD_EXPAND_ARENA_CAP,
    WORD_EXPAND_MAX_VARS,
    WordExpandError,
    WordExpandMode,
    WordAtomKind,
    WordExpandStepKind,
    WordVarBinding,
    WordAtom,
    WordExpandStep,
    WordSpan,
    WordExpandOutput,
    stripTrailingNewlines,
    hasUnquotedGlobMeta,
    normalizeUnquotedSubshellStdout,
    expandLeadingTilde,
    ShellWordExpander,
} from "./wordExpand.cp";

export {
    SHELL_PATH_BUILTIN_MAX_ARGS,
    PathBuiltinError,
    BasenameOpts,
    DirnameOpts,
    posixBasename,
    posixBasenameStripSuffix,
    posixDirname,
    runBasenameBuiltin,
    runDirnameBuiltin,
} from "./pathBuiltin.cp";

export {
    SHELL_EXPORT_MAX_VARS,
    ShellExportError,
    isValidShellIdent,
    ShellExportEntry,
    ShellExportTable,
} from "./exportBuiltin.cp";

export {
    PwdOutputKind,
    PwdState,
    PwdMode,
    PwdError,
    PwdResult,
    parsePwdArgs,
    PwdBuiltin,
    runPwdBuiltin,
} from "./pwdBuiltin.cp";

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
