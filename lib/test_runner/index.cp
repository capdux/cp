// Source: Absorbed from /data/dev/bun/src/test_runner/ | Synced: 2026-11-22T13:00:28Z
// ============================================================================
// lib/test_runner/index.cp — Unified Public Exports for cap:test_runner
// ============================================================================

export {
    TestOrderMode,
    Prng64,
    TestOrder,
} from "./order";

export {
    TestStatus,
    TestMode,
    HookKind,
    TestFilter,
    TestContext,
    TestCallback,
    HookCallback,
    TestCase,
    GroupItem,
    TestGroup,
    TestRunSummary,
    TestRunner,
} from "./suite";

export {
    SNAPSHOT_FILE_HEADER,
    SnapshotUpdateMode,
    SnapshotMatchStatus,
    SnapshotMatchResult,
    SnapshotEntry,
    SnapshotStore,
} from "./snapshot";

export {
    ReportFormat,
    PrettyFormatOptions,
    PrettyKind,
    PrettyField,
    PrettyNode,
    PrettyFormat,
    TestReporter,
} from "./reporter";

export {
    IPC_FRAME_HEADER_SIZE,
    IPC_FRAME_MAX_PAYLOAD,
    IPC_TRUNC_MARKER,
    IpcFrameError,
    IpcFrameKind,
    IpcDecodedFrame,
    IpcPayloadReader,
    IpcFrameWriter,
    decodeIpcFrame,
} from "./ipcFrame";

export {
    WORK_STEAL_MAX_WORKERS,
    FileRange,
    WorkStealPool,
} from "./workSteal";

export {
    JUnitTotals,
    JUnitMergeResult,
    CoverageThreshold,
    MergedCoverageFile,
    CoverageAggregator,
    parseJUnitAttrU32,
    appendXmlEscaped,
    mergeJUnitFragments,
} from "./aggregate";

export {
    TEST_SUFFIX_DOT_TEST,
    TEST_SUFFIX_UNDER_TEST,
    TEST_SUFFIX_DOT_SPEC,
    TEST_SUFFIX_UNDER_SPEC,
    TEST_DEFAULT_IGNORE_DIR,
    TEST_DISCOVERY_MAX_PATH,
    isSupportedTestExt,
    hasBunTestSuffix,
    hasTestNameSuffix,
    isExplicitPathArg,
    shouldSkipDirectory,
    matchesDiscoveryGlob,
    TestDiscoveryFilter,
} from "./discovery";

export {
    COORDINATOR_MAX_WORKERS,
    COORDINATOR_MAX_CRASHED_FILES,
    COORDINATOR_DEFAULT_SCALE_UP_MS,
    CoordinatorError,
    WorkerSlotState,
    WorkerSlot,
    CoordinatorConfig,
    DispatchDecisionKind,
    DispatchDecision,
    ParallelCoordinator,
} from "./coordinator";

export {
    IPC_CHANNEL_BUF_CAP,
    IpcChannelError,
    IpcChannelCloseReason,
    IpcChannelPollStatus,
    IpcChannelPollResult,
    IpcChannel,
} from "./ipcChan";

export {
    WORKER_PROC_CAPTURE_CAP,
    WorkerProcError,
    WorkerPipeRole,
    WorkerExitKind,
    WorkerExitStatus,
    WorkerLifecyclePhase,
    WorkerIpcAction,
    WorkerSpawnPolicy,
    WorkerProcessSlot,
    matchesConcurrentTestGlob,
} from "./workerProc";

export {
    DEFAULT_PARALLEL_SCALE_UP_MS,
    WORKER_FILE_DELTA_BYTES,
    PARALLEL_RUN_MAX_FILES,
    PARALLEL_RUN_MAX_WORKERS,
    WORKER_ARGV_MAX_ARGS,
    WORKER_ARGV_ARENA_CAP,
    ParallelRunError,
    effectiveWorkerCount,
    shouldUseParallelPool,
    resolveScaleUpDelayMs,
    ParallelSliceRange,
    ParallelFilePartition,
    WorkerSummarySnapshot,
    WorkerFileDelta,
    WorkerCliOptions,
    WorkerArgSpan,
    WorkerArgvBuilder,
    WorkerEnvPlan,
} from "./parallelRun";

export {
    isAsciiWhitespace,
    ExpectOutcome,
    equalIgnoringWhitespace,
    equalIgnoringWhitespacePos,
    collapseWhitespace,
    startsWith,
    endsWith,
    includesSubstring,
    countOccurrences,
    includesRepeated,
    isCloseTo,
    isWithinI64,
    isWithinF64,
    isEvenI64,
    isOddI64,
    ExpectMatcher,
} from "./expectMatch";

export {
    FAKE_TIMERS_MAX_SLOTS,
    FAKE_TIMERS_MAX_ADVANCE_MS,
    FAKE_TIMERS_DEFAULT_RUN_LIMIT,
    FakeTimerError,
    FakeTimerKind,
    FakeTimerSlot,
    FakeTimerFired,
    FakeTimerAdvanceResult,
    normalizeFakeDelayMs,
    FakeTimerClock,
} from "./fakeTimers";

export {
    DONE_CALLBACK_DEFAULT_TIMEOUT_MS,
    DONE_CALLBACK_MAX_ERR_MSG,
    DoneCallbackError,
    DoneCallbackState,
    DoneCallback,
} from "./doneCallback";

export {
    MOCK_SPY_MAX_CALLS,
    MockSpyError,
    MockCallOutcome,
    MockCallRecord,
    MockSpy,
} from "./mockSpy";
