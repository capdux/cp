// Source: Absorbed from /data/dev/bun/src/test_runner/ | Synced: 2026-11-20T10:22:50Z
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
